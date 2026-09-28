/**
 * gpiodWrap is part of Raspino Project
 *
 * @file example button_event.cpp
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

constexpr int button = 17;

int main() {

    using namespace gpiodwrap;

    gpio.configurePin(button, PULLUP);

    while (true) {

        auto event = gpio.getPinEvent(button);

       if (!event)
           std::cout << "ERROR\n";
       else if (*event == IS_RISING)
           std::cout << "RISING\n";
       else if (*event == IS_FALLING)
           std::cout << "FALLING\n";
//     else if (*event == NO_EVENT)
//        std::cout << "NONE\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}