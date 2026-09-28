/**
 * gpiodWrap is part of Raspino Project
 *
 * @file buzzer.cpp
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

#include <thread>
#include <chrono>

#include "gpiodWrap.hpp"


gpiodWrap gpio;

constexpr int buzzer = 17;


bool change = false;
unsigned long alert_interval = 1000;
unsigned long last_time = 0;


unsigned long now_ms() {
    using namespace std::chrono;
    static const auto start = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
}


bool timer(unsigned long interval) {
    unsigned long now = now_ms();

    if (last_time == 0) last_time = now;

    if (now - last_time >= interval) {
        last_time = 0;
        return true;
    }
    return false;
}


void alert() {

    if(change) {
        gpio.softPwm(buzzer, 5, 2000);
        change = false;
    }
    else {
        gpio.softPwm(buzzer, 0, 2000);
        change = true;
    }
}



int main() {

    gpio.configurePin(buzzer, gpiodwrap::OUTPUT);
    
    while (true) {

        if (timer(alert_interval)) alert();

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
 
    }
}
