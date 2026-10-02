/**
 * gpiodWrap is part of Raspino Project
 *
 * @class gpiodWrap.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 * Supports basic operations such as set/get, toggling, and automatic cleanup.
 *
 * @author Kay Donau
 * @version 1.2.2
 * @date 01.10.2026
 * @license MIT
 *
 * Requires:
 *  - libgpiod (version 2.x recommended)
 *
 * GitHub: https://github.com/dsmurph/gpiodWrap
 */

#pragma once


#include <gpiod.h>

#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <bitset>
#include <queue>
#include <thread>

#ifndef GPIODWRAP_NO_SIGNALS
   #include <csignal>
#endif


template<typename>
inline constexpr bool always_false = false;


class gpiodWrap {
public:

    enum class PinDirection {
        INPUT,
        OUTPUT,
        PULLUP,
        PULLDOWN
    };

    enum class PinValue {
        LOW = 0,
        HIGH = 1
    };

    enum class Edge {
        NONE,
        RISING,
        FALLING,
        BOTH
    };

    enum class PinEvent {
        NO_EVENT,
        IS_RISING,
        IS_FALLING
    };

    enum class ErrorRegister : uint16_t {
        NO_ERRORS = 0,
        PERMISSION_DENIED,
        SYSTEM_OUT_OF_MEMORY,
        KERNEL_BUFFER_OVERFLOW,
        HARDWARE_LOST,
        INVALID_ARGUMENT,
        CHIP_NOT_FOUND,
        CHIP_NOT_OPEN,
        NO_CHIP_OPEN,
        PIN_ALREADY_LOCKED,
        PIN_INVALID,
        PIN_READ_FAILED,
        PIN_WRITE_FAILED,
        REQUEST_FAILED
    };


    explicit gpiodWrap(int chipNum = -1) {

#ifndef GPIODWRAP_NO_SIGNALS
        setupSignalHandling();
#endif
        openChip(chipNum);
    }


    ~gpiodWrap() { closeChip(); }



/**************************************************************
*
*       Basic methods
*
**************************************************************/

    bool openChip(int num = -1) {
        if (chip) closeChip();

        std::string path = (num == -1) ? findChip() : "/dev/gpiochip" + std::to_string(num);
        chip = gpiod_chip_open(path.c_str());

        if (!chip) {
            setGlobalError(ErrorRegister::CHIP_NOT_OPEN);
            return false;
        }

        closing.store(false);
        return true;
    }


    void closeChip(bool cleanup = false) {

        bool expected = false;
        if (!closing.compare_exchange_strong(expected, true)) return;

        {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& [pin, state] : pins) {
                if (state.running) state.running->store(false);
                if (state.request) {
                    if (state.direction == PinDirection::OUTPUT) safeStatePin(state.request, pin);
                    gpiod_line_request_release(state.request);
                    state.request = nullptr;
                }
            }
        }

        stopAllThreads();
        stopInterruptWorker();

        {
            std::lock_guard<std::mutex> lock(interruptQueueMtx);
            while (!interruptQueue.empty()) 
                interruptQueue.pop();
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            pins.clear();
        }

        if (chip) {
            gpiod_chip_close(chip);
            chip = nullptr;
        }
    }


    void configurePin(unsigned int pin, PinDirection dir) {
        registerPin(pin, dir, Edge::NONE);
    }


    void setPin(unsigned int pin, PinValue value) {
        std::lock_guard<std::mutex> lock(mtx);

        auto* state = getPinStateLocked(pin);
        if (!state || !state->request) {
            setPinError(pin, ErrorRegister::PIN_INVALID);
            return;
        }

        const int ret = gpiod_line_request_set_value(state->request, pin, static_cast<gpiod_line_value>(value));

        if (ret < 0) setPinError(pin, ErrorRegister::PIN_WRITE_FAILED);
    }


    std::optional<PinValue> getPin(unsigned int pin, unsigned long debounce_ms = 0) {
        std::lock_guard<std::mutex> lock(mtx);

        auto* state = getPinStateLocked(pin);
        if (!state || !state->request) {
            setPinError(pin, ErrorRegister::PIN_INVALID);
            return std::nullopt;
        }

        const auto rawValue = readPinLocked(pin);
        if (!rawValue.has_value()) {
            setPinError(pin, ErrorRegister::PIN_READ_FAILED);
            return std::nullopt;
        }

        if (debounce_ms == 0) {
            auto& debounce = state->pinDebounce;
            debounce.initialized = true;
            debounce.stableValue = rawValue.value();
            debounce.currentValue = rawValue.value();
            debounce.lastEvent = now_ms();
            return rawValue.value() ? PinValue::HIGH : PinValue::LOW;
        }

        return getDebouncedPinLocked(pin, rawValue.value(), debounce_ms) ? PinValue::HIGH : PinValue::LOW;
    }


    std::optional<PinEvent> getPinEvent(unsigned int pin, unsigned long debounce_ms = 20) {
        std::lock_guard<std::mutex> lock(mtx);

        auto* state = getPinStateLocked(pin);

        if (!state || !state->request) {
            setPinError(pin, ErrorRegister::PIN_INVALID);
            return std::nullopt;
        }

        const int value = gpiod_line_request_get_value(state->request, pin);
        if (value < 0) {
            setPinError(pin, ErrorRegister::PIN_READ_FAILED);
            return std::nullopt;
        }

        const bool rawValue = value != 0;
        const auto now = now_ms();

        auto& debounce = state->eventDebounce;

        if (!debounce.initialized) {
            debounce.initialized = true;
            debounce.stableValue = rawValue;
            debounce.currentValue = rawValue;
            debounce.lastEvent = now;
            return PinEvent::NO_EVENT;
        }

        if (rawValue != debounce.currentValue) {
            debounce.currentValue = rawValue;
            debounce.lastEvent = now;
        }

        if (debounce.currentValue == debounce.stableValue) return PinEvent::NO_EVENT;
        if (debounce_ms != 0 && (now - debounce.lastEvent < debounce_ms)) return PinEvent::NO_EVENT;

        const bool oldValue = debounce.stableValue;
        debounce.stableValue = debounce.currentValue;

        if (!oldValue && debounce.stableValue) return PinEvent::IS_RISING;

        return PinEvent::IS_FALLING;
    }


    void resetPin(unsigned int pin) {

        unbindInterrupt(pin);
 
        std::lock_guard<std::mutex> lock(mtx);
        pins.erase(pin);
        clearPinErrors(pin);

    }


    void bindInterrupt(unsigned int pin, PinDirection dir, Edge edge = Edge::BOTH, unsigned long debounce_ms = 30) {
        if (dir == PinDirection::OUTPUT && edge != Edge::NONE) {
            setGlobalError(ErrorRegister::INVALID_ARGUMENT);
            return;
        }
        
        registerPin(pin, dir, edge, debounce_ms);
    }



    template <typename Func>
    void watchInterrupt(unsigned int pin, Func userCallback) {
        if (!chip) {
            setGlobalError(ErrorRegister::CHIP_NOT_OPEN);
            return;
        }

        stopPinThread(pin);

        gpiod_line_request* req = nullptr;
        Edge requestedEdge = Edge::NONE;
        unsigned long debounceMs = 0;

        {
            std::lock_guard<std::mutex> lock(mtx);

            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }

            req = it->second.request;
            requestedEdge = it->second.edge;
            debounceMs = it->second.interruptDebounceMs;
        }

        if (!req) {
            setPinError(pin, ErrorRegister::REQUEST_FAILED);
            return;
        }

        std::function<void(int)> callback;
        if constexpr (std::is_invocable_v<Func, int>) callback = std::function<void(int)>(userCallback);
        else if constexpr (std::is_invocable_v<Func>) callback = [userCallback](int) { userCallback(); };
        else static_assert(always_false<Func>, "Callback must be void() or void(int).");

        startInterruptWorker();
        auto runFlag = std::make_shared<std::atomic_bool>(true);

        bool lastReportedState = false;
        {
            std::lock_guard<std::mutex> lock(mtx);

            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }

            it->second.running = runFlag;

            auto initialLevel = readPinLocked(pin);
            if (!initialLevel.has_value()) {
                setPinError(pin, ErrorRegister::PIN_READ_FAILED);
                runFlag->store(false);
                return;
            }
            lastReportedState = initialLevel.has_value();

            auto& debounce = it->second.interruptDebounce;
            debounce.initialized = true;
            debounce.stableValue = initialLevel.value();
            debounce.currentValue = initialLevel.value();
            debounce.lastEvent = now_ms();
        }

       std::thread t([this, pin, req, requestedEdge, debounceMs, callback, runFlag, lastReportedState]() mutable {
           gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(16);
           if (!buffer) {
                setGlobalError(ErrorRegister::SYSTEM_OUT_OF_MEMORY);
                return;
            }

            uint64_t lastValidTimestampMs = 0;

            while (runFlag->load() && !closing.load()) {
                const int waitRes = gpiod_line_request_wait_edge_events(req, 100'000'000);

                if (waitRes < 0) break;
                if (waitRes == 0) continue;

                const int eventCount = gpiod_line_request_read_edge_events(req, buffer, 16);
                if (eventCount < 0) {
                    if (!runFlag->load() || closing.load()) break;
                    setPinError(pin, ErrorRegister::PIN_READ_FAILED);
                    continue;
                }

                for (int i = 0; i < eventCount; ++i) {
                    if (!runFlag->load() || closing.load()) break;

                    gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(buffer, i);
                    if (!event) continue;

                    const auto eventType = gpiod_edge_event_get_event_type(event);
                    const bool isRising  = (eventType == GPIOD_EDGE_EVENT_RISING_EDGE);
                    const bool isFalling = (eventType == GPIOD_EDGE_EVENT_FALLING_EDGE);

                    if (!isRising && !isFalling) continue;

                    const bool currentLevel = isRising;
                    const uint64_t eventNs  = gpiod_edge_event_get_timestamp_ns(event);
                    const uint64_t eventMs  = eventNs / 1'000'000;

                    if (currentLevel != lastReportedState) {

                        if (debounceMs > 0 && lastValidTimestampMs > 0 && (eventMs - lastValidTimestampMs < debounceMs))
                            continue;

                        lastValidTimestampMs = eventMs;
                        lastReportedState    = currentLevel;

                        bool match = false;
                        if (requestedEdge == Edge::BOTH) match = true;
                        else if (requestedEdge == Edge::RISING && isRising) match = true;
                        else if (requestedEdge == Edge::FALLING && isFalling) match = true;

                        if (match) {
                            {
                                std::lock_guard<std::mutex> lock(interruptQueueMtx);
                                interruptQueue.push(InterruptEvent{ pin, eventMs, callback });
                            }
                            interruptCv.notify_one();
                        }
                    }
                }
            }
            gpiod_edge_event_buffer_free(buffer);
        });

        {
            std::lock_guard<std::mutex> lock(mtx);

            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                runFlag->store(false);
                if (t.joinable()) t.detach();
                return;
            }
            it->second.thread = std::move(t);
        }
    }


    void unbindInterrupt(unsigned int pin) {

        stopPinThread(pin);

        std::lock_guard<std::mutex> lock(mtx);
        auto it = pins.find(pin);
        if (it != pins.end()) {
            if (it->second.request) {
                if (it->second.direction == PinDirection::OUTPUT) safeStatePin(it->second.request, pin);

                gpiod_line_request_release(it->second.request);
                it->second.request = nullptr;
            }
            it->second.running.reset();
        }
    }




/**************************************************************
*
*       Error handling
*
**************************************************************/

    bool hasGlobalError(ErrorRegister err) const {
        std::lock_guard<std::mutex> lock(errorMtx);
        return globalErrors_.test(static_cast<size_t>(err));
    }


    bool anyGlobalError() const {
        std::lock_guard<std::mutex> lock(errorMtx);
        return globalErrors_.any();
    }


    bool hasPinError(unsigned int pin, ErrorRegister err) const {
        std::lock_guard<std::mutex> lock(errorMtx);

        auto it = pinErrors_.find(pin);
        if (it == pinErrors_.end()) return false;

        return it->second.test(static_cast<size_t>(err));
    }


    bool anyPinError(unsigned int pin) const {
        std::lock_guard<std::mutex> lock(errorMtx);

        auto it = pinErrors_.find(pin);
        if (it == pinErrors_.end()) return false;

        return it->second.any();
    }


    int getGlobalErrorNum() {
        std::lock_guard<std::mutex> lock(errorMtx);

        if (globalErrors_.any()) {
            for (int i=1; i<=8; i++)
                if (globalErrors_.test(i)) return i;
        }
        return 0;
    }


    int getPinErrorNum(unsigned int pin) {
        std::lock_guard<std::mutex> lock(errorMtx);

        auto it = pinErrors_.find(pin);
        if (it == pinErrors_.end()) return 0;

        if (it->second.any()) {
            for (int i=9; i<=13; i++)
                if (it->second.test(i)) return i;
        }
        return 0;
    }


    void clearGlobalError(ErrorRegister err) {
        std::lock_guard<std::mutex> lock(errorMtx);
        globalErrors_.reset(static_cast<size_t>(err));
    }


    void clearGlobalErrors() {
        std::lock_guard<std::mutex> lock(errorMtx);
        globalErrors_.reset();
    }


    void clearPinError(unsigned int pin, ErrorRegister err) {
        std::lock_guard<std::mutex> lock(errorMtx);

        auto it = pinErrors_.find(pin);
        if (it == pinErrors_.end()) return;
        it->second.reset(static_cast<size_t>(err));
        if (it->second.none()) pinErrors_.erase(it);
    }


    void clearPinErrors(unsigned int pin) {
        std::lock_guard<std::mutex> lock(errorMtx);

        auto it = pinErrors_.find(pin);
        if (it == pinErrors_.end()) return;
        it->second.reset();
        if (it->second.none()) pinErrors_.erase(it);   
    }




/**************************************************************
*
*       Comfort features
*
**************************************************************/

    std::string getStrErr() const {
        std::lock_guard<std::mutex> lock(errorMtx);

        bool hasGlobalErr = globalErrors_.any();
        bool hasPinErr = false;
        for (const auto& [pin, errBitset] : pinErrors_) {
            if (errBitset.any()) {
                hasPinErr = true;
                break;
            }
        }

        if (!hasGlobalErr && !hasPinErr) return strError(static_cast<uint8_t>(ErrorRegister::NO_ERRORS));

        std::string result;

        if (hasGlobalErr) {
            result += "[GLOBAL ERRORS]:\n";
            for (size_t i = 0; i < globalErrors_.size(); ++i) {
                if (globalErrors_.test(i)) {
                    result += "  - " + strError(static_cast<uint8_t>(i)) + "\n";
                }
            }
        }

        if (hasPinErr) {
            if (!result.empty()) result += "\n";
            result += "[PIN ERRORS]:\n";
        
            for (const auto& [pin, errBitset] : pinErrors_) {
                if (errBitset.none()) continue;

                for (size_t i = 0; i < errBitset.size(); ++i) {
                    if (errBitset.test(i)) result += "  - " + strError(static_cast<uint8_t>(i), pin) + "\n";
                }
            }
        }
        return result;
    }


    std::string getStrErrPin(unsigned int pin) const {
        std::lock_guard<std::mutex> lock(errorMtx);

        std::string result;

        for (size_t i = 0; i < globalErrors_.size(); ++i) {
            if (globalErrors_.test(i)) result += "[Global] " + strError(static_cast<uint8_t>(i)) + "\n";
        }

        auto it = pinErrors_.find(pin);
        if (it != pinErrors_.end()) {
            for (size_t i = 0; i < it->second.size(); ++i) {
                if (it->second.test(i)) {
                    result += "[Pin " + std::to_string(pin) + "] " + strError(static_cast<uint8_t>(i), pin) + "\n";
                }
            }
        }

        if (result.empty()) return strError(static_cast<uint8_t>(ErrorRegister::NO_ERRORS));

        return result;
    }


    void blinkPin(unsigned int pin, unsigned int interval_ms, int times = -1) {
        stopPinThread(pin);

        auto runFlag = std::make_shared<std::atomic_bool>(true);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.running = runFlag;
        }

        std::thread t([this, pin, interval_ms, times, runFlag]() {
            int count = 0;
            while (runFlag->load() && (times < 0 || count < times)) {
                setPin(pin, PinValue::HIGH);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                setPin(pin, PinValue::LOW);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                count++;
            }
        });

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.thread = std::move(t);
        }
    }


    void softPwm(unsigned int pin, int percent, int frequency) {
        if (frequency <= 0) return;

        percent = std::clamp(percent, 0, 100);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            auto& pwm = it->second.pwm;
            const auto period_us = 1'000'000 / frequency;
            pwm.high_us = period_us * percent / 100;
            pwm.low_us = period_us - pwm.high_us;
            if (pwm.initialized) return;
            pwm.initialized = true;
        }

        stopPinThread(pin);

        auto runFlag = std::make_shared<std::atomic_bool>(true);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.running = runFlag;
        }

        std::thread t([this, pin, runFlag]() {
            while (runFlag->load()) {
                PwmState currentState;

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    auto it = pins.find(pin);
                    if (it == pins.end()) break;
                    currentState = it->second.pwm;
                }

                if (currentState.high_us > 0) {
                    setPin(pin, PinValue::HIGH);
                    std::this_thread::sleep_for(std::chrono::microseconds{currentState.high_us});
                }

                if (currentState.low_us > 0) {
                    setPin(pin, PinValue::LOW);
                    std::this_thread::sleep_for(std::chrono::microseconds{currentState.low_us});
                }
            }

            setPin(pin, PinValue::LOW);
        });

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.thread = std::move(t);
        }
    }


    void detachPin(unsigned int pin, PinValue value1, PinValue value2, unsigned int interval_ms) {
        stopPinThread(pin);

        auto runFlag = std::make_shared<std::atomic_bool>(true);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.running = runFlag;
        }

        std::thread t([this, pin, value1, value2, interval_ms, runFlag]() {
            while (runFlag->load()) {
                setPin(pin, value1);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                setPin(pin, value2);
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
            }
        });

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end()) {
                setPinError(pin, ErrorRegister::PIN_INVALID);
                return;
            }
            it->second.thread = std::move(t);
        }
    }


private:

    struct DebounceState {
        bool initialized = false;
        bool stableValue = false;
        bool currentValue = false;
        std::uint64_t lastEvent = 0;
    };


    struct PwmState {
        bool initialized = false;
        int high_us = 0;
        int low_us = 0;
    };


    struct PinState {
        gpiod_line_request* request = nullptr;

        PinDirection direction = PinDirection::INPUT;
        Edge edge = Edge::NONE;

        DebounceState pinDebounce;
        DebounceState eventDebounce;
        DebounceState interruptDebounce;

        unsigned long interruptDebounceMs = 30;

        PwmState pwm;

        std::thread thread;
        std::shared_ptr<std::atomic_bool> running;
    };


    struct InterruptEvent {
        unsigned int pin;
        std::uint64_t timestamp;
        std::function<void(int)> callback;
    };

  
    gpiod_chip* chip = nullptr;

    mutable std::mutex mtx;
    mutable std::mutex errorMtx;

    std::atomic_bool closing{false};

    std::map<unsigned int, PinState> pins;
    std::bitset<16> globalErrors_;
    std::map<unsigned int, std::bitset<16>> pinErrors_;

    inline static std::atomic<int> activeLineReqFds[64] = {
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
    };

    std::mutex interruptQueueMtx;
    std::condition_variable interruptCv;
    std::queue<InterruptEvent> interruptQueue;
    std::atomic_bool interruptWorkerRunning{false};
    std::thread interruptWorker;



    unsigned long now_ms() {
        using namespace std::chrono;
        static const auto start = steady_clock::now();
        return duration_cast<milliseconds>(steady_clock::now() - start).count();
    }


    std::string findChip() {
        const char* targets[] = {
            "pinctrl-rp1",
            "rp1-gpio",
            "pinctrl-bcm2712",
            "pinctrl-bcm2835"
        };

        for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
            auto name = entry.path().filename().string();

            if (name.rfind("gpiochip", 0) != 0) continue;

            struct gpiod_chip* chip = gpiod_chip_open(entry.path().c_str());
            if (!chip) continue;

            struct gpiod_chip_info* info = gpiod_chip_get_info(chip);
            if (info) {
                const char* label = gpiod_chip_info_get_label(info);

                for (const char* t : targets) {
                    if (strstr(label, t)) {
                        gpiod_chip_info_free(info);
                        gpiod_chip_close(chip);
                        return "/dev/" + name;
                    }
                }
            }
            gpiod_chip_close(chip);
        }
        setGlobalError(ErrorRegister::CHIP_NOT_FOUND);
        return "";
    }



    void registerPin(unsigned int pin, PinDirection dir, Edge edge, unsigned long debounce_ms = 30) {
        std::lock_guard<std::mutex> lock(mtx);

        if (pins.count(pin)) {
            setPinError(pin, ErrorRegister::PIN_ALREADY_LOCKED);
            return;
        }

        if (!chip) {
            setGlobalError(ErrorRegister::CHIP_NOT_OPEN);
            return;
        }

        gpiod_line_settings* settings = gpiod_line_settings_new();
        gpiod_line_config* lcfg = gpiod_line_config_new();
        unsigned int offset = pin;

        switch (dir) {
            case PinDirection::OUTPUT:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
                break;

            case PinDirection::INPUT:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_NONE);
                break;

            case PinDirection::PULLUP:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_NONE);
                break;

            case PinDirection::PULLDOWN:
                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
                gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_NONE);
                break;
        }

        bool isInterruptInput = (dir != PinDirection::OUTPUT);
        if (isInterruptInput) gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

        gpiod_line_config_add_line_settings(lcfg, &offset, 1, settings);

        gpiod_request_config* rcfg = gpiod_request_config_new();
        gpiod_request_config_set_consumer(rcfg, "gpiodWrapper");

        gpiod_line_request* req = gpiod_chip_request_lines(chip, rcfg, lcfg);

        gpiod_line_config_free(lcfg);
        gpiod_line_settings_free(settings);
        gpiod_request_config_free(rcfg);

        if (!req) {
            setPinError(pin, ErrorRegister::REQUEST_FAILED);
            return;
        }

        PinState state;
        state.request = req;
        state.direction = dir;
        state.edge = edge;
        if (isInterruptInput) state.interruptDebounceMs = debounce_ms;
        else if (pin < 64) activeLineReqFds[pin].store(gpiod_line_request_get_fd(req), std::memory_order_relaxed);

        pins[pin] = std::move(state);
    }


    PinState* getPinStateLocked(unsigned int pin) {
        auto it = pins.find(pin);
        if (it == pins.end()) {
            setPinError(pin, ErrorRegister::PIN_INVALID);
            return nullptr;
        }
        return &it->second;
    }


    std::optional<bool> readPinLocked(unsigned int pin) {
        auto* state = getPinStateLocked(pin);
        if (!state || !state->request) return std::nullopt;

        const int value = gpiod_line_request_get_value(state->request, pin);
        if (value < 0) {
            setPinError(pin, ErrorRegister::PIN_READ_FAILED);
            return std::nullopt;
        }

        return value != 0;
    }


    bool getDebouncedPinLocked(unsigned int pin, bool rawValue, unsigned long debounce_ms) {
        auto& state = pins.at(pin).pinDebounce;
        const auto now = now_ms();

        if (!state.initialized) {
            state.initialized = true;
            state.stableValue = rawValue;
            state.currentValue = rawValue;
            state.lastEvent = now;
            return state.stableValue;
        }

        if (rawValue != state.currentValue) {
            state.currentValue = rawValue;
            state.lastEvent = now;
        }

        if (state.currentValue == state.stableValue) return state.stableValue;
        if (now - state.lastEvent >= debounce_ms) state.stableValue = state.currentValue;

        return state.stableValue;
    }


    bool getDebouncedEventLocked(unsigned int pin, bool rawValue, unsigned long debounce_ms) {
        auto& state = pins.at(pin).eventDebounce;
        const auto now = now_ms();

        if (!state.initialized) {
            state.initialized = true;
            state.stableValue = rawValue;
            state.currentValue = rawValue;
            state.lastEvent = now;
            return state.stableValue;
        }

        if (rawValue != state.currentValue) {
            state.currentValue = rawValue;
            state.lastEvent = now;
        }

        if (state.currentValue == state.stableValue) return state.stableValue;
        if (now - state.lastEvent >= debounce_ms) state.stableValue = state.currentValue;

        return state.stableValue;
    }


    bool safeStatePin(gpiod_line_request* request, unsigned int pin) {

        if (pin < 64) activeLineReqFds[pin].store(-1, std::memory_order_relaxed);

        gpiod_line_settings* settings = gpiod_line_settings_new();
        if (!settings) return false;

        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

        gpiod_line_config* config = gpiod_line_config_new();
        if (!config) {
            gpiod_line_settings_free(settings);
            return false;
        }

        int ret = gpiod_line_config_add_line_settings(config, &pin, 1, settings);
        if (ret == 0) ret = gpiod_line_request_reconfigure_lines(request, config);

        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);

        return ret == 0;
    }


    void stopPinThread(unsigned int pin) {
        std::thread threadToJoin;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = pins.find(pin);
            if (it == pins.end())  return;
            if (it->second.running) it->second.running->store(false);
            if (it->second.thread.joinable()) threadToJoin = std::move(it->second.thread);
            if (it->second.pwm.initialized) it->second.pwm.initialized = false;
        }

        if (threadToJoin.joinable()) threadToJoin.join();
    }


    void stopAllThreads() {
        std::vector<std::thread> threadsToJoin;

        {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& [pin, state] : pins) {
                if (state.running) state.running->store(false);
                if (state.thread.joinable()) threadsToJoin.push_back(std::move(state.thread));
            }
        }

        for (auto& t : threadsToJoin)
            if (t.joinable()) t.join();
    }


    void startInterruptWorker() {
        if (interruptWorkerRunning.exchange(true)) return;

        interruptWorker = std::thread([this]() {
            while (true) {
                InterruptEvent ev;

                {
                    std::unique_lock<std::mutex> lock(interruptQueueMtx);
                    interruptCv.wait(lock, [this] { return !interruptQueue.empty() || !interruptWorkerRunning.load(); });
                    if (!interruptWorkerRunning.load() && interruptQueue.empty()) break;
                    if (interruptQueue.empty()) continue;
                    ev = std::move(interruptQueue.front());
                    interruptQueue.pop();
                }

                if (ev.callback) ev.callback(static_cast<int>(ev.pin));
            }
        });
    }


    void stopInterruptWorker() {

        {
            std::lock_guard<std::mutex> lock(interruptQueueMtx);
            interruptWorkerRunning.store(false);
        }

        interruptCv.notify_all();
        if (interruptWorker.joinable()) interruptWorker.join();
    }


    void setGlobalError(ErrorRegister err) {
        std::lock_guard<std::mutex> lock(errorMtx);
        globalErrors_.set(static_cast<size_t>(err));
    }


    void setPinError(unsigned int pin, ErrorRegister err) {
        std::lock_guard<std::mutex> lock(errorMtx);
        pinErrors_[pin].set(static_cast<size_t>(err));
    }


    std::string strError(uint8_t num, int pin = -1) const {
        std::string output_str = "";
        std::string pin_num = "";

        if (pin >= 0) pin_num = std::to_string(pin) + " ";

        switch (static_cast<ErrorRegister>(num)) {
            case ErrorRegister::NO_ERRORS:              output_str = "No errors"; break;
            case ErrorRegister::PERMISSION_DENIED:      output_str = "Access to resources denied"; break;
            case ErrorRegister::SYSTEM_OUT_OF_MEMORY:   output_str = "No memory reserved for event buffer or threads"; break;
            case ErrorRegister::KERNEL_BUFFER_OVERFLOW: output_str = "Event buffer overflow"; break;
            case ErrorRegister::HARDWARE_LOST:          output_str = "Hardware has occurred"; break;
            case ErrorRegister::INVALID_ARGUMENT:       output_str = "Incorrect arguments"; break;
            case ErrorRegister::CHIP_NOT_FOUND:         output_str = "Chip not found"; break;
            case ErrorRegister::CHIP_NOT_OPEN:          output_str = "Chip not open"; break;
            case ErrorRegister::NO_CHIP_OPEN:           output_str = "No chip open"; break;
            case ErrorRegister::PIN_ALREADY_LOCKED:     output_str = "Pin " + pin_num + "is already configured"; break;
            case ErrorRegister::PIN_INVALID:            output_str = "Pin " + pin_num + "is not configured"; break;
            case ErrorRegister::PIN_READ_FAILED:        output_str = "Read pin " + pin_num + "failed"; break;
            case ErrorRegister::PIN_WRITE_FAILED:       output_str = "Write pin " + pin_num + "failed"; break;
            case ErrorRegister::REQUEST_FAILED:         output_str = "Line request for pin " + pin_num + "failed"; break;
            default: output_str = "Unknown error"; break;

        }

        return output_str;
    }


#ifndef GPIODWRAP_NO_SIGNALS

    static_assert(std::atomic<bool>::is_always_lock_free, "Error: std::atomic<bool> is not lock-free on this platform!");

    std::thread watchdog;
    struct sigaction oldSigInt_{};
    struct sigaction oldSigTerm_{};

    static inline std::atomic<bool> cleanup_done_{false};
    static inline std::atomic<bool> shutdown_requested_ {false};
    static inline std::atomic<int> caught_signal_{0};


    static void signalHandler(int signum) {
        caught_signal_.store(signum, std::memory_order_relaxed);   
        shutdown_requested_.store(true, std::memory_order_relaxed);
    }


    void setupSignalHandling() {
    
        struct sigaction sa {};
        sa.sa_handler = &gpiodWrap::signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        ::sigaction(SIGINT, &sa, &oldSigInt_);
        ::sigaction(SIGTERM, &sa, &oldSigTerm_);

        watchdog = std::thread([this]() {
            while (!shutdown_requested_.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            closeChip();
            cleanup_done_.store(true, std::memory_order_relaxed);

            int sig = caught_signal_.load(std::memory_order_relaxed);
            if (sig != 0) {
                ::sigaction(SIGINT, &oldSigInt_, nullptr);
                ::sigaction(SIGTERM, &oldSigTerm_, nullptr);

                struct sigaction sa_dfl{};
                sa_dfl.sa_handler = SIG_DFL;
                ::sigemptyset(&sa_dfl.sa_mask);
                ::sigaction(sig, &sa_dfl, nullptr);
                ::raise(sig);
            }
        });

        watchdog.detach();
    }
#endif

};


namespace gpiodwrap {

    constexpr auto INPUT    = gpiodWrap::PinDirection::INPUT;
    constexpr auto OUTPUT   = gpiodWrap::PinDirection::OUTPUT;
    constexpr auto PULLUP   = gpiodWrap::PinDirection::PULLUP;
    constexpr auto PULLDOWN = gpiodWrap::PinDirection::PULLDOWN;

    constexpr auto HIGH     = gpiodWrap::PinValue::HIGH;
    constexpr auto LOW      = gpiodWrap::PinValue::LOW;

    constexpr auto RISING   = gpiodWrap::Edge::RISING;
    constexpr auto FALLING  = gpiodWrap::Edge::FALLING;
    constexpr auto BOTH     = gpiodWrap::Edge::BOTH;

    constexpr auto NO_EVENT   = gpiodWrap::PinEvent::NO_EVENT;
    constexpr auto IS_RISING  = gpiodWrap::PinEvent::IS_RISING;
    constexpr auto IS_FALLING = gpiodWrap::PinEvent::IS_FALLING;

    constexpr auto NO_ERRORS              = gpiodWrap::ErrorRegister::NO_ERRORS;
    constexpr auto CHIP_NOT_FOUND         = gpiodWrap::ErrorRegister::CHIP_NOT_FOUND;
    constexpr auto CHIP_NOT_OPEN          = gpiodWrap::ErrorRegister::CHIP_NOT_OPEN;
    constexpr auto PIN_ALREADY_LOCKED     = gpiodWrap::ErrorRegister::PIN_ALREADY_LOCKED;
    constexpr auto PIN_INVALID            = gpiodWrap::ErrorRegister::PIN_INVALID;
    constexpr auto PIN_READ_FAILED        = gpiodWrap::ErrorRegister::PIN_READ_FAILED;
    constexpr auto PIN_WRITE_FAILED       = gpiodWrap::ErrorRegister::PIN_WRITE_FAILED;
    constexpr auto REQUEST_FAILED         = gpiodWrap::ErrorRegister::REQUEST_FAILED;
    constexpr auto PERMISSION_DENIED      = gpiodWrap::ErrorRegister::PERMISSION_DENIED;
    constexpr auto SYSTEM_OUT_OF_MEMORY   = gpiodWrap::ErrorRegister::SYSTEM_OUT_OF_MEMORY;
    constexpr auto KERNEL_BUFFER_OVERFLOW = gpiodWrap::ErrorRegister::KERNEL_BUFFER_OVERFLOW;
    constexpr auto HARDWARE_LOST          = gpiodWrap::ErrorRegister::HARDWARE_LOST;
    constexpr auto INVALID_ARGUMENT       = gpiodWrap::ErrorRegister::INVALID_ARGUMENT;

} //namespace gpiodwrap
