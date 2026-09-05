/**
 * @file highlow.cpp
 * @class gpiodWrap.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay Donau
 * @version 1.0.0
 * @date 04.09.2026
 * @license MIT
 *
 * Requires:
 *  - libgpiod (version 2.x recommended)
 *
 * GitHub: https://github.com/dsmurph/gpiodWrap
 */


#include "gpiodWrap.hpp"

gpiodWrap gpio(0);

int main() {
    using namespace gpiowrap;
 
    gpio.configurePin(17, OUTPUT); // configure Pin 
    gpio.setPin(17, HIGH);         // Set Pin HIGH
    gpio.setPin(17, LOW);          // Set Pin LOW
     
    return 0:
}
