# Changelog gpiodWrap
# Autor Kay Donau

Alle signifikanten Änderungen am gpiodWrap werden hier dokumentiert.


## [1.2.1] - 29.09.2026

> Die angewandte Methode des Signal-Handlings kann Deadlocks und Crashes verursachen.
> Der Interrupt-Thread watchInterrupt hängt bis zu 10 Sekunden fest in wait_edge_events und
  prüft das runFlag erst nach Ablauf der verbleibenden Zeit, was zu unerwarteten verhalten führen kann.
> Selbst angewendete Handler werden bei dieser Anwendungform gegebenenfalls überschrieben.
> Das Signal-Handling kann jetzt flexibel über den Konstruktor deaktiviert werden,
  sodass die volle Kontrolle über das Abfangen von Signalen (wie SIGINT oder SIGTERM)
  in eigenen Anwendung übernommen werden kann.

### Added
- gpiodWrap(): Auto Chip / SignalHandling::Enabled
- gpiodWrap(SignalHandling sh): SignalHandling::Enabled / Disabled (default=Enabled) gedeckelt mit enum class
- gpiodWrap(pin): Für die spezielle Chip-Auswahl
- getStrErr(): Text-Ausgabe des ErrorRegisters
- getStrErrPin(pin): Text-Ausgabe pin fehler 

### Changed
- getErrorStr(): Wurde komplett neu geschrieben und in getStrErr() bzw. getStrErrPin() umbenannt.

### Fixed
- Signal-Handling
- watchInterrupt()
- signalHandler()

- getPinErrorNum(): return 0;

---

## [1.2.0] - 26.09.2026

> Schwerpunkte: Thread-Sicherheit, Sicherer Ausgangszustand der Pins, Fehler-Behandlung, Event-Behandung
> CMake-System: Vereinfachtes erstellen der Beispiele.

### Added
- getPinEvent(pin, debounce_ms): Liest Pin-Flankenereignisse mit integrierter Software-Entprellung aus und gibt PinEvent (IS_RISING, IS_FALLING, NO_EVENT) zurück.

- bindInterrupt(pin, dir, edge, debounce_ms): Registriert und konfiguriert einen GPIO-Pin für Interrupt-Erkennung mit frei wählbarer Flanke und Entprellzeit.
- unbindInterrupt(pin): Meldet den Interrupt für den angegebenen Pin ab und beendet den zugehörigen Thread.
- watchInterrupt(pin, callback): Startet die Interrupt-Überwachung für einen Pin und übergibt erkannte Events an den angegebenen Callback (void() oder void(int)).

- anyGlobalError(): Prüft, ob überhaupt ein Globaler-Fehler im Register vorliegt.
- hasGlobalError(enum): Prüft, ob ein bestimmter Fehler im Bitregister gesetzt ist.
- getGlobalErrorNum(): Gibt die Bit-Nummer des ersten globalen Fehlers zurück.

- anyPinError(pin): Prüft, ob überhaupt Pin-Fehler im Register vorliegt.
- hasPinError(pin, enum): Prüft, ob ein spezifischer Pin-Fehler im Bitregister gesetzt ist.
- getPinErrorNum(pin): Gibt die Bit-Nummer des ersten aktiven Pin-Fehlers zurück.

- clearGlobalErrors(): Setzt das globale Fehler-Register zurück.
- clearGlobalError(enum): Löscht gezielt Fehler im globalen Fehler-Register.

- clearPinErrors(pin): Setzt das ganze Fehler-Register eines Pins zurück.
- clearPinError(pin, enum): Löscht den angegebenen Pin-Fehler in seinem Register.

### Komfort
- getErrorStr(): Gibt den globalen Systemfehler als Text zurück.
- getErrorStr(enum): Gibt spezifische Globale-Fehler zurück.
- getErrorStr(pin): Gibt den aktuellen Fehler des angegebenen Pins als Text zurück.
- getErrorStr(enum, pin): Gibt spezifische Pin-Fehler zurück.

- softPwm(pin, percent, frequency): Ersetzt pwmPin(). Wurde auf Mikroskunden-Präzision (high_us, low_us) umgestellt und erlaubt dynamische Änderungen des Duty Cycles zur Laufzeit ohne Thread-Neustart.

### Private
- registerPin(): Übernimmt zentral die Konfiguration der GPIO-Lines via libgpiod v2 API (gpiod_line_settings, gpiod_line_config, gpiod_request_config).
- PinState Struct: Zentralisierte Struktur zur Verwaltung aller Pin-Ressourcen (Requests, Threads, PWM-Zustände und dedizierte Entprell-Instanzen).
- InterruptWorker: Dedizierter Worker-Thread mit thread-sicherer Ereignis-Warteschlange (std::queue, std::condition_variable) zur entkoppelten Callback-Ausführung.
- safeStatePin(): Schaltet Output-Pins beim Schließen oder Zurücksetzen automatisch in einen sicheren Input- und Ausgangs-Zustand um.

- Signal-Handler (SIGINT, SIGTERM) zur automatischen und sicheren Freigabe der GPIO-Ressourcen bei unvorhergesehenem Anwendungsabbruch.

- ErrorRegister Enum: Bitmasken-basierte Fehlerverwaltung (uint16_t) für System- und Pin-Ebene.

### Expanded
- getPin(pin, debounce_ms): Gibt nun PinValue zur ausnahmefreien Fehlerbehandlung zurück und unterstützt optional direkte Software-Entprellung.
- closeChip(): Beendet alle aktiven Threads und den Interrupt-Worker sauber, leert die Event-Queue, setzt Ausgänge in einen sicheren Zustand zurück und gibt den Chip abfang- und ausnahmesicher frei.
- Namespace gpiodwrap: Name korrigiert (gpiowrap -> gpiodwrap) und um Aliase für PinEvent- und ErrorRegister-Konstanten erweitert.

### Removed
- debouncePin(): Die separate Entprell-Funktion wurde entfernt; Entprellung ist nun direkt in getPin(), getPinEvent() und watchInterrupt() integriert.
- attachInterrupt() / detachInterrupt(): Ersetzt durch das neue, flexiblere API-Trio bindInterrupt(), watchInterrupt() und unbindInterrupt().
- pwmPin(): Ersetzt durch softPwm().
- Exceptions in Basisfunktionen weitgehend entfernt und durch std::optional sowie den internen Fehler-Speicher ersetzt.

### Changed
- Direction Enum: Umbenannt in PinDirection zur eindeutigen Abgrenzung.
- Interrupt Event Handling: Vollständig überarbeitete Entkopplung zwischen der libgpiod-Hardware-Event-Schleife und der Callback-Ausführung via Interrupt-Worker-Queue, um Blocking im Event-Loop zu verhindern.
- Entprell-Architektur: Basiert nun auf dedizierten DebounceState-Strukturen pro Pin und Anwendungsfall (Lesen, Event, Interrupt).

### Fixed
- Thread-Safety-Probleme bei gleichzeitigem Zugriff auf Pin-Zustände durch konsequente Verwendung von Mutexes (mtx, errorMtx, interruptQueueMtx).
- Mögliche Ressourcenlecks und hängende Threads beim abrupten Beenden der Anwendung durch Signalabfangung und atomare Laufzeit-Flags.


---


## [1.1.0] - 07.09.2026

### Added
- openChip()`: Neue Methode zum initialisieren des GPIO-Chips. Wird keine Chipnummer eingegeben wir der GPIO-Chip automatisch initialisiert.
- findChip()`: Diese Funktion findet anhand der Label pinctrl-rp1, rp1-gpio, pinctrl-bcm2712, pinctrl-bcm2835 den richtingen Pfad der Raspberry Pi Boards 1-5 und übergibt in an den gpiod_chip_open Prozess.
- Chip Optionen`: Konstruktor übergabe eines bestimmten GPIO-Chips bleibt erhalten, es kann auch openChip als übergabe genutzt werden

- closeChip()`: Sicheres beenden aller Threads und Ressourcen freigeben während der Laufzeit. 


---

 
## [1.0.0] - 04.09.2026

> Vollständiges Refactoring / "Neugeburt" auf libgpiod 2.x API
> Klassenname angepasst gpiodWrapper zu gpiodWrap

### Added
- API-Shortcuts (`namespace gpiowrap`): Globale Aliase (`INPUT`, `OUTPUT`, `PULLUP`, `HIGH`, `LOW`, `RISING`, `FALLING`, etc.) für eine intuitive, Arduino-nahe Syntax ohne Namenskonflikte oder Makro-Probleme.
- debouncePin()`: Neue eingebaute Entprell-Methode zur sauberen Auswertung von Tastern, Reeds oder >Sensoren im Loop.
- Thread-Safety: Vollständige Absicherung aller Klassenressourcen via `std::mutex` und `std::atomic` für robusten Multithread-Einsatz.

### Changed
- Kapselung von Enums: Enums (`Direction`, `PinValue`, `Edge`) wurden als `enum class` direkt in die Klasse `gpiodWrap` verschoben (starke Typisierung, kein Global-Scope-Pollution).
- libgpiod 2.x Standard: Aktualisierung aller Event-Enums auf die korrekte libgpiod 2.x Nomenklatur (z. B. `GPIOD_LINE_EDGE_RISING`).

### Fixed
- Deadlock-Behebung in Interrupts: `stopPinThread()` prüft nun die Thread-ID (`get_id()`), was Einfrieren/Deadlocks verhindert, wenn ein Interrupt-Callback versucht, sich selbst neu zu konfigurieren oder zu stoppen.
- Performance & Buffer-Leak: `gpiod_edge_event_buffer` wird in Event-Loops jetzt einmalig statt pro Durchlauf allokiert und wiederverwendet (verhindert Heap-Fragmentierung).
- Data Races vermieden: Sichere Aufrufe und Löschungen von Thread-Handles und Edge-Events.


---


## [0.1.1] - 16.01.2026
### Fixed
- GPIOD_LINE_BIAS_DISABLE -> GPIOD_LINE_BIAS_D