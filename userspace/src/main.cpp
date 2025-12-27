#include "sensor_manager.h"
#include "data_logger.h"
#include "network_manager.h"
#include "config.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

std::atomic<bool> running{true};

void signal_handler(int) { running = false; }

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    SensorManager sensor_manager;
    if (!sensor_manager.initialize()) {
        std::cerr << "Failed to initialize SensorManager. Exiting." << std::endl;
        return 1;
    }

    DataLogger data_logger("sensor_data.db");
    NetworkManager network_manager;

    std::cout << "PiRTOS Sensor Hub Started. Press Ctrl+C to exit." << std::endl;

    using clock = std::chrono::steady_clock;
    auto next_log = clock::now();
    auto next_alert = clock::now();

    while (running) {
        auto now = clock::now();

        if (now >= next_log) {
            auto data = sensor_manager.read_sensors();
            data_logger.log_data(data);
            network_manager.broadcast_data(data);
            next_log = now + std::chrono::milliseconds(DATA_LOG_INTERVAL_MS);
        }

        if (now >= next_alert) {
            sensor_manager.check_alerts();
            next_alert = now + std::chrono::milliseconds(5000);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    sensor_manager.shutdown();
    std::cout << "PiRTOS Sensor Hub Stopped." << std::endl;
    return 0;
}

