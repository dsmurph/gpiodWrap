/**
 * @file combi.cpp
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
#include <thread>
#include <chrono>

#include "gpiodWrap.hpp"

gpiodWrap gpio(14);

int main() {
    using namespace gpiowrap;
    
    gpio.configurePin(17, OUTPUT);
    gpio.configurePin(18, INPUT);

    gpio.attachInterrupt(18, RISING, []() {
        std::cout << "Button pressed!\n";
    });

    gpio.blinkPin(17, 500, 10);
    gpio.pwmPin(17, 50, 2);
    gpio.detachPin(17, HIGH, LOW, 100);

    std::this_thread::sleep_for(std::chrono::seconds(10));


    gpio.detachInterrupt(18);
    gpio.resetPin(17);
    gpio.resetPin(18);

    return 0;
}
