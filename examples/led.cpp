/**
 * @file example led.cpp
 * @class gpiodWrap.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay Donau
 * @date 26.09.2026
 * @license MIT
 *
 * Requires:
 *  - libgpiod (version 2.x recommended)
 *
 * GitHub: https://github.com/dsmurph/gpiodWrap
 */

#include <chrono>
#include <thread>

#include "gpiodWrap.hpp"

gpiodWrap gpio;

constexpr int led = 27;


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(led, OUTPUT); // configure Pin
    
    while (true) {
        gpio.setPin(led, HIGH);         // Set Pin HIGH
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        gpio.setPin(led, LOW);          // Set Pin LOW
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
