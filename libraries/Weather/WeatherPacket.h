//
//  weather data, serialized to JSON for MQTT publishing
//

#ifndef _WEATHERPACKET_H_
#define _WEATHERPACKET_H_

#include <Arduino.h>
#include <WeatherConfig.h>

#define UNDEFINEDVALUE -1.0
#define STRINGNOTINITIALIZED "-"

class WeatherPacket {

  public:

    //  rain gauge
    double mDeltaRainMM; // mm

    //  temperature et al
    float mTemperatureDegreeCelsius; // degree Celsius
    float mPressureHPA; // hPa
    float mHumidityPercent; // %

    //  wind vane
    float mWindDirectionDegrees; // 0..359.9, compass degrees

    //  anemometer
    float mWindSpeedMpS;

    WeatherPacket() {
      mDeltaRainMM = UNDEFINEDVALUE;

      mTemperatureDegreeCelsius = UNDEFINEDVALUE;
      mPressureHPA = UNDEFINEDVALUE;
      mHumidityPercent = UNDEFINEDVALUE;

      mWindDirectionDegrees = UNDEFINEDVALUE;
      mWindSpeedMpS = UNDEFINEDVALUE;
    }

    void print(Print *p) {
#if USE_RAIN
			if (mDeltaRainMM!=UNDEFINEDVALUE) {
      	p->print("rain: ");
        p->print(mDeltaRainMM, 1);
      	p->println(" mm delta");
      }
#endif // USE_RAIN

#if USE_TEMPERATURE
			if (mTemperatureDegreeCelsius!=UNDEFINEDVALUE) {
					p->print("temperature: ");
					p->print(mTemperatureDegreeCelsius, 1);
					p->println(" degree C");

					p->print("pressure: ");
					p->print(mPressureHPA, 1);
					p->println(" hPa");

					p->print("humidity: ");
					p->print(mHumidityPercent, 0);
					p->println(" %rH");
			}
#endif // USE_TEMPERATURE

#if USE_WIND_AS5600
			if (mWindDirectionDegrees!=UNDEFINEDVALUE) {
					p->print("wind direction: ");
					p->print(mWindDirectionDegrees, 1);
					p->println(" degree");
			}

			if (mWindSpeedMpS!=UNDEFINEDVALUE) {
				p->print("wind speed: ");
				p->print(mWindSpeedMpS, 1);
				p->println(" m/s");
			}
#endif // USE_WIND_AS5600
    }

		const char *jsonLine(const char *name, bool valid, float value, int precision, bool closingComma = true) {
			static char buffer[128];

			if (valid)
				snprintf(buffer, 128, "\t\"%s\" : %.*f%s\n", name, precision, value, closingComma?",":"");
			else
				snprintf(buffer, 128, "\t\"%s\" : \"%s\"%s\n", name, STRINGNOTINITIALIZED, closingComma?",":"");

			return buffer;
		}

		//  MQTT consumers want the 16-point compass label (as the pre-AS5600-PWM firmware sent),
		//  not a raw degree number; the float stays internally for calibration precision (see
		//  WIND_DIRECTION_OFFSET_DEGREES / README "Wind direction calibration")
		const char *windDirectionToCompass(float degrees) {
			static const char *directions[] = {
				"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
				"S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
			};

			int index = ((int) (degrees/22.5f+0.5f))%16;
			if (index<0)
				index += 16;

			return directions[index];
		}

    String toJson(String linePrefix = "") {

      String json = linePrefix + "{\n";

			json += linePrefix + jsonLine("raindelta", mDeltaRainMM!=UNDEFINEDVALUE, mDeltaRainMM, 1);
			json += linePrefix + jsonLine("temperature", mTemperatureDegreeCelsius!=UNDEFINEDVALUE, mTemperatureDegreeCelsius, 1);
			json += linePrefix + jsonLine("humidity", mHumidityPercent!=UNDEFINEDVALUE, mHumidityPercent, 1);
			json += linePrefix + jsonLine("pressure", mPressureHPA!=UNDEFINEDVALUE, mPressureHPA, 1);

			if (mWindDirectionDegrees!=UNDEFINEDVALUE)
				json += linePrefix + "\t\"winddirection\" : \"" + String(windDirectionToCompass(mWindDirectionDegrees)) + "\",\n";
			else
				json += linePrefix + "\t\"winddirection\" : \"" STRINGNOTINITIALIZED "\",\n";

			json += linePrefix + jsonLine("windspeed", mWindSpeedMpS!=UNDEFINEDVALUE, mWindSpeedMpS, 1, false);

      json += linePrefix + "}";

      return json;
    }
};

#endif // _WEATHERPACKET_H_
