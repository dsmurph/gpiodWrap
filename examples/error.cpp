/**
 * @file example error.cpp
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

gpiodWrap gpio(15);

int gpioPin = 56;

bool handle_err = true;


int main() {
    using namespace gpiodwrap;

    gpio.configurePin(gpioPin, INPUT);

    while (true) {
        gpio.setPin(gpioPin, HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        gpio.setPin(gpioPin, LOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (handle_err) {

            bool isglobal_err = gpio.anyGlobalError();
            if (isglobal_err) {
               std::cout << "An global error has occurred!" << std::endl;

               std::string entire = gpio.getErrorStr();
               if (!entire.empty()) std::cout << "Any global error: " << entire << std::endl;

               int error_num = gpio.getGlobalErrorNum();
               if (error_num > 0) std::cout << "Global error number: " << error_num << std::endl;

               std::string err_with_num = gpio.getErrorStr(error_num);
               if (!err_with_num.empty()) std::cout << "Global error find with number: " << err_with_num << std::endl;

               std::cout << "Delete errors!" << std::endl;
               gpio.clearGlobalErrors();
               std::cout << "Open valid chip!" << std::endl;
               gpio.openChip(0);
               gpio.configurePin(gpioPin, OUTPUT);
               std::cout << "\n";
               continue;
            }

           bool ispin_err = gpio.anyPinError(gpioPin);
           if (ispin_err) {
               std::cout << "An pin error has occurred!" << std::endl;

               int pin_errnum = gpio.getPinErrorNum(gpioPin);
               if (pin_errnum > 0) std::cout << "Pin error number: " << pin_errnum << std::endl;

               std::string pin_err_with_num = gpio.getErrorStr(pin_errnum, gpioPin);
               if (!pin_err_with_num.empty()) std::cout << "Pin error find with number: " << pin_err_with_num << std::endl;

               std::cout << "Delete pin errors!" << std::endl;
               gpio.clearPinErrors(gpioPin);
               gpio.resetPin(gpioPin);

               std::cout << "Configure pin new!" << std::endl;
               gpioPin = 27;
               gpio.configurePin(gpioPin, OUTPUT);
               std::cout << "\n";
               continue;
            }
        }
    }
}
