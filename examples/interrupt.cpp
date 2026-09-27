/**
 * @file example interrupt.cpp
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

#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrap.hpp"

gpiodWrap gpio(14);

constexpr int interrupt_pin = 17;

int event_count = 0;
bool isevent = false;


int main() {
    using namespace gpiodwrap;

    gpio.bindInterrupt(interrupt_pin, PULLUP, BOTH, 100);

    std::cout << "Interrupts are active...\n";
    
    gpio.watchInterrupt(interrupt_pin, [&](int pin) { 
        std::cout << event_count <<" event of pin: " << pin << "\n";
        isevent = true;
    });

    while (true) {

        if (event_count >= 20) {
            std::cout << "Interrupts test end...\n";
            break;
        }

        if (isevent) {
            event_count++;
            isevent = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gpio.unbindInterrupt(interrupt_pin);
    gpio.resetPin(interrupt_pin);
    
    return 0;
}
