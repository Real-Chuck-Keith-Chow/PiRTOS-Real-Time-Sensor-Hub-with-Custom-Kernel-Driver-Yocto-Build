#pragma once
#include <functional>
#include <string>

struct TaskSpec {
  std::string name;     // e.g., "sensor"
  int hz;               // loop rate
  int priority;         // SCHED_FIFO 1..99 (higher = more urgent)
  std::function<void()> fn;
};
