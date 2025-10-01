#include "Scheduler.hpp"
#include <chrono>

Scheduler::Scheduler() = default;
Scheduler::~Scheduler() { stop(); }

void Scheduler::add(Task* t) {
  std::lock_guard<std::mutex> lk(m_);
  tasks_.push_back(t);
}

void Scheduler::start() {
  if (running_.exchange(true)) return;

  std::lock_guard<std::mutex> lk(m_);
  workers_.clear();
  workers_.reserve(tasks_.size());

  for (Task* t : tasks_) {
    workers_.emplace_back([this, t] {
      auto next = std::chrono::steady_clock::now();
      while (running_) {
        next += t->period();
        t->run();
        std::unique_lock<std::mutex> ul(m_);
        cv_.wait_until(ul, next, [this]{ return !running_; });
      }
    });
  }
}

void Scheduler::stop() {
  if (!running_.exchange(false)) return;
  cv_.notify_all();
  for (auto& th : workers_) if (th.joinable()) th.join();
  workers_.clear();
}
