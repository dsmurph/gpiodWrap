/**
 * @file example mockup.cpp
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

constexpr int test_pin = 17;

int max_events = 40;
int event_count = 1;
bool isevent = false;

int main() {
    using namespace gpiodwrap;

    gpio.bindInterrupt(test_pin, PULLUP, FALLING, 60);

    std::cout << "Interrupt are active, start test...\n";
    
    gpio.watchInterrupt(test_pin, [&](int pin) { 
        std::cout << event_count <<" eventnum of pin: " << pin << "\n";
        isevent = true;
    });

    while (true) {

        if (event_count >= max_events) {
            std::cout << "Interrupt test end...\n";
            break;
        }

        if (isevent) {
            event_count++;
            isevent = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gpio.unbindInterrupt(test_pin);
    gpio.resetPin(test_pin);
    
    return 0;
}
