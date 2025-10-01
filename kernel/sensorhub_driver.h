#ifndef SENSORHUB_DRIVER_H
#define SENSORHUB_DRIVER_H

#include <linux/ioctl.h>

#define SENSORHUB_IOC_MAGIC 'S'

#define SENSORHUB_RESET_DATA _IO(SENSORHUB_IOC_MAGIC, 1)
#define SENSORHUB_GET_STATUS _IOR(SENSORHUB_IOC_MAGIC, 2, int)

struct sensorhub_data {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    unsigned long timestamp;
};

#endif /* SENSORHUB_DRIVER_H */
