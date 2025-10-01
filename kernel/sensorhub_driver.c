#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/slab.h>

#define DEVICE_NAME "sensorhub"
#define CLASS_NAME "pirtos"
#define BUFFER_SIZE 1024
#define I2C_ADDRESS 0x40  // Typical I2C address for temp/humidity sensors

static int major_number;
static struct class* sensorhub_class = NULL;
static struct device* sensorhub_device = NULL;
static struct cdev sensorhub_cdev;

// Sensor data structure
struct sensor_data {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    unsigned long timestamp;
};

static struct sensor_data current_data;
static DECLARE_WAIT_QUEUE_HEAD(data_wait_queue);
static int data_ready = 0;

// GPIO and I2C handles
static struct gpio_desc *pir_gpio = NULL;
static struct gpio_desc *button_gpio = NULL;
static struct gpio_desc *led_gpio = NULL;
static int pir_irq_number;

// I2C device structure
static struct i2c_client *sensor_client = NULL;

// File operations
static int device_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "PiRTOS: Device opened\n");
    return 0;
}

static int device_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "PiRTOS: Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int bytes_read = 0;
    int ret;
    
    // Wait for data to be ready
    ret = wait_event_interruptible(data_wait_queue, data_ready);
    if (ret) {
        return ret;
    }
    
    if (copy_to_user(buffer, &current_data, sizeof(struct sensor_data))) {
        return -EFAULT;
    }
    
    bytes_read = sizeof(struct sensor_data);
    data_ready = 0;
    
    return bytes_read;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case 0x01:  // Reset data
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
    int motion_value = gpiod_get_value(pir_gpio);
    
    current_data.motion_detected = motion_value;
    current_data.timestamp = jiffies;
    data_ready = 1;
    
    // Toggle LED on motion detection
    gpiod_set_value(led_gpio, motion_value);
    
    wake_up_interruptible(&data_wait_queue);
    printk(KERN_INFO "PiRTOS: Motion %s\n", motion_value ? "detected" : "cleared");
    return IRQ_HANDLED;
}

// Button interrupt handler
static irqreturn_t button_interrupt_handler(int irq, void *dev_id) {
    current_data.button_pressed = 1;
    current_data.timestamp = jiffies;
    data_ready = 1;
    
    wake_up_interruptible(&data_wait_queue);
    printk(KERN_INFO "PiRTOS: Button pressed\n");
    return IRQ_HANDLED;
}

// Simulated I2C temperature/humidity reading
static void read_temperature_humidity(void) {
    // For real implementation, this would read from actual I2C sensor
    // Simulating data for demonstration
    current_data.temperature = 23.5 + (prandom_u32() % 100) / 10.0;
    current_data.humidity = 45.0 + (prandom_u32() % 300) / 10.0;
    current_data.timestamp = jiffies;
}

// Timer for periodic sensor reading
static struct timer_list sensor_timer;

static void sensor_timer_callback(struct timer_list *t) {
    read_temperature_humidity();
    data_ready = 1;
    wake_up_interruptible(&data_wait_queue);
    
    // Restart timer
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(2000));
}

// I2C driver functions
static int sensor_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    printk(KERN_INFO "PiRTOS: I2C sensor probed\n");
    sensor_client = client;
    return 0;
}

static int sensor_i2c_remove(struct i2c_client *client) {
    sensor_client = NULL;
    printk(KERN_INFO "PiRTOS: I2C sensor removed\n");
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
    int ret = 0;
    
    printk(KERN_INFO "PiRTOS: Initializing SensorHub driver\n");
    
    // Register character device
    ret = alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "PiRTOS: Failed to allocate device number\n");
        return ret;
    }
    
    cdev_init(&sensorhub_cdev, &fops);
    sensorhub_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&sensorhub_cdev, major_number, 1);
    if (ret < 0) {
        printk(KERN_ALERT "PiRTOS: Failed to add character device\n");
        unregister_chrdev_region(major_number, 1);
        return ret;
    }
    
    // Create device class
    sensorhub_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sensorhub_class)) {
        printk(KERN_ALERT "PiRTOS: Failed to create device class\n");
        cdev_del(&sensorhub_cdev);
        unregister_chrdev_region(major_number, 1);
        return PTR_ERR(sensorhub_class);
    }
    
    // Create device
    sensorhub_device = device_create(sensorhub_class, NULL, major_number, NULL, DEVICE_NAME);
    if (IS_ERR(sensorhub_device)) {
        printk(KERN_ALERT "PiRTOS: Failed to create device\n");
        class_destroy(sensorhub_class);
        cdev_del(&sensorhub_cdev);
        unregister_chrdev_region(major_number, 1);
        return PTR_ERR(sensorhub_device);
    }
    
    // GPIO setup
    pir_gpio = gpiod_get_index(NULL, "pir", 0, GPIOD_IN);
    if (IS_ERR(pir_gpio)) {
        printk(KERN_WARNING "PiRTOS: Failed to get PIR GPIO\n");
    } else {
        pir_irq_number = gpiod_to_irq(pir_gpio);
        ret = request_irq(pir_irq_number, pir_interrupt_handler, 
                         IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                         "pir_irq", NULL);
        if (ret) {
            printk(KERN_WARNING "PiRTOS: Failed to request PIR IRQ\n");
        }
    }
    
    button_gpio = gpiod_get_index(NULL, "button", 0, GPIOD_IN);
    if (IS_ERR(button_gpio)) {
        printk(KERN_WARNING "PiRTOS: Failed to get button GPIO\n");
    } else {
        int button_irq = gpiod_to_irq(button_gpio);
        ret = request_irq(button_irq, button_interrupt_handler, 
                         IRQF_TRIGGER_FALLING, "button_irq", NULL);
        if (ret) {
            printk(KERN_WARNING "PiRTOS: Failed to request button IRQ\n");
        }
    }
    
    led_gpio = gpiod_get_index(NULL, "led", 0, GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio)) {
        printk(KERN_WARNING "PiRTOS: Failed to get LED GPIO\n");
    }
    
    // Initialize sensor timer
    timer_setup(&sensor_timer, sensor_timer_callback, 0);
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(1000));
    
    // Register I2C driver
    ret = i2c_add_driver(&sensor_i2c_driver);
    if (ret) {
        printk(KERN_WARNING "PiRTOS: Failed to register I2C driver\n");
    }
    
    // Initialize sensor data
    memset(&current_data, 0, sizeof(current_data));
    
    printk(KERN_INFO "PiRTOS: SensorHub driver loaded successfully\n");
    return 0;
}

static void __exit sensorhub_exit(void) {
    // Cleanup timer
    del_timer(&sensor_timer);
    
    // Free IRQs
    if (!IS_ERR(pir_gpio)) {
        free_irq(pir_irq_number, NULL);
    }
    if (!IS_ERR(button_gpio)) {
        int button_irq = gpiod_to_irq(button_gpio);
        free_irq(button_irq, NULL);
    }
    
    // Release GPIOs
    if (!IS_ERR(pir_gpio)) gpiod_put(pir_gpio);
    if (!IS_ERR(button_gpio)) gpiod_put(button_gpio);
    if (!IS_ERR(led_gpio)) gpiod_put(led_gpio);
    
    // Unregister I2C driver
    i2c_del_driver(&sensor_i2c_driver);
    
    // Cleanup device
    if (!IS_ERR(sensorhub_device)) {
        device_destroy(sensorhub_class, major_number);
    }
    if (!IS_ERR(sensorhub_class)) {
        class_destroy(sensorhub_class);
    }
    
    // Cleanup character device
    cdev_del(&sensorhub_cdev);
    unregister_chrdev_region(major_number, 1);
    
    printk(KERN_INFO "PiRTOS: SensorHub driver unloaded\n");
}

module_init(sensorhub_init);
module_exit(sensorhub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PiRTOS Developer");
MODULE_DESCRIPTION("Real-time IoT Sensor Hub Driver");
MODULE_VERSION("1.0");
