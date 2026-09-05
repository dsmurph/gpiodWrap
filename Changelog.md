# Changelog

Alle signifikanten Änderungen am gpiodWrap werden hier dokumentiert.

## [1.0.0] - 04.09.2026

**Vollständiges Refactoring / "Neugeburt" auf libgpiod 2.x API**

**Klassenname angepasst gpiodWrapper zu gpiodWrap**

### Added
- **API-Shortcuts (`namespace gpiowrap`):** Globale Aliase (`INPUT`, `OUTPUT`, `PULLUP`, `HIGH`, `LOW`, `RISING`, `FALLING`, etc.) für eine intuitive, Arduino-nahe Syntax ohne Namenskonflikte oder Makro-Probleme.
- **`debouncePin()`:** Neue eingebaute Entprell-Methode zur sauberen Auswertung von Tastern, Reeds oder >Sensoren im Loop.
- **Thread-Safety:** Vollständige Absicherung aller Klassenressourcen via `std::mutex` und `std::atomic` für robusten Multithread-Einsatz.

### Changed
- **Kapselung von Enums:** Enums (`Direction`, `PinValue`, `Edge`) wurden als `enum class` direkt in die Klasse `gpiodWrap` verschoben (starke Typisierung, kein Global-Scope-Pollution).
- **libgpiod 2.x Standard:** Aktualisierung aller Event-Enums auf die korrekte libgpiod 2.x Nomenklatur (z. B. `GPIOD_LINE_EDGE_RISING`).

### Fixed
- **Deadlock-Behebung in Interrupts:** `stopPinThread()` prüft nun die Thread-ID (`get_id()`), was Einfrieren/Deadlocks verhindert, wenn ein Interrupt-Callback versucht, sich selbst neu zu konfigurieren oder zu stoppen.
- **Performance & Buffer-Leak:** `gpiod_edge_event_buffer` wird in Event-Loops jetzt einmalig statt pro Durchlauf allokiert und wiederverwendet (verhindert Heap-Fragmentierung).
- **Data Races vermieden:** Sichere Aufrufe und Löschungen von Thread-Handles und Edge-Events.

---

## [0.1.1] - 16.01.2026
### Fixed
- GPIOD_LINE_BIAS_DISABLE -> GPIOD_LINE_BIAS_DISABLED

## [0.1.0] - 23/25.01.2025
### Added
- Erste Version des gpiodWrapper
- Unterstützung für GPIO Pins: setPin, getPin, resetPin, configurePin
- Interrupt-Handling mit attachInterrupt / detachInterrupt
- PWM-Funktionalität für LEDs oder Motoren
- Beispiele: blink.cpp, taster.cpp, pwm.cpp, interrupt.cpp, highlow.cpp, LEDTasterPWM.cpp
- CMake-Build-System eingerichtet
- Dokumentation: README.md, Installationsanleitung

