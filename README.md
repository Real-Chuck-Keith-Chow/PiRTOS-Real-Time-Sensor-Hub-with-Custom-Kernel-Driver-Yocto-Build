# PiRTOS: Real-Time IoT Sensor Hub on Raspberry Pi
**An Embedded Linux + Kernel Driver + Yocto Project**

---

## 📌 Overview
PiRTOS is a real-time IoT sensor hub built on Raspberry Pi that demonstrates expertise in:

- **Embedded C/C++** — Multi-threaded applications, interrupt handlers, and memory-mapped I/O  
- **FreeRTOS Concepts** — Task scheduling, concurrency, and inter-thread communication  
- **Embedded Linux** — Custom Yocto builds, device tree overlays, and low-level hardware access  
- **Kernel Module Development** — Writing Linux drivers to expose sensor data via `/dev`  
- **Networking** — Optional REST API + WebSocket dashboard for live sensor visualization  

This project showcases end-to-end embedded development, from bare-metal GPIO/I²C drivers to Linux kernel modules and optimized Yocto-based builds.

---

## ⚡ Features
- 🟢 **Real-Time Sensor Integration** — Reads temperature, humidity, and motion data via GPIO/I²C  
- 🟢 **Interrupt-Driven GPIO** — Handles motion detection and button presses with minimal latency  
- 🟢 **Multi-Threaded C++ App** — Implements concurrency for sensor polling, logging, and networking  
- 🟢 **Custom Linux Kernel Module** — Exposes sensor data via `/dev/sensorhub`  
- 🟢 **Yocto/OpenEmbedded Build (not done yet, still in the testing stage)** — Builds a minimal, optimized Linux image  
- 🟢 **REST API & Web Dashboard (not done yet, still in the testing stage)** — Displays live readings via Flask + Chart.js  

---

## 🛠️ Tech Stack
| Layer            | Technologies Used |
|------------------|-------------------|
| Language         | C, C++17, Python (optional for dashboard) |
| Embedded Linux   | Yocto, OpenEmbedded, BitBake, Poky |
| Driver Dev       | Linux Kernel Modules, Device Tree Compiler (DTC), DKMS |
| Concurrency      | POSIX Threads, Mutexes, Semaphores |
| Hardware Access  | libgpiod, libi2c, spidev |
| Networking       | MQTT / REST API / WebSockets (optional) |
| Visualization    | Flask, Chart.js, SQLite (optional) |

---

## 🔌 Circuit Setup

### Components
- **Raspberry Pi 4 (or Pi 3)**  
- **PIR Motion Sensor** (Adafruit Mini PIR 4871 or HC-SR501) → GPIO interrupt  
- **Temp/Humidity Sensor** (Adafruit 3721 I²C breakout *recommended*, or DHT11) → I²C / single-wire  
- **Push-button** → GPIO input with pull-up  
- **LED + 330 Ω resistor** → GPIO output  
- **Breadboard + jumper wires**  

### Wiring (Recommended Pin Mapping)

| Device             | Pi Pin | GPIO   | Notes                        |
|--------------------|--------|--------|------------------------------|
| PIR VCC            | Pin 2  | 5V     | Power                        |
| PIR OUT            | Pin 11 | GPIO17 | Motion interrupt input       |
| PIR GND            | Pin 6  | GND    | Ground                       |
| Temp/Humidity VCC  | Pin 1  | 3.3V   | Power                        |
| Temp/Humidity SDA  | Pin 3  | GPIO2  | I²C data                     |
| Temp/Humidity SCL  | Pin 5  | GPIO3  | I²C clock                    |
| Temp/Humidity GND  | Pin 9  | GND    | Ground                       |
| Button             | Pin 16 | GPIO23 | Input (with pull-up enabled) |
| Button GND         | Pin 14 | GND    |                              |
| LED Anode (+)      | Pin 15 | GPIO22 | Through 330Ω resistor        |
| LED Cathode (–)    | Pin 20 | GND    |                              |

<img width="944" height="1259" alt="image" src="https://github.com/user-attachments/assets/2445b5d1-d35f-4c5e-a07a-9a46fbaa6706" />


---

