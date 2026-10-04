<div align="center">
  <img src="/resources/images/gw.png" alt="gpiodWrap Logo" width="260">
</div>
<br>

A lightweight and user-friendly C++ wrapper for **libgpiod 2.x**, designed to make GPIO access on Linux and Raspberry Pi simple, readable, and intuitive.
  
  
---

## ✨ Features

✔️ Simple GPIO input/output  
✔️ One-line pin configuration  
✔️ Pin Event (IS_RISING, IS_FALLING)  
✔️ Interrupt support (RISING, FALLING, BOTH)  
✔️ Non-blocking debouncing function  
✔️ Automatic cleanup & securely reset  
✔️ No dynamic memory handling required  
✔️ Asynchron and without continuous polling  
✔️ Works with libgpiod 2.x  


---
# 🧩 Provided Functions

Instead of complex gpiod structures, this wrapper provides easy functions like:

<br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Basic configuration</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>gpiodWrap gpio</code></td>
      <td>Automatic search chip number.</td>
    </tr>
    <tr>
      <td><code>gpiodWrap gpio(chipnum)</code></td>
      <td>Specific chip number.</td>
    </tr>
  </tbody>
</table>
<br><br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Basic function</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>openChip(chipnum)</code></td>
      <td>Open specifically.</td>
    </tr>
    <tr>
      <td><code>closeChip()</code></td>
      <td>Returns outputs to a safe state, releases resources.</td>
    </tr>
    <tr>
      <td><code>configurePin(pin, direction)</code></td>
      <td>Configured and registered PIN.</td>
    </tr>
    <tr>
      <td><code>setPin(pin, HIGH/LOW)</code></td>
      <td>Sets pin high/low.</td>
    </tr>
    <tr>
      <td><code>getPin(pin, debounce)</code></td>
      <td>Reads pin optional debouncing.</td>
    </tr>
    <tr>
      <td><code>getPinEvent(pin, debounce)</code></td>
      <td>Reads pin edge events with integrated software debouncing and outputs PinEvent.</td>
    </tr>
    <tr>
      <td><code>resetPin(pin)</code></td>
      <td>resets the PIN and releases its resources.</td>
    </tr>
  </tbody>
</table>
<br><br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Interrupt methods</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>bindInterrupt(pin, direction, edge, debounce)</code></td>
      <td>Registers and configures a GPIO pin for interrupt detection with debouncing.</td>
    </tr>
    <tr>
      <td><code>watchInterrupt(pin, userCallback void/void(int))</code></td>
      <td>Interrupt monitoring; passes events to the specified callback.</td>
    </tr>
    <tr>
      <td><code>unbindInterrupt(pin)</code></td>
      <td>Disables interrupt for pin and terminates the associated thread.</td>
    </tr>
  </tbody>
</table>
<br><br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Error handling</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>anyGlobalError()</code></td>
      <td>Returns a global error</td>
    </tr>
    <tr>
      <td><code>hasGlobalError(enum)</code></td>
      <td>Returns a specific global error</td>
    </tr>
    <tr>
      <td><code>getGlobalErrorNum()</code></td>
      <td>Returns global error number.</td>
    </tr>
    <tr>
      <td><code>anyPinError(pin)</code></td>
      <td>Returns a pin error</td>
    </tr>
    <tr>
      <td><code>hasPinError(pin, enum)</code></td>
      <td>Returns a specific pin error.</td>
    </tr>
    <tr>
      <td><code>getPinErrorNum(pin)</code></td>
      <td>Returns pin error number.</td>
    </tr>
    <tr>
      <td><code>clearGlobalErrors()</code></td>
      <td>Clears all global errors</td>
    </tr>
    <tr>
      <td><code>clearGlobalError(enum)</code></td>
      <td>Clears specific global error</td>
    </tr>
    <tr>
      <td><code>clearPinErrors(pin)</code></td>
      <td>Clears all specific pin errors</td>
    </tr>
    <tr>
      <td><code>clearPinError(pin, enum)</code></td>
      <td>Clears specific pin error</td>
    </tr>
  </tbody>
</table>
<br><br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Komfort functions</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>getStrErr()</code></td>
      <td>Returns error as text.</td>
    </tr>
    <tr>
      <td><code>getStrErrPin(pin)</code></td>
      <td>Returns specific pin errors.</td>
    </tr>
    <tr>
      <td><code>blinkPin(pin, interval, time)</code></td>
      <td>e.g., implementing an status LED</td>
    </tr>
    <tr>
      <td><code>softPwm(pin, percent, frequency)</code></td>
      <td>Dynamic Software PWM control LEDs, BUZZER...</td>
    </tr>
    <tr>
      <td><code>detachPin(pin, PinValue, PinValue, interval)</code></td>
      <td>e.g., status LEDs...</td>
    </tr>
  </tbody>
</table>
<br>

Perfect for hobbyists, students, and projects where you just want GPIO control without becoming a libgpiod expert.

---


## 📦 Dependencies

| Requirement | Version |
|-------------|---------|
| libgpiod    | ≥ 2.0   |
| C++         | ≥ C++17 |
| CMake       | optional for building |

Install libgpiod build-essential cmake(Debian / Raspberry Pi OS):

```bash
sudo apt update
sudo apt install -y libgpiod-dev build-essential cmake

gpioinfo -v
```

libgpiod-2.x self build e.g. RaspberryOS bullseye/bookworm
```
sudo apt update
sudo apt install -y build-essential autoconf automake libtool pkg-config autoconf-archive

wget https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-2.3.1.tar.gz
tar -xvf libgpiod-2.3.1.tar.gz
cd libgpiod-2.3.1

./configure --enable-tools
make -j4
sudo make install
sudo ldconfig

```
---


## 🚀 Basic Example

```cpp

#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrap.hpp"


int main() {
    try {

        using namespace gpiodwrap;

        gpiodWrap gpio;

        gpio.configurePin(17, Output);

        gpio.setPin(17, HIGH);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    
        gpio.setPin(17, LOW);
        std::this_thread::sleep_for(std::chrono::seconds(1));
  
        gpio.resetPin(17);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Destructor of gpiodWrap is called automatically
    return 0;
}

```

---


## ⚡ Interrupt Example

```cpp

#include <iostream>
#include <chrono>

#include "gpiodWrap.hpp"

gpiodWrap gpio;

int main() {

    using namespace gpiodwrap;

    gpio.bindInterrupt(22, PULLUP, FALLING, 60);

    gpio.watchInterrupt(pin, [](int pin) 
         { std::cout << "Falling-Event on Pin: " << pin << std::endl; });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

```
---


📁 examples/
```

 ├── blink.cpp       // Make individual LEDs blink
 ├── button.cpp      // Input example
 ├── button_led.cpp  // Input / Output example
 ├── buzzer.cpp      // Alert example
 ├── error.cpp       // Error handling
 ├── fade_led.cpp    // Dynamic PWM control
 ├── interrupt.cpp   // Kernel-Event 
 ├── led.cpp         // Output example
 ├── mockup.cpp      // Event / Debounce stress test
 ├── pin_event.cpp   // Input event falling/rising
 └── pwm.cpp         // Static PWM control


```
 
---


## 📦 Install build-essential and CMake 
```bash
sudo apt update
sudo apt upgrade -y
sudo apt install build-essential -y
sudo apt install cmake -y
```
---


## 🔧 Integration Example 

CMake-System
```
cd gpioWarp
mkdir build
cd build
cmake ..
make or make -j4

output bin/...
```
---


## 📦 Project Build Instructions

```bash
mkdir myproject
cd myproject
mkdir src include build
mv ../gpioWarp/gpiodWrap.hpp include
nano CMakeLists.txt
cd build
cmake ..
make or make -j4
cd ../bin/myproject
./myproject

```


## 🔧 Your build station looks like this:

```
 myproject
 │
 ├── CMakeLists.txx
 ├── build
 ├── bin
 │     └── myproject
 ├── include
 │     └── gpiodWrap.hpp
 └── src
        └── myproject.cpp

```

## 🔧 Your project CMakeLists.txt:

```

set(SOURCE_NAME myproject)

cmake_minimum_required(VERSION 3.16)
project(${SOURCE_NAME} VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${SOURCE_NAME}
    src/${SOURCE_NAME}.cpp
)

target_link_libraries(${SOURCE_NAME}
    gpiod
)

set_target_properties(${SOURCE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/bin/${SOURCE_NAME}
)

```

## or quickly
(The files to be compiled are located in one directory!)
```bash
 g++ myproject.cpp -o myproject -lgpiod
```
---

## 🛠️ Projekt 
Here's another nice example from a different project where I'm using gpiodWrap.

```cpp

#include <chrono>

#include "gpiodWrap.hpp"

gpiodWrap gpio;

constexpr int errorLED = 18;

void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long millis() {
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return duration_cast<milliseconds>(steady_clock::now() - start).count();
}

void errReport(unsigned long interval = 500) {

    static unsigned long last = 0;
    static bool state = true;
       
    if (millis() - last >= interval) {
        last = millis();   
        state = !state;
        gpiodwrap::gpio.setPin(faultLED, state ? HIGH : LOW);
    }    
}

int main() {

    using namespace gpiodwrap;
    gpio.configurePin(errorLED, Output);
    gpio.setPin(errorLED, LOW);

    bool syserror = true;

    while(true) {

        if (syserror) errReport(300);

        delay(20); 
    }
}

```
---


## 🚨 Hardware Safety & Signal Handling (Safe-by-Default)

Uncontrolled program termination (e.g., via `Ctrl + C` / `SIGINT or SIGTERM`) can be hazardous in hardware control applications if outputs connected to motors, relays, or heating elements remain active in an undefined state.

`gpiodWrap` addresses this by adhering to the **Safe-by-Default** principle:

* **Automatic Fail-Safe:**

    Upon receiving `SIGINT or SIGTERM`, all active output pins are automatically reverted to a safe, high-impedance input state (`INPUT`) before the GPIO chip is released.

* **Decoupled Watchdog Thread:**

    The POSIX signal handler exclusively modifies atomic flags (100% `async-signal-safe`). Hardware cleanup is executed asynchronously in a separate watchdog thread—completely free from C library locks or deadlock risks.

* **Optional Compile-Time Disabling:**

     If your application logic manages signal handling independently, the built-in signal mechanism can be disabled entirely with zero runtime overhead:

1. In C++ Source Code:
```
#define GPIODWRAP_NO_SIGNALS
#include "gpiodWrap.hpp"
```

2. Via CMake:
```
# old school
mkdir -p build && cd build
cmake .. -DGPIODWRAP_DISABLE_SIGNALS=ON
make or make -j4

# Directly in the project directory:
cmake -B build -DGPIODWRAP_DISABLE_SIGNALS=ON
cmake --build build
```

3. Direct GCC/Clang Invocation:
```
g++ -std=c++17 -DGPIODWRAP_NO_SIGNALS main.cpp -o app -lgpiod
```

---


## 📄 License

Copyright (C) 2026 Raspino-Projekt <dev@raspino.org>  
GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007  
You are free to use, modify, and distribute this project under the terms of the GPLv3.

---


## 🤝 Contributions

Pull requests and improvements are welcome.  
Feel free to fork, enhance, or suggest features.

---


⭐ If this wrapper helps your project — consider starring it on GitHub!
