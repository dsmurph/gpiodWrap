#!/bin/bash

# Pfad zum Mockup-Chip (bitte prüfen, ob es gpiochip14 ist)
CHIP_DIR="/sys/kernel/debug/gpio-mockup/gpiochip14"

DIR="INPUT"
PAUSE=2
WATCHPIN=25
GPIOPIN=(16 17 18)


if [ ! -d "$CHIP_DIR" ]; then
    echo "Fehler: GPIO-Chip unter $CHIP_DIR nicht gefunden!"
    exit 1
fi

echo "Starte GPIOWARP Test-Skript"
echo "Drücke [CTRL+C] zum Beenden."

CHANGEPIN=0
echo "${#GPIOPIN[@]}"
if [[ "$DIR" == "OUTPUT" ]]; then
  while true; do

    echo "Toggle gpio ${GPIOPIN[CHANGEPIN]}"
    echo 0 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}"
    echo 1 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}"
    echo 0 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}"
    echo 1 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}"
    echo 0 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}" 
    echo 1 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}" 
    echo 0 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}" 
    echo 1 > "$CHIP_DIR/${GPIOPIN[CHANGEPIN]}"                 
    sleep $PAUSE
    
    if (( "$CHANGEPIN" < (("${#GPIOPIN[@]}"-1)) )); then
        ((CHANGEPIN++))
    else
        CHANGEPIN=0
    fi 
  done

elif [[ "$DIR" == "INPUT" ]]; then
echo "$CHIP_DIR/$WATCHPIN"
  watch -n 0.1 "cat $CHIP_DIR/$WATCHPIN"
fi