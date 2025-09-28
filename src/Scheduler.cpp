#include "Scheduler.hpp"
#include "RtUtils.hpp"

void Scheduler::start(const std::vector<TaskSpec>& tasks) {
  if (running_.exchange(true)) return;
  rt::lock_memory();
  threads_.clear();
  for (auto& t : tasks) {
    threads_.emplace_back([t](std::stop_token st){
      pthread_setname_np(pthread_self(), t.name.c_str());
      try { rt::set_realtime(pthread_self(), t.priority); } catch (...) { /* run best-effort */ }
      const long period_ns = 1'000'000'000LL / (t.hz > 0 ? t.hz : 1);
      timespec next = rt::now();
      while (!st.stop_requested()) {
        t.fn();
        rt::sleep_until(next, period_ns);
      }
    });
  }
}

void Scheduler::stop() {
  if (!running_.exchange(false)) return;
  for (auto& th : threads_) if (th.joinable()) th.request_stop();
  for (auto& th : threads_) if (th.joinable()) th.join();
  threads_.clear();
}
