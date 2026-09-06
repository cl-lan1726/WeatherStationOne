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

Wind direction and wind speed are each read from their own AS5600 magnetic encoder via its PWM `OUT` pin (no I2C wiring needed for these two sensors). The rain gauge is a reed contact wired to a digital interrupt pin. Temperature, humidity, and barometric pressure come from a LaskaKit outdoor meteo THP board (Sensirion SHT40 + Bosch BMP280) on I2C. See `sketches/weatherstation/StationConfig.h` and `libraries/Weather/WeatherConfig.h` for the exact pin assignments and calibration constants - the defaults there are proposed values and should be checked against your own wiring.

Legacy circuit diagrams (`CircuitSensors.pdf`, `CircuitSensorsAS5600.pdf`, `plan.svg`) document the original HC-12/deep-sleep hardware and are kept for historical reference only; they predate the ESP32-C6/MQTT/AS5600-PWM rework in this repository.

## Software Installation

- install support for your ESP32 developer board (ESP32-C6-DevkitM-1) in the Arduino IDE
- install these libraries via the Arduino Library Manager: **PubSubClient** (Nick O'Leary, MQTT), **Adafruit SHT4x Library**, **Adafruit BMP280 Library**, **Adafruit Unified Sensor**
- copy `libraries/Weather` to your Arduino library directory; on macOS, this is `~/Documents/Arduino/libraries`
- restart Arduino IDE afterwards
- edit `sketches/weatherstation/StationConfig.h` to set your WiFi credentials, MQTT broker address, and sensor calibration constants
- compile and flash `sketches/weatherstation`

## TODO

- add support for illumination and ground humidity sensors
- add help on all the pitfalls one may run into creating this project
