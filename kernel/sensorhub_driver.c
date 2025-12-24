#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/random.h>

#define DEVICE_NAME "sensorhub"
#define CLASS_NAME "pirtos"
#define I2C_ADDRESS 0x40  // default for many temp/humidity parts; adjust in DT if needed

// Raspberry Pi GPIO BCM numbers per README
#define GPIO_PIR    17
#define GPIO_BUTTON 23
#define GPIO_LED    22

static dev_t dev_number;
static int major_number;
static struct class* sensorhub_class = NULL;
static struct device* sensorhub_device = NULL;
static struct cdev sensorhub_cdev;

// Sensor data structure (must match userspace SensorManager view)
struct sensor_data {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    unsigned long timestamp;
};

static struct sensor_data current_data;
static DECLARE_WAIT_QUEUE_HEAD(data_wait_queue);
static int data_ready;

static int pir_irq_number;
static int button_irq_number;

// I2C device
static struct i2c_client *sensor_client = NULL;

// File operations
static int device_open(struct inode *inodep, struct file *filep) {
    pr_info("PiRTOS: Device opened\n");
    return 0;
}

static int device_release(struct inode *inodep, struct file *filep) {
    pr_info("PiRTOS: Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    int ret;

    // Wait for data to be ready
    ret = wait_event_interruptible(data_wait_queue, data_ready);
    if (ret)
        return ret;

    if (copy_to_user(buffer, &current_data, sizeof(struct sensor_data)))
        return -EFAULT;

    data_ready = 0;
    return sizeof(struct sensor_data);
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case 0x01: // reset data_ready
        data_ready = 0;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .open = device_open,
    .read = device_read,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

// Interrupt handler for PIR sensor
static irqreturn_t pir_interrupt_handler(int irq, void *dev_id) {
    int motion_value = gpio_get_value(GPIO_PIR);

    current_data.motion_detected = motion_value;
    current_data.timestamp = jiffies;
    data_ready = 1;

    // Toggle LED on motion detection
    gpio_set_value(GPIO_LED, motion_value);

    wake_up_interruptible(&data_wait_queue);
    pr_info("PiRTOS: Motion %s\n", motion_value ? "detected" : "cleared");
    return IRQ_HANDLED;
}

// Button interrupt handler
static irqreturn_t button_interrupt_handler(int irq, void *dev_id) {
    current_data.button_pressed = 1;
    current_data.timestamp = jiffies;
    data_ready = 1;

    wake_up_interruptible(&data_wait_queue);
    pr_info("PiRTOS: Button pressed\n");
    return IRQ_HANDLED;
}

// Attempt real I2C temperature/humidity reading; fall back to pseudo-random
static void read_temperature_humidity(void) {
    if (sensor_client) {
        // Simple example: read two 16-bit registers (0x00 temp, 0x01 humidity)
        s32 temp_raw = i2c_smbus_read_word_data(sensor_client, 0x00);
        s32 hum_raw  = i2c_smbus_read_word_data(sensor_client, 0x01);

        if (temp_raw >= 0 && hum_raw >= 0) {
            // Interpret as hundredths; adjust as needed for your part
            current_data.temperature = (float)be16_to_cpu((__be16)temp_raw) / 100.0f;
            current_data.humidity    = (float)be16_to_cpu((__be16)hum_raw)  / 100.0f;
            current_data.timestamp = jiffies;
            data_ready = 1;
            wake_up_interruptible(&data_wait_queue);
            return;
        }
    }

    // Fallback: simulated data for testing
    current_data.temperature = 23.5f + (prandom_u32() % 100) / 10.0f;
    current_data.humidity = 45.0f + (prandom_u32() % 300) / 10.0f;
    current_data.timestamp = jiffies;
    data_ready = 1;
    wake_up_interruptible(&data_wait_queue);
}

// Timer for periodic sensor reading
static struct timer_list sensor_timer;

static void sensor_timer_callback(struct timer_list *t) {
    read_temperature_humidity();
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(2000));
}

// I2C driver functions
static int sensor_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    pr_info("PiRTOS: I2C sensor probed\n");
    sensor_client = client;
    return 0;
}

static int sensor_i2c_remove(struct i2c_client *client) {
    sensor_client = NULL;
    pr_info("PiRTOS: I2C sensor removed\n");
    return 0;
}

static const struct i2c_device_id sensor_i2c_id[] = {
    { "pirtos_sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sensor_i2c_id);

static struct i2c_driver sensor_i2c_driver = {
    .driver = {
        .name = "pirtos_sensor",
        .owner = THIS_MODULE,
    },
    .probe = sensor_i2c_probe,
    .remove = sensor_i2c_remove,
    .id_table = sensor_i2c_id,
};

static int __init sensorhub_init(void) {
    int ret;

    pr_info("PiRTOS: Initializing SensorHub driver\n");

    // Register character device
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_alert("PiRTOS: Failed to allocate device number\n");
        return ret;
    }
    major_number = MAJOR(dev_number);

    cdev_init(&sensorhub_cdev, &fops);
    sensorhub_cdev.owner = THIS_MODULE;

    ret = cdev_add(&sensorhub_cdev, dev_number, 1);
    if (ret < 0) {
        pr_alert("PiRTOS: Failed to add character device\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    // Create device class
    sensorhub_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sensorhub_class)) {
        pr_alert("PiRTOS: Failed to create device class\n");
        cdev_del(&sensorhub_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(sensorhub_class);
    }

    // Create device node
    sensorhub_device = device_create(sensorhub_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(sensorhub_device)) {
        pr_alert("PiRTOS: Failed to create device\n");
        class_destroy(sensorhub_class);
        cdev_del(&sensorhub_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(sensorhub_device);
    }

    // GPIO setup (BCM numbering)
    ret = gpio_request_one(GPIO_PIR, GPIOF_IN, "pir");
    if (ret) {
        pr_warn("PiRTOS: Failed to request PIR GPIO\n");
    } else {
        pir_irq_number = gpio_to_irq(GPIO_PIR);
        ret = request_irq(pir_irq_number, pir_interrupt_handler,
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                          "pir_irq", NULL);
        if (ret)
            pr_warn("PiRTOS: Failed to request PIR IRQ\n");
    }

    ret = gpio_request_one(GPIO_BUTTON, GPIOF_IN | GPIOF_PULLUP, "button");
    if (ret) {
        pr_warn("PiRTOS: Failed to request button GPIO\n");
    } else {
        button_irq_number = gpio_to_irq(GPIO_BUTTON);
        ret = request_irq(button_irq_number, button_interrupt_handler,
                          IRQF_TRIGGER_FALLING, "button_irq", NULL);
        if (ret)
            pr_warn("PiRTOS: Failed to request button IRQ\n");
    }

    ret = gpio_request_one(GPIO_LED, GPIOF_OUT_INIT_LOW, "led");
    if (ret)
        pr_warn("PiRTOS: Failed to request LED GPIO\n");

    // Initialize sensor timer
    timer_setup(&sensor_timer, sensor_timer_callback, 0);
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(1000));

    // Register I2C driver
    ret = i2c_add_driver(&sensor_i2c_driver);
    if (ret)
        pr_warn("PiRTOS: Failed to register I2C driver\n");

    memset(&current_data, 0, sizeof(current_data));
    pr_info("PiRTOS: SensorHub driver loaded successfully (major=%d)\n", major_number);
    return 0;
}

static void __exit sensorhub_exit(void) {
    del_timer_sync(&sensor_timer);

    if (pir_irq_number)
        free_irq(pir_irq_number, NULL);
    if (button_irq_number)
        free_irq(button_irq_number, NULL);

    gpio_free(GPIO_PIR);
    gpio_free(GPIO_BUTTON);
    gpio_free(GPIO_LED);

    i2c_del_driver(&sensor_i2c_driver);

    if (!IS_ERR(sensorhub_device))
        device_destroy(sensorhub_class, dev_number);
    if (!IS_ERR(sensorhub_class))
        class_destroy(sensorhub_class);

    cdev_del(&sensorhub_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("PiRTOS: SensorHub driver unloaded\n");
}

module_init(sensorhub_init);
module_exit(sensorhub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PiRTOS Developer");
MODULE_DESCRIPTION("Real-time IoT Sensor Hub Driver");
MODULE_VERSION("1.1");
