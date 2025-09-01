# PiRTOS: Real-Time IoT Sensor Hub on Raspberry Pi  
> **An Embedded Linux + Kernel Driver + Yocto Project**

![Raspberry Pi](https://img.shields.io/badge/Platform-Raspberry%20Pi-red?style=flat-square)
![Language](https://img.shields.io/badge/C/C++-100%25-blue?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)
![Status](https://img.shields.io/badge/Status-Active-brightgreen?style=flat-square)

---

## **📌 Overview**
**PiRTOS** is a **real-time IoT sensor hub** built on **Raspberry Pi** that demonstrates expertise in:

- **Embedded C/C++**: Multi-threaded applications, interrupt handlers, and memory-mapped I/O.
- **FreeRTOS Concepts**: Task scheduling, concurrency, and inter-thread communication.
- **Embedded Linux**: Custom Yocto builds, device tree overlays, and low-level hardware access.
- **Kernel Module Development**: Writing Linux drivers to expose sensor data via `/dev`.
- **Networking**: Optional REST API + WebSocket dashboard for live sensor visualization.

This project showcases **end-to-end embedded development**, from **bare-metal I²C/SPI GPIO drivers** to **Linux kernel modules** and **optimized Yocto-based builds**.

---

## **⚡ Features**
- 🟢 **Real-Time Sensor Integration** — Reads temperature, humidity, and motion data via **I²C/SPI**.
- 🟢 **Interrupt-Driven GPIO** — Handles button presses and sensor thresholds with minimal latency.
- 🟢 **Multi-Threaded C++ App** — Implements concurrency for sensor polling, logging, and networking.
- 🟢 **Custom Linux Kernel Module** — Exposes sensor data via `/dev/sensorhub`.
- 🟢 **Yocto/OpenEmbedded Build** *(optional but recommended)* — Builds a **minimal, optimized Linux image**.
- 🟢 **REST API & Web Dashboard** *(optional)* — Displays live readings via Flask + Chart.js.

---

## **🛠️ Tech Stack**
| **Layer**           | **Technologies Used**                                    |
|---------------------|-----------------------------------------------------------|
**Language**          | C, C++17, Python *(optional for dashboard)*               |
**Embedded Linux**    | Yocto, OpenEmbedded, BitBake, Poky                        |
**Driver Dev**        | Linux Kernel Modules, Device Tree Compiler (DTC), DKMS    |
**Concurrency**       | POSIX Threads, Mutexes, Semaphores                        |
**Hardware Access**   | pigpio, libi2c, spidev                                   |
**Networking**        | MQTT / REST API / WebSockets *(optional)*                 |
**Visualization**     | Flask, Chart.js, SQLite *(optional)*                      |

---

## **📂 Project Structure**
```bash
PiRTOS/
├── src/                      # Core C/C++ source code
│   ├── main.cpp              # Multi-threaded app entry point
│   ├── drivers/              # Low-level sensor drivers (I2C/SPI/GPIO)
│   ├── kernel_module/        # Custom Linux kernel driver
│   ├── utils/                # Helper libs: logging, threading, etc.
│   └── CMakeLists.txt        # Build configuration
│
├── yocto/                    # Yocto build files for custom Pi image
│   ├── recipes/              # Custom recipes for drivers & dependencies
│   └── configs/              # Image & kernel configs
│
├── dashboard/ *(optional)*   # Flask-based visualization dashboard
│   ├── app.py                # REST API server
│   ├── static/               # Chart.js graphs + frontend assets
│   └── templates/            # HTML templates
│
├── docs/                     # Diagrams, schematics, notes
│
└── README.md                # This file
