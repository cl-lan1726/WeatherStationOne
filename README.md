# WeatherStationOne

This is a DIY weather station sensor unit built around an ESP32 (targeting an ESP32-C6-DevkitM-1 board), mains-powered and always-on. It reports data over MQTT.

- collect common weather channels: rain, wind speed and direction, temperature, humidity, barometric pressure
- modular design allowing one to implement part of it, or extend the range of sensors
- high depth of production; DIY as much as possible

This repository includes everything you need to setup the electronic and software part of the sensor unit.

## Other Sources

Besides this repository, the 3D printable parts are provided on [Harry's Prusa Prints](https://www.printables.com/@Harry/collections/101642).

[Building instructions](http://www.met.fu-berlin.de/%7Estefan/huette.html) for the weather hut / Stevenson Screen created as part of the project.

## System Environment

This README describes how to implement the electronics and software part of the sensor unit using the Arduino IDE, targeting an ESP32-C6-DevkitM-1 board.

## Electronics

Legacy circuit diagrams for the sensor wiring are provided in `CircuitSensors.pdf` and `CircuitSensorsAS5600.pdf`; `plan.svg` documents the original combined system and is kept for historical reference. These predate the ESP32-C6/MQTT rework in this repository and are being updated incrementally as the firmware is rebuilt.

## Software Installation

- install support for your ESP32 developer board (ESP32-C6-DevkitM-1) in the Arduino IDE
- copy `libraries/Weather` to your Arduino library directory; on macOS, this is `~/Documents/Arduino/libraries`
- restart Arduino IDE afterwards
- compile and flash `sketches/weatherstation`

## TODO

- add support for illumination and ground humidity sensors
- add help on all the pitfalls one may run into creating this project
