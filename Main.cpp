/**
 * PIRTOS: Real-Time IoT Sensor Hub on Raspberry Pi
 * Main Application - Multi-threaded sensor hub controller
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>

// Custom kernel module interface
#ifndef _PIRTOS_DRIVER_H
#define _PIRTOS_DRIVER_H

// IOCTL commands for our kernel module
#define SENSORHUB_IOC_MAGIC 'S'
#define SENSORHUB_GET_TEMP _IOR(SENSORHUB_IOC_MAGIC, 1, int)
#define SENSORHUB_GET_HUMIDITY _IOR(SENSORHUB_IOC_MAGIC, 2, int)
#define SENSORHUB_GET_MOTION _IOR(SENSORHUB_IOC_MAGIC, 3, int)
#define SENSORHUB_SET_THRESHOLD _IOW(SENSORHUB_IOC_MAGIC, 4, int)

#endif

// Global variables for graceful shutdown
std::atomic<bool> running{true};
std::mutex data_mutex;

// Sensor data structure
struct SensorData {
    float temperature;
    float humidity;
    bool motion_detected;
    uint64_t timestamp;
};

// Current sensor readings
SensorData current_data;

// Kernel module file descriptor
int kernel_fd = -1;

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

/**
 * Initialize communication with kernel module
 */
bool init_kernel_module() {
    kernel_fd = open("/dev/sensorhub", O_RDWR);
    if (kernel_fd < 0) {
        std::cerr << "Failed to open kernel module: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "Successfully opened kernel module" << std::endl;
    return true;
}

/**
 * Read temperature from kernel module
 */
float read_temperature() {
    int temp = 0;
    if (ioctl(kernel_fd, SENSORHUB_GET_TEMP, &temp) < 0) {
        std::cerr << "Failed to read temperature: " << strerror(errno) << std::endl;
        return -1.0f;
    }
    return temp / 100.0f; // Assuming kernel returns temperature * 100
}

/**
 * Read humidity from kernel module
 */
float read_humidity() {
    int humidity = 0;
    if (ioctl(kernel_fd, SENSORHUB_GET_HUMIDITY, &humidity) < 0) {
        std::cerr << "Failed to read humidity: " << strerror(errno) << std::endl;
        return -1.0f;
    }
    return humidity / 100.0f; // Assuming kernel returns humidity * 100
}

/**
 * Read motion detection from kernel module
 */
bool read_motion() {
    int motion = 0;
    if (ioctl(kernel_fd, SENSORHUB_GET_MOTION, &motion) < 0) {
        std::cerr << "Failed to read motion: " << strerror(errno) << std::endl;
        return false;
    }
    return motion != 0;
}

/**
 * Set threshold for sensor interrupts
 */
bool set_sensor_threshold(int threshold) {
    if (ioctl(kernel_fd, SENSORHUB_SET_THRESHOLD, &threshold) < 0) {
        std::cerr << "Failed to set threshold: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

/**
 * Sensor polling thread
 * Reads data from sensors at regular intervals
 */
void sensor_polling_thread() {
    std::cout << "Sensor polling thread started" << std::endl;
    
    while (running) {
        // Read data from kernel module
        float temp = read_temperature();
        float humidity = read_humidity();
        bool motion = read_motion();
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        // Update shared data with lock
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            current_data.temperature = temp;
            current_data.humidity = humidity;
            current_data.motion_detected = motion;
            current_data.timestamp = timestamp;
        }
        
        // Sleep for polling interval (e.g., 1 second)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Sensor polling thread terminated" << std::endl;
}

/**
 * Data logging thread
 * Logs sensor data to file or database
 */
void data_logging_thread() {
    std::cout << "Data logging thread started" << std::endl;
    
    // Open log file
    FILE* log_file = fopen("/var/log/pirtos_sensor.log", "a");
    if (!log_file) {
        std::cerr << "Failed to open log file" << std::endl;
        return;
    }
    
    while (running) {
        // Copy data to avoid long lock times
        SensorData local_data;
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            local_data = current_data;
        }
        
        // Log data if valid
        if (local_data.temperature > -40.0f && local_data.humidity >= 0.0f) {
            fprintf(log_file, "[%llu] Temp: %.2f°C, Humidity: %.2f%%, Motion: %s\n",
                   local_data.timestamp,
                   local_data.temperature,
                   local_data.humidity,
                   local_data.motion_detected ? "YES" : "NO");
            fflush(log_file);
        }
        
        // Sleep for logging interval (e.g., 5 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    fclose(log_file);
    std::cout << "Data logging thread terminated" << std::endl;
}

/**
 * Network communication thread (optional)
 * Sends data to REST API or MQTT broker
 */
void network_thread() {
    std::cout << "Network thread started" << std::endl;
    
    while (running) {
        // Copy data to avoid long lock times
        SensorData local_data;
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            local_data = current_data;
        }
        
        // TODO: Implement network communication
        // This could be MQTT, REST API calls, WebSocket updates, etc.
        
        // Sleep for network update interval (e.g., 10 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    std::cout << "Network thread terminated" << std::endl;
}

/**
 * Main application entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "PIRTOS: Real-Time IoT Sensor Hub Starting..." << std::endl;
    
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Initialize kernel module
    if (!init_kernel_module()) {
        std::cerr << "Failed to initialize kernel module. Exiting." << std::endl;
        return 1;
    }
    
    // Set sensor threshold (example value)
    if (!set_sensor_threshold(2500)) { // 25.00°C
        std::cerr << "Warning: Failed to set sensor threshold" << std::endl;
    }
    
    // Create threads
    std::vector<std::thread> threads;
    
    threads.emplace_back(sensor_polling_thread);
    threads.emplace_back(data_logging_thread);
    threads.emplace_back(network_thread);
    
    // Main loop (could handle CLI or other tasks)
    while (running) {
        // Display current readings occasionally
        static int counter = 0;
        if (++counter % 10 == 0) {
            SensorData local_data;
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                local_data = current_data;
            }
            
            std::cout << "Current: " << local_data.temperature << "°C, " 
                      << local_data.humidity << "%, Motion: " 
                      << (local_data.motion_detected ? "YES" : "NO") << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clean up kernel module
    if (kernel_fd >= 0) {
        close(kernel_fd);
    }
    
    std::cout << "PIRTOS: Real-Time IoT Sensor Hub Shutdown Complete" << std::endl;
    return 0;
}
