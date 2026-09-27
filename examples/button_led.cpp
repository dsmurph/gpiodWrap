/**
 * @file example button_led.cpp
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

constexpr int button = 17;
constexpr int led = 27;

bool change = false;

int main() {
    using namespace gpiodwrap;

    gpio.configurePin(button, PULLUP);
    gpio.configurePin(led, OUTPUT);
    
    while(true) {
        const auto event = gpio.getPinEvent(button, 50);

        if (event == IS_RISING) {
            change = false;
        }
        else if (event == IS_FALLING) {
            change = true;
        }

        if (change) {
            gpio.setPin(led, HIGH);
        }
        else {
            gpio.setPin(led, LOW);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
