#include "network_manager.h"
#include <iostream>

void NetworkManager::broadcast_data(const SensorData& data) {
    // Stub: replace with MQTT/REST/WebSocket implementation.
    std::cout << "[NET] broadcast "
              << "temp=" << data.temperature << "C "
              << "hum=" << data.humidity << "% "
              << "motion=" << data.motion_detected << " "
              << "button=" << data.button_pressed << " "
              << "ts=" << data.timestamp
              << std::endl;
}
