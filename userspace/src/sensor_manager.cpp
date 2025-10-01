#include "sensor_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdexcept>

SensorManager::SensorManager() {
    // Open the character device
    fd = open("/dev/sensorhub", O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Failed to open sensorhub device");
    }
}

SensorManager::~SensorManager() {
    if (fd >= 0) {
        close(fd);
    }
}

SensorData SensorManager::read_sensors() {
    SensorData data;
    
    if (read(fd, &data, sizeof(SensorData)) != sizeof(SensorData)) {
        throw std::runtime_error("Failed to read from sensorhub device");
    }
    
    return data;
}

void SensorManager::check_alerts() {
    auto data = read_sensors();
    
    // Check temperature threshold
    if (data.temperature > TEMPERATURE_ALERT_THRESHOLD) {
        std::cout << "ALERT: High temperature detected: " << data.temperature << "°C" << std::endl;
    }
    
    // Check motion alerts
    if (data.motion_detected) {
        std::cout << "ALERT: Motion detected!" << std::endl;
    }
}
