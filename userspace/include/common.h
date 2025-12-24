#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <chrono>
#include <iostream>
#include <string>

struct SensorData {
    float temperature;
    float humidity;
    int motion_detected;
    int button_pressed;
    uint64_t timestamp;

    SensorData()
        : temperature(0.0f), humidity(0.0f),
          motion_detected(0), button_pressed(0),
          timestamp(0) {}
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

inline void log_message(LogLevel level, const std::string& message) {
    const char* tag = "";
    switch (level) {
    case LogLevel::DEBUG:   tag = "DEBUG"; break;
    case LogLevel::INFO:    tag = "INFO"; break;
    case LogLevel::WARNING: tag = "WARN"; break;
    case LogLevel::ERROR:   tag = "ERROR"; break;
    }
    std::cout << "[" << tag << "] " << message << std::endl;
}

#endif // COMMON_H
