#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include "Task.hpp"

class Scheduler {
  std::vector<std::jthread> threads_;
  std::atomic<bool> running_{false};
public:
  void start(const std::vector<TaskSpec>& tasks);
  void stop();
  ~Scheduler() { stop(); }
};
