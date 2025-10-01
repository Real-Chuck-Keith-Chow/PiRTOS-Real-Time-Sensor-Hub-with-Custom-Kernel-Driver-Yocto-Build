#pragma once
#include <chrono>
#include <iostream>

inline void pinThreadNice(int nice = -10) {
  // portable "hint"; on Linux you could set sched params or nice()
  (void)nice;
}

inline void log_ts(const char* tag, const char* msg) {
  using namespace std::chrono;
  auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
  std::cout << "[" << tag << " @" << now << "ms] " << msg << std::endl;
}

