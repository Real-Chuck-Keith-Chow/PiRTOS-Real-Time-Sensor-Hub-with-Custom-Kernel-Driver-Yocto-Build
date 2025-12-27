#ifndef CONFIG_H
#define CONFIG_H

// GPIO Pin configuration (BCM numbering, matches README and kernel driver)
#define PIR_GPIO_PIN         17
#define BUTTON_GPIO_PIN      23
#define LED_GPIO_PIN         22

// I2C configuration
#define I2C_BUS              1
#define TEMP_SENSOR_ADDRESS  0x40  // adjust if your sensor differs

// Application settings
#define TEMPERATURE_ALERT_THRESHOLD  30.0
#define HUMIDITY_ALERT_THRESHOLD     80.0
#define DATA_LOG_INTERVAL_MS         2000
#define NETWORK_UPDATE_INTERVAL_MS   1000

// Network configuration (stubs for now)
#define MQTT_BROKER          "localhost"
#define MQTT_PORT            1883
#define MQTT_TOPIC           "pirtos/sensors"

// File paths
#define DEVICE_PATH          "/dev/sensorhub"
#define DATABASE_PATH        "/var/lib/pirtos/sensor_data.db"

#endif // CONFIG_H
