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

## Wind direction calibration

The AS5600's raw angle is relative to wherever its magnet happens to sit when mounted, not to true north, so it needs a one-time offset per installation. There's no remote/runtime calibration protocol (deliberately - see the MQTT config note above); instead set a constant and reflash:

1. Leave `WIND_DIRECTION_OFFSET_DEGREES` at `0.0f` in `StationConfig.h` and flash the station.
2. Physically point the wind vane's reference mark at true north.
3. Read the current `winddirection` value from a report - either the Serial monitor (with `DEBUG` enabled) or by subscribing to `MQTT_TOPIC_DATA`, e.g. `mosquitto_sub -h <broker> -t weatherstation/data`. Temporarily lowering `DEFAULT_SECONDS_BETWEEN_REPORTS` (e.g. to 5) makes this quicker. Call this reading `X`.
4. Set `WIND_DIRECTION_OFFSET_DEGREES` to `-X` (keep it within ±360; e.g. if `X` is 250, use `-250`) and reflash.
5. Point the vane at north again and confirm the reported value is now close to `0`/`360`. Restore `DEFAULT_SECONDS_BETWEEN_REPORTS` to your normal value if you lowered it for step 3.

## TODO

- add support for illumination and ground humidity sensors
- add help on all the pitfalls one may run into creating this project
