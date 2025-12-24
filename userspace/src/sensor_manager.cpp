#include "sensor_manager.h"
#include "config.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
// Mirror of the kernel’s struct sensor_data in sensorhub_driver.c
struct KernelSensorData {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    unsigned long timestamp;
};

// Ioctl command used in the kernel driver (0x01: reset data_ready)
constexpr unsigned int SENSORHUB_IOCTL_RESET = 0x01;
}

SensorManager::SensorManager()
    : device_fd_(-1), initialized_(false), running_(false) {}

SensorManager::~SensorManager() { shutdown(); }

bool SensorManager::initialize() {
    if (initialized_) return true;

    device_fd_ = ::open(DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    if (device_fd_ < 0) {
        std::cerr << "Failed to open sensorhub device " << DEVICE_PATH
                  << ": " << std::strerror(errno) << std::endl;
        return false;
    }

    // Clear any stale readiness flag
    ::ioctl(device_fd_, SENSORHUB_IOCTL_RESET, 0);

    running_ = true;
    update_thread_ = std::thread(&SensorManager::update_thread, this);
    initialized_ = true;

    std::cout << "SensorManager initialized (device=" << DEVICE_PATH << ")"
              << std::endl;
    return true;
}

void SensorManager::shutdown() {
    running_ = false;

    if (update_thread_.joinable()) {
        update_thread_.join();
    }

    if (device_fd_ >= 0) {
        ::close(device_fd_);
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
    if (device_fd_ < 0) return false;

    KernelSensorData kdata{};
    ssize_t bytes_read = ::read(device_fd_, &kdata, sizeof(kdata));

    if (bytes_read == sizeof(kdata)) {
        data.temperature = kdata.temperature;
        data.humidity = kdata.humidity;
        data.motion_detected = kdata.motion_detected;
        data.button_pressed = kdata.button_pressed;
        data.timestamp = kdata.timestamp;
        return true;
    }

    if (bytes_read < 0 && (errno == EAGAIN || errno == EINTR)) {
        return false; // non-fatal, just try again
    }

    // Any other short read or error is unexpected
    std::cerr << "SensorManager read error: " << std::strerror(errno) << std::endl;
    return false;
}

void SensorManager::update_thread() {
    // Block until data arrives; driver wakes readers via wait queue
    while (running_) {
        SensorData new_data;
        if (read_from_device(new_data)) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            last_reading_ = new_data;
        } else {
            // Avoid busy-spin on EAGAIN/EINTR
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void SensorManager::check_alerts() {
    auto data = read_sensors();

    if (data.temperature > TEMPERATURE_ALERT_THRESHOLD) {
        std::cout << "ALERT: High temperature: " << data.temperature << " C" << std::endl;
    }

    if (data.humidity > HUMIDITY_ALERT_THRESHOLD) {
        std::cout << "ALERT: High humidity: " << data.humidity << " %" << std::endl;
    }

    if (data.motion_detected) {
        std::cout << "ALERT: Motion detected!" << std::endl;
    }

    if (data.button_pressed) {
        std::cout << "ALERT: Button pressed!" << std::endl;
    }
}
