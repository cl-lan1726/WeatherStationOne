//
//  station-specific configuration: WiFi/MQTT connection and sensor calibration constants.
//  everything here is meant to be customized per station/deployment.
//

#ifndef _STATIONCONFIG_H_
#define _STATIONCONFIG_H_

#include <WeatherConfig.h>

/****************************************************************************************************
  WiFi
 ****************************************************************************************************/

#define WIFI_SSID "your-ssid" // customize
#define WIFI_PASSWORD "your-password" // customize

/****************************************************************************************************
  MQTT
 ****************************************************************************************************/

#define MQTT_BROKER_HOST "192.168.1.10" // customize
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "weatherstation" // customize if you run more than one station
#define MQTT_TOPIC_DATA "weatherstation/data"

/****************************************************************************************************
  reporting interval
 ****************************************************************************************************/

#define DEFAULT_SECONDS_BETWEEN_REPORTS 20 // customize, seconds between MQTT publishes

/****************************************************************************************************
  wind speed calibration: assuming the cup/vane centers move at wind speed, we have
    speed = (rotations * circumference) / time
  with a factor for loss of cup speed compared to wind (ANEMOMETER_LOSS)
 ****************************************************************************************************/

#if USE_WIND_REED||USE_WIND_AS5600
#define ANEMOMETER_RADIUS 0.08 // 80mm = 0.08m, customize
#define ANEMOMETER_LOSS 1.18 // customize
#define DEFAULT_WINDSPEED_FACTOR (2*M_PI*ANEMOMETER_RADIUS*ANEMOMETER_LOSS)
#else
#define DEFAULT_WINDSPEED_FACTOR 2.7
#endif // USE_WIND_REED||USE_WIND_AS5600

#define DEFAULT_MEASUREMENT_HEIGHT 10.0 // meters above ground, customize; 10.0 = no compensation

/****************************************************************************************************
  rain gauge calibration: 8 pulses = 25ml, one bucket = 3.125ml
 ****************************************************************************************************/

#define DEFAULT_BUCKET_TRIGGER_VOLUME 3125.0f // mm3 per bucket tip, customize

#endif // _STATIONCONFIG_H_
