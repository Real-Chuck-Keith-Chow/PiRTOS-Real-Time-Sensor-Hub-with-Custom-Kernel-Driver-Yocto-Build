#include "sensor_manager.h"
#include "task_scheduler.h"
#include "data_logger.h"
#include "network_manager.h"
#include "config.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Initialize components
        SensorManager sensor_manager;
        DataLogger data_logger("sensor_data.db");
        NetworkManager network_manager;
        
        TaskScheduler scheduler;
        
        // Add tasks
        scheduler.add_task([&]() {
            auto data = sensor_manager.read_sensors();
            data_logger.log_data(data);
        }, 2000); // Every 2 seconds
        
        scheduler.add_task([&]() {
            auto data = sensor_manager.read_sensors();
            network_manager.broadcast_data(data);
        }, 1000); // Every 1 second
        
        scheduler.add_task([&]() {
            sensor_manager.check_alerts();
        }, 5000); // Every 5 seconds
        
        std::cout << "PiRTOS Sensor Hub Started. Press Ctrl+C to exit." << std::endl;
        
        // Main loop
        while (running) {
            scheduler.run();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "PiRTOS Sensor Hub Stopped." << std::endl;
    return 0;
}
