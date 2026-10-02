/**
 * gpiodWrap is part of Raspino Project
 *
 * @file example mockup.cpp
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

gpiodWrap gpio(14);

constexpr int test_pin = 17;


int event_count = 1;
bool isevent = false;
bool first_event = false;
unsigned long last_time = 0;


unsigned long now_ms() {
    using namespace std::chrono;
    static const auto start = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
}


// Ends the test when no more events occur.
bool watchdog(unsigned long interval) {
    unsigned long now = now_ms();

    if (last_time == 0) last_time = now;

    if (now - last_time >= interval) {
        last_time = 0;
        return true;
    }
    return false;
}


int main() {
    using namespace gpiodwrap;

    gpio.bindInterrupt(test_pin, PULLUP, FALLING, 80);

    std::cout << "Interrupt are active, start test...\n";
    
    gpio.watchInterrupt(test_pin, [&](int pin) { 
        std::cout << event_count <<" eventnum of pin: " << pin << "\n";
        isevent = true;
    });

    while (true) {

        if (first_event) {
            if (watchdog(1000)) {
                std::cout << "Interrupt test end...\n";
                break;
            }
        }

        if (isevent) {
            event_count++;
            isevent = false;
            last_time = 0;
            first_event = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gpio.unbindInterrupt(test_pin);
    gpio.resetPin(test_pin);
    
    return 0;
}
