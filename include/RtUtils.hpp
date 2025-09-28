#pragma once
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <stdexcept>

namespace rt {
inline void lock_memory() {
  if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
    throw std::runtime_error("mlockall failed (need CAP_IPC_LOCK or sudo)");
}
inline void set_realtime(pthread_t th, int prio) {
  sched_param sp{}; sp.sched_priority = prio;
  if (pthread_setschedparam(th, SCHED_FIFO, &sp) != 0)
    throw std::runtime_error("setschedparam SCHED_FIFO failed (need CAP_SYS_NICE)");
}
inline timespec now() { timespec t{}; clock_gettime(CLOCK_MONOTONIC, &t); return t; }
inline void sleep_until(timespec& next, long period_ns) {
  next.tv_nsec += period_ns;
  while (next.tv_nsec >= 1'000'000'000) { next.tv_nsec -= 1'000'000'000; next.tv_sec++; }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
}
} // namespace rt
