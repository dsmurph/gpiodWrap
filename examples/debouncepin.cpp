/**
 * @file debouncepin.cpp
 * @class gpiodWrap.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay Donau
 * @version 1.0.0
 * @date 04.09.2026
 * @license MIT
 *
 * Requires:
 *  - libgpiod (version 2.x recommended)
 *
 * GitHub: https://github.com/dsmurph/gpiodWrap
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <map>

#include "gpiodWrap.hpp" 

gpiodWrap gpio(0);

int main() {
  using namespace gpiowrap;
  
  bool button0 = false;
  bool button1 = false;  

  unsigned long count_16 = 1;
  unsigned long count_17 = 1;
  
  gpio.configurePin(16, PULLUP);
  gpio.configurePin(17, PULLUP);

  gpio.attachInterrupt(16, FALLING, [&]() {
      if (! button0) std::cout << "gpio16 first falling" << "\n";
      button0 = true;
      std::cout << "gpio16 falling" << "\n";
    });

  gpio.attachInterrupt(17, FALLING, [&]() {
      if (! button1) std::cout << "gpio17 first falling" << "\n";
      button1 = true;
      std::cout << "gpio17 falling" << "\n";
    });

  std::cout << "Start debouncePin example" << "\n";

  while (true) {
 
    if (button0) {
      if (gpio.debouncePin(20, 16)) {
        std::cout << "gpio16 debounce time " << count_16 << "\n";
        button0 = false;
        count_16 = 0;
      }
      count_16++;
    }

    if (button1) {
      if (gpio.debouncePin(50, 17)) {
        std::cout << "gpio17 debounce time " << count_17 << "\n";
        button1 = false;
        count_17 = 0;
      }
      count_17++;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
