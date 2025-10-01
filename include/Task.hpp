#pragma once
#include <chrono>
#include <string>

struct Task {
  virtual ~Task() = default;
  virtual const char* name() const = 0;
  virtual std::chrono::milliseconds period() const = 0;
  virtual void run() = 0;
};

