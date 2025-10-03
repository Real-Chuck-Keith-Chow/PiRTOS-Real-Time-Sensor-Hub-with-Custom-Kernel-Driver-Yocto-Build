#include "sensor_manager.h"
#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

SensorManager::SensorManager() 
    : device_fd_(-1), initialized_(false), running_(false) {
}

SensorManager::~SensorManager() {
    shutdown();
}

bool SensorManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Open the character device
    device_fd_ = open(DEVICE_PATH, O_RDONLY);
    if (device_fd_ < 0) {
        std::cerr << "Failed to open sensorhub device: " << strerror(errno) << std::endl;
        return false;
    }
    
    initialized_ = true;
    running_ = true;
    
    // Start update thread
    update_thread_ = std::thread(&SensorManager::update_thread, this);
    
    std::cout << "SensorManager initialized successfully" << std::endl;
    return true;
}

void SensorManager::shutdown() {
    running_ = false;
    
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    
    if (device_fd_ >= 0) {
        close(device_fd_);
        device_fd_ = -1;
    }
    
    initialized_ = false;
    std::cout << "SensorManager shutdown" << std::endl;
}

SensorData SensorManager::read_sensors() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return last_reading_;
}

bool SensorManager::read_from_device(SensorData& data) {
    if (device_fd_ < 0) {
        return false;
    }
    
    struct sensor_data kernel_data;
    ssize_t bytes_read = read(device_fd_, &kernel_data, sizeof(kernel_data));
    
    if (bytes_read == sizeof(kernel_data)) {
        data.temperature = kernel_data.temperature;
        data.humidity = kernel_data.humidity;
        data.motion_detected = kernel_data.motion_detected;
        data.button_pressed = kernel_data.button_pressed;
        data.timestamp = kernel_data.timestamp;
        return true;
    }
    
    return false;
}

void SensorManager::update_thread() {
    while (running_) {
        SensorData new_data;
        if (read_from_device(new_data)) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            last_reading_ = new_data;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SensorManager::check_alerts() {
    auto data = read_sensors();
    
    // Check temperature threshold
    if (data.temperature > TEMPERATURE_ALERT_THRESHOLD) {
        std::cout << "ALERT: High temperature detected: " 
                  << data.temperature << "°C" << std::endl;
    }
    
    // Check humidity threshold
    if (data.humidity > HUMIDITY_ALERT_THRESHOLD) {
        std::cout << "ALERT: High humidity detected: " 
                  << data.humidity << "%" << std::endl;
    }
    
    // Check motion alerts
    if (data.motion_detected) {
        std::cout << "ALERT: Motion detected!" << std::endl;
    }
    
    // Check button press
    if (data.button_pressed) {
        std::cout << "ALERT: Button pressed!" << std::endl;
    }
}
