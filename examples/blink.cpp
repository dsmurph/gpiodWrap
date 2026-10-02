/**
 * gpiodWrap is part of Raspino Project
 *
 * @file example blink.cpp
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

constexpr int led = 17;


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(led, OUTPUT);
    gpio.blinkPin(led, 100, 30);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    gpio.resetPin(led);
}
