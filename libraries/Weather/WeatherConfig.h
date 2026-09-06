/****************************************************************************************************
  development settings
 ****************************************************************************************************/
#define DEBUG 1 // customize, more text output

//	make implementation decisions
#define USE_WIND_AS5600 1 // customize, enable code for anemometer and wind vane using AS5600 PWM output
#define USE_TEMPERATURE 1 // customize, enable code for temperature at al
#define USE_RAIN 1 // customize, enable code for rain gauge

/****************************************************************************************************
  configuration

  pin choices below avoid ESP32-C6-DevkitM-1 strapping pins (GPIO4/5/8/9/15) and the
  USB-JTAG pins (GPIO12/13) - double check against your actual wiring
 ****************************************************************************************************/

//  wind vane and anemometer: both are AS5600 magnetic encoders read via their PWM OUT pin (no I2C)
#if USE_WIND_AS5600
#	define WIND_VANE_PIN 2 // wind direction AS5600 PWM OUT
#	define WINDSPEED_PIN 3 // wind speed AS5600 PWM OUT
#endif // USE_WIND_AS5600

//  wind speed/measurement height calibration constants and the rain bucket trigger
//  volume live in StationConfig.h alongside the other station-specific customizations

//  rain gauge (reed contact)
#if USE_RAIN
#define RAIN_PIN 1
#endif // USE_RAIN

//  temperature/humidity/pressure: LaskaKit outdoor meteo THP board (SHT40 + BMP280) on I2C
#define SDA_PIN 6
#define SCL_PIN 7

//  others
#define LED_PIN 10 // avoid GPIO0, it doubles as the BOOT button on many ESP32-C6 devkits

//  other constants
#define MS2S_FACTOR 1000ul // conversion factor for milli seconds to seconds
