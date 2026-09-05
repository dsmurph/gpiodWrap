/**
 * @file interrupt.cpp
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

#include "gpiodWrap.hpp"

gpiodWrap gpio(0);

using namespace gpiowrap;

int main() {

    gpio.configurePin(18, INPUT);
    gpio.attachInterrupt(18, RISING, []() {
        std::cout << "Button pressed!\n";
    });

    gpio.attachInterrupt(18, FALLING, [](int pin) {
        std::cout << "Pin " << pin << " has triggered a FALLING event!\n";
    });

    std::cout << "Interrupts are active. Wait 15 seconds....\n";
    std::this_thread::sleep_for(std::chrono::seconds(15));

    std::cout << "End interrupts...\n";
    gpio.detachInterrupt(18);

    return 0;
}
