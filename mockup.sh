#!/bin/bash

#
# mockup erstellen pins ab 0; 40 stück
#   sudo modprobe gpio-mockup gpio_mockup_ranges=0,40
#
# mockup löschen
#   sudo rmmod gpio-mockup
#
# ausführbar machen
# chmod +x mockup.sh
#
# mockup benötigt root
#   sudo ./mockup.sh
#
# Dieses Skript liefert insgesamt 140 logische Flanken-Events und simuliert verschiedene Prellzeiten,
# Im Testprogramm mit 60-100ms debounce falling oder rising 13 und both 26 gültige Tasten-Events.
#


CHIP_DIR="/sys/kernel/debug/gpio-mockup/gpiochip14"

if [ ! -d "$CHIP_DIR" ]; then
    echo "Fehler: GPIO-Chip unter $CHIP_DIR nicht gefunden!"
    exit 1
fi

TEST_PIN=17


echo "Starte GPIO Mockup Test-Skript"
echo "Drücke [CTRL+C] zum Beenden."


set_pin() {
    local pin=$1
    local value=$2
    echo $value > "$CHIP_DIR/$pin"
}


sleep_ms() {
    local ms=$1
    sleep 0.$(printf "%03d" $ms)
}


simulate_button_press() {
    local pin=$1
    local hold_ms=$2
    local bounce_count=$3
    local bounce_ms=$4

    echo "  [Pin $pin] Druck simulieren (hold=${hold_ms}ms, bounces=$bounce_count @ ${bounce_ms}ms)..."

    set_pin $pin 0
    sleep_ms 1

    for ((i=0; i<bounce_count; i++)); do
        set_pin $pin 1
        sleep_ms $bounce_ms
        set_pin $pin 0
        sleep_ms $bounce_ms
    done

    sleep_ms $hold_ms

    set_pin $pin 1
    sleep_ms 1

    for ((i=0; i<bounce_count; i++)); do
        set_pin $pin 0
        sleep_ms $bounce_ms
        set_pin $pin 1
        sleep_ms $bounce_ms
    done

    sleep_ms 200
}


echo ""
echo "=== Test 1: Idealer Button ==="
set_pin $TEST_PIN 1
sleep_ms 100
set_pin $TEST_PIN 0
sleep_ms 100
set_pin $TEST_PIN 1
sleep_ms 200

echo ""
echo "=== Test 2: Button mit leichtem Prellen (3x @ 5ms) ==="
simulate_button_press $TEST_PIN 100 3 5

echo ""
echo "=== Test 3: Schlechter Button mit starkem Prellen (5x @ 20ms) ==="
simulate_button_press $TEST_PIN 150 5 20

echo ""
echo "=== Test 4: Mehrere schnelle Tastendrücke ==="
for press in {1..5}; do
    echo "  Druck $press"
    simulate_button_press $TEST_PIN 50 2 10
done

echo ""
echo "=== Test 5: Sehr schnelle Prellung (10x @ 2ms) ==="
simulate_button_press $TEST_PIN 200 10 2

echo ""
echo "=== Test 6: Sehr langsame Prellung (2x @ 50ms) ==="
simulate_button_press $TEST_PIN 100 2 50

echo ""
echo "=== Test fertig ==="