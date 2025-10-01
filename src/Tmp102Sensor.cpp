#include "Tmp102Sensor.hpp"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

Tmp102Sensor::Tmp102Sensor(std::string bus, uint8_t addr)
  : bus_(std::move(bus)), addr_(addr) {}

bool Tmp102Sensor::openBus() {
  fd_ = ::open(bus_.c_str(), O_RDWR);
  return fd_ >= 0;
}
bool Tmp102Sensor::setAddr() {
  return ::ioctl(fd_, I2C_SLAVE, addr_) >= 0;
}
bool Tmp102Sensor::begin() {
  return openBus() && setAddr();
}

std::optional<float> Tmp102Sensor::readCelsius() {
  if (fd_ < 0 && !begin()) return std::nullopt;

  // TMP102 temp register 0x00 (2 bytes)
  uint8_t reg = 0x00;
  if (::write(fd_, &reg, 1) != 1) return std::nullopt;
  uint8_t data[2]{};
  if (::read(fd_, data, 2) != 2) return std::nullopt;

  // 12-bit temperature: (data[0]<<4) | (data[1]>>4)
  int16_t raw = ((data[0] << 8) | data[1]) >> 4;
  // sign extend if negative
  if (raw & 0x800) raw |= 0xF000;
  float c = raw * 0.0625f;
  return c;
}
