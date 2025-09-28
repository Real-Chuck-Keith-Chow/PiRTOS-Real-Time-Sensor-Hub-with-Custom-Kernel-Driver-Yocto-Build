#include <iostream>
#include "Scheduler.hpp"

int main() {
  Scheduler sch;
  sch.start({
    {"heartbeat", 5, 10, []{ std::cout << "." << std::flush; }},
    {"tick",      1,  5,  []{ std::cout << " tick\\n"; }}
  });
  std::this_thread::sleep_for(std::chrono::seconds(5));
  sch.stop();
  return 0;
}
