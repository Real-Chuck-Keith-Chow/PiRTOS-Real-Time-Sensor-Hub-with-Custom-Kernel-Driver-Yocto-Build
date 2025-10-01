#pragma once
#include <cstdint>
#include <string>
#include <optional>

class Tmp102Sensor {
public:
  // bus like "/dev/i2c-1", default TMP102 addr 0x48
  Tmp102Sensor(std::string bus = "/dev/i2c-1", uint8_t addr = 0x48);
  bool begin();
  std::optional<float> readCelsius();

private:
  std::string bus_;
  uint8_t addr_;
  int fd_ = -1;
  bool openBus();
  bool setAddr();
};
