# 🚀 gpiodWrap

A lightweight and user-friendly C++ wrapper for **libgpiod 2.x**, designed to make GPIO access on Linux and Raspberry Pi simple, readable, and intuitive.

Instead of complex gpiod structures, this wrapper provides easy functions like:

```

configurePin()
setPin()
getPin()
resetPin()
debouncePin()
attachInterrupt()
detachInterrupt()

```

Perfect for hobbyists, students, and projects where you just want GPIO control — without becoming a libgpiod expert.

---


## 📦 Dependencies

| Requirement | Version |
|-------------|---------|
| libgpiod    | ≥ 2.0   |
| C++         | ≥ C++17 |
| CMake       | optional for building |

Install libgpiod (Debian / Raspberry Pi OS):

```bash
sudo apt install libgpiod-dev
```
libgpiod-2.x self build e.g. RaspberryOS bullseye/bookworm
```
sudo apt update
sudo apt install -y build-essential autoconf automake libtool pkg-config autoconf-archive

wget https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-2.2.2.tar.xz
tar -xvf libgpiod-2.2.2.tar.xz
cd libgpiod-2.2.2

./configure --enable-tools
make -j4
sudo make install
sudo ldconfig

```
---


## ✨ Features

✔️ Simple GPIO input/output  
✔️ One-line pin configuration  
✔️ Interrupt support (RISING, FALLING, BOTH)
✔️ Non-blocking debouncing function   
✔️ Automatic cleanup  
✔️ No dynamic memory handling required  
✔️ Works with libgpiod 2.x  

---


## 🧩 Provided Functions

| Function | Description |
|----------|-------------|
| `gpiodWrap(index)` | Opens `/dev/gpiochipX` |
| `configurePin(pin, Output/Input/Pullup/Pulldown)` | Configures pin direction |
| `setPin(pin, HIGH/LOW)` | Sets pin output state |
| `getPin(pin)` | Reads digital input |
| `resetPin(pin)` | Releases pin and clears configuration |
| `debouncePin()` | Non-blocking debouncing tool for push-buttons, reed switch and sensors |
| `attachInterrupt(pin, edge, callback)` | Executes function on edge event |
| `detachInterrupt(pin)` | Stops monitoring interrupt on the pin |

---


## 🚀 Basic Example

```cpp

#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrap.hpp"

gpiodWrap gpio(0);

int main() {
    
  using namespace gpiowrap;
  
  gpio.configurePin(17, OUTPUT);

  gpio.setPin(17, HIGH);
  std::this_thread::sleep_for(std::chrono::seconds(1));
    
  gpio.setPin(17, LOW);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  
  gpio.resetPin(17);

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

gpiodWrap gpio(0);

int main() {
    using namespace gpiowrap;

    gpio.configurePin(22, PULLDOWN);

    gpio.attachInterrupt(22, RISING, [](int pin) 
         {std::cout << "Interrupt! Pin: " << pin << std::endl;});

    while (true) {
        // Main application loop
        std::this_thread::sleep_for(std::chrono::milliseconds(10));    
    }
}

```
---


📁 examples/
```
 ├── debouncepin.cpp   // Non-blocking debouncing of GPIO signals
 ├── blink.cpp         // Make individual LEDs blink
 ├── taster.cpp        // Query buttons
 ├── pwm.cpp           // PWM-control unit for LEDs or motors
 ├── interrupt.cpp     // Interrupt on pins
 ├── highlow.cpp       // Set Pin high/low
 └── LEDTasterPWM.cpp  // Combination: LED, push button & PWM simultaneously

```
 
---


## 📦 Install build-essential and CMake 
```bash
sudo apt update
sudo apt upgrade -y
sudo apt install build-essential cmake -y
```
---


## 🔧 Integration Example 

CMakeLists.txt (change file.cpp for your project)
```cmake

set(SOURCE_NAME YOUR PROJECT NAME)

project(${SOURCE_NAME} VERSION 1.00 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

find_library(GPIOD_LIBRARY gpiod)

if(NOT GPIOD_LIBRARY)
    message(WARNING "libgpiod not found. Skipping '${SOURCE_NAME}'.\n sudo apt install libgpiod-dev")
else()
    add_executable(${SOURCE_NAME}
        src/${SOURCE_NAME}.cpp
    )

    target_link_libraries(${SOURCE_NAME}
        ${GPIOD_LIBRARY}
    )

    set_target_properties(${SOURCE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_SOURCE_DIR}/bin/${SOURCE_NAME}
    )
endif()

```
---


## 📦 Build Instructions

```bash

cd gpiodWrap-master
mkdir src include build
mv gpiodWrap.hpp include
mv your_project.cpp src
cd build
cmake ..
make

cd ../bin/your_project
./your_project

```

## 🔧 Your build station looks like this:
📁 gpiodWrap-master/

```
  CMakeLists.txt
  📁 include
   ├── gpiodWrap.hpp
  📁 src
   ├── debouncePin.cpp
   ├── blink.cpp
   ├── taster.cpp
   ├── pwm.cpp
   ├── interrupt.cpp
   ├── highlow.cpp
   └── combi.cpp
  📁 build
   ├── ...
  📁 bin
   ├── your_project

```

## or quickly
(The files to be compiled are located in one directory!)

```bash
 📁 gpiodWrap-master/
 ├── gpioWrap.hpp
 ├── your_project.cpp
 
 g++ your_project.cpp -o your_project -lgpiod
 
```
---

## 🛠️ Projekt 
Here's another nice example from a different project where I'm using gpiodWrap.

```cpp

#include <chrono>
#include "gpiodWrap.hpp"

gpiodWrap gpio(0);

using namespace gpiowrap;

int errorLED = 18;

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
        gpio.setPin(errorLED, state ? HIGH : LOW);
    }    
}

int main() {
    gpio.configurePin(errorLED, OUTPUT);
    gpio.setPin(errorLED, HIGH);

    bool syserror = true;

    while(true) {
        if (syserror) errReport(300);
        delay(20); 
    }
}

```
---
Millis and delay seen...?
Little header helper.
---

```


#pragma once

#include <chrono>
#include <thread>

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline unsigned long millis() {
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return duration_cast<milliseconds>(steady_clock::now() - start).count();
}

```

---

## 📄 License

MIT License  
You are free to use, modify, and distribute this project.

---


## 🤝 Contributions

Pull requests and improvements are welcome.  
Feel free to fork, enhance, or suggest features.

---


⭐ If this wrapper helps your project — consider starring it on GitHub!
