/**
 * @file gpiodWrapper.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay (dsmurph)
 * @version 0.1.0
 * @date 2025-11-23
 * @license MIT
 *
 * Requires:
 *  - libgpiod (version 2.x recommended)
 *
 * GitHub: https://github.com/dsmurph/gpiodWrapper
 */

#pragma once

#include <gpiod.h>
#include <iostream>
#include <map>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

enum Direction { Input, Output, Pullup, Pulldown };
enum PinValue { LOW = 0, HIGH = 1 };
enum Edge { RISING, FALLING, BOTH };

template<typename>
inline constexpr bool always_false = false;

class gpiodWrapper {
public:
    gpiodWrapper(int num) {
        std::string path = "/dev/gpiochip" + std::to_string(num);
        chip = gpiod_chip_open(path.c_str());
        if (!chip)
            throw std::runtime_error("Could not open " + path);
    }

    ~gpiodWrapper() {
        stopAllThreads();
        for (auto &p : line)
            gpiod_line_request_release(p.second);
        line.clear();

        if (chip)
            gpiod_chip_close(chip);
    }

    // ----------------- Basic functions -----------------
   void configurePin(unsigned int pin, Direction dir) {
       if (line.count(pin)) return;
       if (!chip) throw std::runtime_error("No chip opened!");

       gpiod_line_settings *settings = gpiod_line_settings_new();
       gpiod_line_config *lcfg = gpiod_line_config_new();
       unsigned int offset = pin;

       // Direction & Pull Settings
       switch (dir) {
           case Output:
               gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
               break;

           case Input:
               gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
               gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
               break;

           case Pullup:
               gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
               gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
               break;

           case Pulldown:
               gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
               gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
               break;

           default:
               std::cerr << "Invalid direction value for PIN "
               << pin << " – focus on secure input (High-Impedance)\n";
               gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
               gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
               break;     
       }

       gpiod_line_config_add_line_settings(lcfg, &offset, 1, settings);

       gpiod_request_config *rcfg = gpiod_request_config_new();
       gpiod_request_config_set_consumer(rcfg, "gpiodWrapper");

       gpiod_line_request *req = gpiod_chip_request_lines(chip, rcfg, lcfg);
       if (!req) {
           gpiod_line_config_free(lcfg);
           gpiod_line_settings_free(settings);
           gpiod_request_config_free(rcfg);
           throw std::runtime_error("Pin " + std::to_string(pin) + " could not be requested");
       }

       gpiod_line_config_free(lcfg);
       gpiod_line_settings_free(settings);
       gpiod_request_config_free(rcfg);

       line[pin] = req;
    }


    void setPin(unsigned int pin, PinValue value) {
        checkPin(pin);
        gpiod_line_request_set_value(line[pin], pin, static_cast<gpiod_line_value>(value));
    }

    PinValue getPin(unsigned int pin) {
        checkPin(pin);
        int val = gpiod_line_request_get_value(line[pin], pin);
        return val ? HIGH : LOW;
    }

    void resetPin(unsigned int pin) {
        stopPinThread(pin);
        if (line.count(pin)) {
            gpiod_line_request_release(line[pin]);
            line.erase(pin);
        }
    }

    // ----------------- Comfort features -----------------
    void blinkPin(unsigned int pin, int interval_ms, int times = -1) {
        stopPinThread(pin);
        running[pin] = true;
        std::thread t([this, pin, interval_ms, times]() {
            int count = 0;
            while (running[pin] && (times < 0 || count < times)) {
                setPin(pin, HIGH);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                setPin(pin, LOW);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                count++;
            }
        });
        t.detach();
        threads[pin] = std::move(t);
    }

    void pwmPin(unsigned int pin, int dutyPercent, int frequency) {
        stopPinThread(pin);
        running[pin] = true;
        std::thread t([this, pin, dutyPercent, frequency]() {
            int period_ms = 1000 / frequency;
            int high_ms = period_ms * dutyPercent / 100;
            int low_ms = period_ms - high_ms;
            while (running[pin]) {
                setPin(pin, HIGH);
                std::this_thread::sleep_for(std::chrono::milliseconds(high_ms));
                setPin(pin, LOW);
                std::this_thread::sleep_for(std::chrono::milliseconds(low_ms));
            }
        });
        t.detach();
        threads[pin] = std::move(t);
    }

    void detachPin(unsigned int pin, PinValue value1, PinValue value2, int interval_ms) {
        stopPinThread(pin);
        running[pin] = true;
        std::thread t([this, pin, value1, value2, interval_ms]() {
            while (running[pin]) {
                setPin(pin, value1);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                setPin(pin, value2);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
            }
        });
        t.detach();
        threads[pin] = std::move(t);
    }


// Interrupt handling (Edge Events) – libgpiod 2.x compatible
template <typename Func>
   void attachInterrupt(int pin, Edge edge, Func userCallback) {
      if (!chip) return;

      if (line.find(pin) == line.end()) {
         std::cerr << "Pin " << pin << " is not configured!\n";
         return;
      }

      auto *req = line[pin];
      if (!req) {
         std::cerr << "No active line request for Pin " << pin << "\n";
         return;
      }

      // Automatically convert lambda to std::function<void(int)>
      std::function<void(int)> callback;
      if constexpr (std::is_invocable_v<Func, int>) {
         callback = userCallback; // That's fine.
      } 
      else if constexpr (std::is_invocable_v<Func>) {
         callback = [userCallback](int) { userCallback(); }; // without parameters
      } 
      else {
         static_assert(always_false<Func>, "Callback must be void() or void(int).");
      }

      running[pin] = true;

      threads[pin] = std::thread([=]() mutable {
         while (running.at(pin)) {
             gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
             if (!buffer) {
                 std::cerr << "Error creating event buffer\n";
                 break;
             }

             int ret = gpiod_line_request_wait_edge_events(req, 1'000'000'000);
             if (ret <= 0) {
                 gpiod_edge_event_buffer_free(buffer);
                 continue;
             }

             ret = gpiod_line_request_read_edge_events(req, buffer, 1);
             if (ret <= 0) {
                 gpiod_edge_event_buffer_free(buffer);
                 continue;
             }

             gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, 0);
             if (!event) {
                 gpiod_edge_event_buffer_free(buffer);
                 continue;
             }

             auto type = gpiod_edge_event_get_event_type(event);

             if ((edge == RISING && type == GPIOD_EDGE_EVENT_RISING_EDGE) ||
                 (edge == FALLING && type == GPIOD_EDGE_EVENT_FALLING_EDGE) ||
                 (edge == BOTH)) {
                 callback(pin);
             }

             gpiod_edge_event_buffer_free(buffer);
         }
    });
}


void detachInterrupt(int pin) {
    if (threads.find(pin) != threads.end()) {
        running[pin] = false;
        if (threads[pin].joinable())
            threads[pin].join();
        threads.erase(pin);
        running.erase(pin);
    }
}

private:
    gpiod_chip *chip;
    std::map<int, gpiod_line_request*> line;
    std::map<int, std::thread> threads;
    std::map<int, bool> running;
    
    void checkPin(unsigned int pin) {
        if (!line.count(pin))
            throw std::runtime_error("Pin " + std::to_string(pin) + " not configured");
    }

    void stopPinThread(unsigned int pin) {
        if (running[pin]) {
            running[pin] = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            threads.erase(pin);
        }
    }

    void stopAllThreads() {
        for (auto &p : running) p.second = false;
        threads.clear();
    }
};
