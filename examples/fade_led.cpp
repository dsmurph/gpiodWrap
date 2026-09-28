/**
 * gpiodWrap is part of Raspino Project
 *
 * @file fade_led.cpp
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
#include <iostream>
#include <chrono>

#include "gpiodWrap.hpp"

gpiodWrap gpio;

constexpr int ledPin = 27;

int fade_min = 2;
int fade_max = 100;

int fade = fade_min;
bool change = true;


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(ledPin, OUTPUT);
    
    
    while (true) {

        if (change && fade <= fade_max) {
            fade++;
        }
        else if (fade >= fade_min) {
            fade--;
        }

        gpio.softPwm(ledPin, fade, 200); //LEDs at 200 Hz - 800 Hz (Caution: Higher refresh rate means higher cpu load.)

        if (fade >= fade_max) change = false;
        else if (fade <= fade_min) change = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}
