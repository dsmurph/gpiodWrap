/**
 * gpiodWrap is part of Raspino Project
 *
 * @file example button.cpp
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

gpiodWrap gpio;

constexpr int button_1 = 27;
constexpr int button_2 = 22;


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(button_1, PULLUP);
    gpio.configurePin(button_2, PULLUP);

    while (true) {

         // without debouncing
        if (gpio.getPin(button_1) == LOW) {
            std::cout << "Button1 pressed\n";
        }

         // with debouncing
        if (gpio.getPin(button_2, 100) == LOW) {
            std::cout << "Button2 debounced pressed\n";
        }

	    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  
}
