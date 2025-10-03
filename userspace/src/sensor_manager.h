#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "common.h"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

class SensorManager {
public:
    SensorManager();
    ~SensorManager();
    
    bool initialize();
    void shutdown();
    
    SensorData read_sensors();
    void check_alerts();
    bool is_initialized() const { return initialized_; }
    
private:
    void update_thread();
    bool read_from_device(SensorData& data);
    
    int device_fd_;
    std::atomic<bool> initialized_;
    std::atomic<bool> running_;
    std::thread update_thread_;
    std::mutex data_mutex_;
    SensorData last_reading_;
};

#endif // SENSOR_MANAGER_H
