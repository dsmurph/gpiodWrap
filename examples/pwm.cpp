/**
 * @file pwm.cpp
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

#include <thread>
#include <iostream>
#include <chrono>

#include "gpiodWrap.hpp"

gpiodWrap gpio(0);

int main() {
    using namespace gpiowrap;
    
    gpio.configurePin(17, OUTPUT);
    gpio.pwmPin(17, 50, 2);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    gpio.resetPin(17);
  
    return 0;
}
