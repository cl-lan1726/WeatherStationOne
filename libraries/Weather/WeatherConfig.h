/****************************************************************************************************
  development settings
 ****************************************************************************************************/
#define DEBUG 1 // customize, more text output

//	make implementation decisions
#define USE_WIND_REED 0 // customize, enable code for anemometer and wind vane using reeds/HC4051 muxer
#define USE_WIND_AS5600 0 // customize, enable code for anemometer and wind vane using reed/AS5600
#define USE_TEMPERATURE 0 // customize, enable code for temperature at al
#define USE_RAIN 1 // customize, enable code for rain gauge

/****************************************************************************************************
  configuration
 ****************************************************************************************************/

#define NUM_DIRECTIONS_PER_PIN 4

//  wind vane and anemometer
#if USE_WIND_AS5600
#	define WIND_VANE_PIN 32
#	define WINDSPEED_PIN 5
#endif // USE_WIND_AS5600

#if USE_WIND_REED
#	define WIND_VANE_S0 15 // address direction
#	define WIND_VANE_S1 2
#	define WIND_VANE_S2 18
#	define WIND_VANE_Z 5 // read direction
#	define WINDSPEED_PIN 32
#endif // USE_WIND_REED

#if USE_WIND_REED||USE_WIND_AS5600
#define NUM_COUNTS_PER_TURN 1 // depends on magnet / reed position
#endif // USE_WIND_REED||USE_WIND_AS5600
//  wind speed/measurement height calibration constants and the rain bucket trigger
//  volume live in StationConfig.h alongside the other station-specific customizations

//  rain gauge
#if USE_RAIN
#define RAIN_PIN 27
#endif // USE_RAIN

//  temperature et al
#define SDA_PIN 21 // documentation only
#define SCL_PIN 22 // documentation only

//  others
#define LED_PIN 4

//  other constants
#define MS2S_FACTOR 1000ul // conversion factor for milli seconds to seconds
