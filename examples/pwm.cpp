/**
 * gpiodWrap is part of Raspino Project
 *
 * @file pwm.cpp
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


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(ledPin, OUTPUT);
    gpio.softPwm(ledPin, 50, 200); //50% illumination, LEDs at 200 Hz – 800 Hz (Caution: Higher refresh rate means higher cpu load.)

    std::this_thread::sleep_for(std::chrono::seconds(10));

    gpio.resetPin(ledPin);

    return 0;
}
