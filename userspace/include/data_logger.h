#pragma once
#include "common.h"
#include <string>

class DataLogger {
public:
    explicit DataLogger(std::string path);
    void log_data(const SensorData& data);
private:
    std::string path_;
};
