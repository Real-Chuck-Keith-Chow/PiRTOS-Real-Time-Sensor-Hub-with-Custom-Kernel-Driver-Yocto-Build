#include "Tmp102Sensor.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <cstring>
#include <chrono>
#include <thread>

Tmp102Sensor::Tmp102Sensor(int bus, uint8_t addr)
    : bus_(bus), addr_(addr), fd_(-1) {}

Tmp102Sensor::~Tmp102Sensor() {
    if (fd_ >= 0) ::close(fd_);
}

Tmp102Sensor::Tmp102Sensor(Tmp102Sensor&& rhs) noexcept
    : bus_(rhs.bus_), addr_(rhs.addr_), fd_(rhs.fd_) {
    rhs.fd_ = -1;
}
Tmp102Sensor& Tmp102Sensor::operator=(Tmp102Sensor&& rhs) noexcept {
    if (this != &rhs) {
        if (fd_ >= 0) ::close(fd_);
        bus_  = rhs.bus_;
        addr_ = rhs.addr_;
        fd_   = rhs.fd_;
        rhs.fd_ = -1;
    }
    return *this;
}

std::string Tmp102Sensor::devicePath() const {
    return "/dev/i2c-" + std::to_string(bus_);
}

void Tmp102Sensor::open() {
    if (fd_ >= 0) return; // already open
    const std::string path = devicePath();
    fd_ = ::open(path.c_str(), O_RDWR);
    if (fd_ < 0) {
        throw std::runtime_error("Tmp102Sensor: failed to open " + path + ": " + std::strerror(errno));
    }
    if (ioctl(fd_, I2C_SLAVE, addr_) < 0) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("Tmp102Sensor: I2C_SLAVE ioctl failed: " + std::string(std::strerror(errno)));
    }
}

void Tmp102Sensor::ensureOpen() {
    if (fd_ < 0) open();
}

uint16_t Tmp102Sensor::readRegister16(uint8_t reg) {
    ensureOpen();
    // Write register pointer
    if (::write(fd_, &reg, 1) != 1) {
        throw std::runtime_error("Tmp102Sensor: failed to select register");
    }
    // Read 2 bytes MSB..LSB
    uint8_t buf[2];
    if (::read(fd_, buf, 2) != 2) {
        throw std::runtime_error("Tmp102Sensor: failed to read register");
    }
    return static_cast<uint16_t>((buf[0] << 8) | buf[1]);
}

void Tmp102Sensor::writeRegister16(uint8_t reg, uint16_t value) {
    ensureOpen();
    uint8_t buf[3] = { reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    if (::write(fd_, buf, 3) != 3) {
        throw std::runtime_error("Tmp102Sensor: failed to write register");
    }
}

double Tmp102Sensor::readCelsius() {
    // TMP102 temperature register = 0x00, 12-bit two's complement at bits [15:4]
    constexpr uint8_t TEMP_REG = 0x00;
    uint16_t raw = readRegister16(TEMP_REG);

    // Align to 12-bit value: MSB:bits15..8, LSB:bits7..0; temperature is [15:4]
    int16_t temp12 = static_cast<int16_t>(raw >> 4);

    // Sign-extend from 12-bit two's complement
    if (temp12 & 0x0800) { // if sign bit (bit 11) set
        temp12 |= 0xF000;
    }
    // Each LSB = 0.0625°C
    return static_cast<double>(temp12) * 0.0625;
}

void Tmp102Sensor::setShutdown(bool enable) {
    constexpr uint8_t CONF_REG = 0x01;
    uint16_t conf = readRegister16(CONF_REG);
    if (enable) conf |=  (1u << 8);  // SD bit
    else        conf &= ~(1u << 8);
    writeRegister16(CONF_REG, conf);
}

void Tmp102Sensor::setConversionRate(uint8_t rateCode) {
    // rateCode 0..3 maps to 0.25, 1, 4, 8 Hz at CR1..CR0 (bits 7..6) of config register LSB
    constexpr uint8_t CONF_REG = 0x01;
    uint16_t conf = readRegister16(CONF_REG);
    conf &= ~(0b11u << 6);                 // clear CR bits
    conf |= (static_cast<uint16_t>(rateCode & 0x3) << 6);
    writeRegister16(CONF_REG, conf);
}
