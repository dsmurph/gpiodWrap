/**
 * gpiodWrap is part of Raspino Project
 *
 * @file example interrupt.cpp
 * @class gpiodWrap.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay Donau
 * @date 01.10.2026
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

gpiodWrap gpio;

constexpr int interrupt_pin = 27;

int event_count = 1;


int main() {
    using namespace gpiodwrap;

    gpio.bindInterrupt(interrupt_pin, PULLUP, RISING, 80);

    std::cout << "Interrupts are active...\n";
    
    gpio.watchInterrupt(interrupt_pin, [&](int pin) { 
        std::cout << event_count <<" event of pin: " << pin << "\n";
        event_count++;
    });

    while (true) {

        if (event_count > 20) {
            std::cout << "Interrupts test end...\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    gpio.unbindInterrupt(interrupt_pin);
    gpio.resetPin(interrupt_pin);

    return 0;
}
