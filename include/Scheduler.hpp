#pragma once
#include "Task.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

class Scheduler {
public:
  Scheduler();
  ~Scheduler();
  void add(Task* t);                 // non-owning
  void start();
  void stop();
  bool running() const { return running_; }

private:
  std::atomic<bool> running_{false};
  std::mutex m_;
  std::condition_variable cv_;
  std::vector<std::thread> workers_;
  std::vector<Task*> tasks_;
};
