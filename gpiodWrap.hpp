/**
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

#pragma once

#include <gpiod.h>
#include <iostream>
#include <map>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <mutex>

template<typename>
inline constexpr bool always_false = false;

class gpiodWrap {
public:

    enum class Direction { Input, Output, Pullup, Pulldown };
    enum class PinValue  { LOW = 0, HIGH = 1 };
    enum class Edge      { RISING, FALLING, BOTH };

    explicit gpiodWrap(int num) {
        std::string path = "/dev/gpiochip" + std::to_string(num);
        chip = gpiod_chip_open(path.c_str());
        if (!chip)
            throw std::runtime_error("Could not open " + path);
    }

    ~gpiodWrap() {
        stopAllThreads();
        std::lock_guard<std::mutex> lock(mtx);
        for (auto &p : line) {
            if (p.second) gpiod_line_request_release(p.second);
        }
        line.clear();

        if (chip)
            gpiod_chip_close(chip);
    }

    // ----------------- Basic functions -----------------
    void configurePin(unsigned int pin, Direction dir, Edge edge = Edge::BOTH) {
        std::lock_guard<std::mutex> lock(mtx);
        if (line.count(pin)) return;
        if (!chip) throw std::runtime_error("No chip opened!");

        gpiod_line_settings *settings = gpiod_line_settings_new();
        gpiod_line_config *lcfg = gpiod_line_config_new();
        unsigned int offset = pin;

        switch (dir) {
            case Direction::Output:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
                break;

            case Direction::Input:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
                break;

            case Direction::Pullup:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
                break;

            case Direction::Pulldown:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
                break;

            default:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
                break;      
        }

        if (dir != Direction::Output) {
            if (edge == Edge::RISING)
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_RISING);
            else if (edge == Edge::FALLING)
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_FALLING);
            else
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
        }

        gpiod_line_config_add_line_settings(lcfg, &offset, 1, settings);

        gpiod_request_config *rcfg = gpiod_request_config_new();
        gpiod_request_config_set_consumer(rcfg, "gpiodWrapper");

        gpiod_line_request *req = gpiod_chip_request_lines(chip, rcfg, lcfg);

        gpiod_line_config_free(lcfg);
        gpiod_line_settings_free(settings);
        gpiod_request_config_free(rcfg);

        if (!req) {
            throw std::runtime_error("Pin " + std::to_string(pin) + " could not be requested");
        }

        line[pin] = req;
    }

    void setPin(unsigned int pin, PinValue value) {
        std::lock_guard<std::mutex> lock(mtx);
        checkPinNoLock(pin);
        gpiod_line_request_set_value(line[pin], pin, static_cast<gpiod_line_value>(value));
    }

    PinValue getPin(unsigned int pin) {
        std::lock_guard<std::mutex> lock(mtx);
        checkPinNoLock(pin);
        int val = gpiod_line_request_get_value(line[pin], pin);
        return val ? PinValue::HIGH : PinValue::LOW;
    }

    void resetPin(unsigned int pin) {
        stopPinThread(pin);
        std::lock_guard<std::mutex> lock(mtx);
        if (line.count(pin)) {
            gpiod_line_request_release(line[pin]);
            line.erase(pin);
        }
    }

    bool debouncePin(unsigned int id, unsigned long debounce_ms) {
        std::lock_guard<std::mutex> lock(mtx);
        unsigned long now = now_ms();
        auto &last_time = debounce_times[id];
        
        if (last_time == 0 || (now - last_time >= debounce_ms)) {
            last_time = now;
            return true;
        }
        return false;
    }

    template <typename Func>
    void attachInterrupt(int pin, Edge edge, Func userCallback) {
        if (!chip) return;

        stopPinThread(pin);

        gpiod_line_request* req = nullptr;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (line.find(pin) == line.end()) {
                std::cerr << "Pin " << pin << " is not configured!\n";
                return;
            }
            req = line[pin];
        }

        if (!req) return;

        std::function<void(int)> callback;
        if constexpr (std::is_invocable_v<Func, int>) {
            callback = userCallback;
        } else if constexpr (std::is_invocable_v<Func>) {
            callback = [userCallback](int) { userCallback(); };
        } else {
            static_assert(always_false<Func>, "Callback must be void() or void(int).");
        }

        running[pin] = true;

        threads[pin] = std::thread([this, pin, req, edge, callback]() {
            gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(16);
            if (!buffer) return;

            while (running[pin]) {

                int ret = gpiod_line_request_wait_edge_events(req, 100'000'000ULL); 
                if (ret <= 0) continue;

                int num_events = gpiod_line_request_read_edge_events(req, buffer, 16);
                for (int i = 0; i < num_events; i++) {
                    gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, i);
                    if (!event) continue;

                    auto type = gpiod_edge_event_get_event_type(event);
                    
                    bool trigger = false;
                    if (edge == Edge::BOTH) {
                        trigger = true;
                    } else if (edge == Edge::RISING && type == GPIOD_EDGE_EVENT_RISING_EDGE) {
                        trigger = true;
                    } else if (edge == Edge::FALLING && type == GPIOD_EDGE_EVENT_FALLING_EDGE) {
                        trigger = true;
                    }

                    if (trigger) {
                        callback(pin);
                    }
                }
            }
            gpiod_edge_event_buffer_free(buffer);
        });
    }

    void detachInterrupt(int pin) {
        stopPinThread(pin);
    }

    // ----------------- Comfort features -----------------
    void blinkPin(unsigned int pin, int interval_ms, int times = -1) {
        stopPinThread(pin);
        running[pin] = true;
        std::thread t([this, pin, interval_ms, times]() {
            int count = 0;
            while (running[pin] && (times < 0 || count < times)) {
                setPin(pin, PinValue::HIGH);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                setPin(pin, PinValue::LOW);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                count++;
            }
        });
        threads[pin] = std::move(t);
    }

    void pwmPin(unsigned int pin, int percent, int frequency) {
        stopPinThread(pin);
        running[pin] = true;
        std::thread t([this, pin, percent, frequency]() {
            int period_ms = 1000 / frequency;
            int high_ms = period_ms * percent / 100;
            int low_ms = period_ms - high_ms;
            while (running[pin]) {
                setPin(pin, PinValue::HIGH);
                std::this_thread::sleep_for(std::chrono::milliseconds(high_ms));
                setPin(pin, PinValue::LOW);
                std::this_thread::sleep_for(std::chrono::milliseconds(low_ms));
            }
        });
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
        threads[pin] = std::move(t);
    }

private:
    gpiod_chip *chip = nullptr;
    std::mutex mtx;

    std::map<int, gpiod_line_request*> line;
    std::map<int, std::thread> threads;
    std::map<int, std::atomic<bool>> running;
    std::map<int, unsigned long> debounce_times;

    unsigned long now_ms() {
        using namespace std::chrono;
        static const auto start = steady_clock::now();
        return duration_cast<milliseconds>(steady_clock::now() - start).count();
    }

    void checkPinNoLock(unsigned int pin) {
        if (!line.count(pin))
            throw std::runtime_error("Pin " + std::to_string(pin) + " not configured");
    }

    void stopPinThread(unsigned int pin) {
        std::thread t;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (running.count(pin)) {
                running[pin] = false;
            }
            if (threads.count(pin)) {
                t = std::move(threads[pin]);
                threads.erase(pin);
            }
            running.erase(pin);
        }

        if (t.joinable()) {
            if (t.get_id() == std::this_thread::get_id()) {
                t.detach();
            } else {
                t.join();
            }
        }
    }

    void stopAllThreads() {
        std::map<int, std::thread> threads_to_join;
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto &p : running) p.second = false;
            threads_to_join = std::move(threads);
            running.clear();
        }

        for (auto &p : threads_to_join) {
            if (p.second.joinable()) {
                p.second.join();
            }
        }
    }
};

namespace gpiowrap {
    constexpr auto INPUT   = gpiodWrap::Direction::Input;
    constexpr auto OUTPUT  = gpiodWrap::Direction::Output;
    constexpr auto PULLUP  = gpiodWrap::Direction::Pullup;
    constexpr auto PULLDN  = gpiodWrap::Direction::Pulldown;

    constexpr auto HIGH    = gpiodWrap::PinValue::HIGH;
    constexpr auto LOW     = gpiodWrap::PinValue::LOW;

    constexpr auto RISING  = gpiodWrap::Edge::RISING;
    constexpr auto FALLING = gpiodWrap::Edge::FALLING;
    constexpr auto BOTH    = gpiodWrap::Edge::BOTH;
}
