#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>

class Tmp102Sensor {
public:
    // bus: I2C bus number (e.g. 1 => /dev/i2c-1), addr: 7-bit I2C address (default 0x48)
    explicit Tmp102Sensor(int bus = 1, uint8_t addr = 0x48);
    ~Tmp102Sensor();

    // Non-copyable (file descriptor ownership)
    Tmp102Sensor(const Tmp102Sensor&) = delete;
    Tmp102Sensor& operator=(const Tmp102Sensor&) = delete;

    // Movable
    Tmp102Sensor(Tmp102Sensor&&) noexcept;
    Tmp102Sensor& operator=(Tmp102Sensor&&) noexcept;

    // Open the device (idempotent)
    void open();

    // Read temperature in Celsius / Fahrenheit
    double readCelsius();
    double readFahrenheit() { return readCelsius() * 9.0 / 5.0 + 32.0; }

    // Optional controls
    void setShutdown(bool enable);              // Low-power shutdown
    void setConversionRate(uint8_t rateCode);   // 0..3 => 0.25, 1, 4, 8 Hz

    // For diagnostics
    std::string devicePath() const;

private:
    int     bus_;
    uint8_t addr_;
    int     fd_;

    // Low-level helpers
    void ensureOpen();
    uint16_t readRegister16(uint8_t reg);
    void     writeRegister16(uint8_t reg, uint16_t value);
};
