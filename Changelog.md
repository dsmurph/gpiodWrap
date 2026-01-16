# Changelog

Alle signifikanten Änderungen am gpiodWrapper werden hier dokumentiert.

## [0.1.1] - 2026-01-16
### fixed
- GPIOD_LINE_BIAS_DISABLE -> GPIOD_LINE_BIAS_DISABLED

## [0.1.0] - 2025-11-23/25
### Added
- Erste Version des gpiodWrapper
- Unterstützung für GPIO Pins: setPin, getPin, resetPin, configurePin
- Interrupt-Handling mit attachInterrupt / detachInterrupt
- PWM-Funktionalität für LEDs oder Motoren
- Beispiele: blink.cpp, taster.cpp, pwm.cpp, interrupt.cpp, highlow.cpp, LEDTasterPWM.cpp
- CMake-Build-System eingerichtet
- Dokumentation: README.md, Installationsanleitung

