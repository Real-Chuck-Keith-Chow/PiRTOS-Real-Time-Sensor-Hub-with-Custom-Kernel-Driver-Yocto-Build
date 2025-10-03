#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <chrono>

struct SensorData {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    uint64_t timestamp;
    
    SensorData() : temperature(0.0), humidity(0.0), 
                  motion_detected(0), button_pressed(0), 
                  timestamp(0) {}
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

void log_message(LogLevel level, const std::string& message);

#endif // COMMON_H
