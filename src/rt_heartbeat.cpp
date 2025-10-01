#include "Task.hpp"
#include "RtUtils.hpp"
#include <atomic>
#include <chrono>

class HeartbeatTask : public Task {
public:
  const char* name() const override { return "heartbeat"; }
  std::chrono::milliseconds period() const override { return std::chrono::milliseconds(500); }
  void run() override {
    state_ = !state_;
    log_ts(name(), state_ ? "tick" : "tock");
    // TODO: toggle a GPIO here if running on hardware (libgpiod recommended)
  }
private:
  bool state_{false};
};
