//
//  binary packet of weather data
//

#ifndef _WEATHERPACKET_H_
#define _WEATHERPACKET_H_

#include <Packet.h>

#define STRINGNOTINITIALIZED "-"

class WeatherPacket : public Packet {

  private:

    //  header
    byte mMagicByte;

  public:

    //  rain gauge
    double mDeltaRainMM; // mm

    //  temperature et al
    float mTemperatureDegreeCelsius; // degree Celsius
    float mPressureHPA; // hPa
    float mHumidityPercent; // %

    //  wind vane
    char mWindDirection [4]; // three digits plus \0

    //  anemometer
    float mWindSpeedMpS;

  private:

    //  CRC16 checksum
    uint16_t  mCRC16;

  public:

    WeatherPacket() : Packet() {
      mDeltaRainMM = UNDEFINEDVALUE;

      mTemperatureDegreeCelsius = UNDEFINEDVALUE;
      mPressureHPA = UNDEFINEDVALUE;
      mHumidityPercent = UNDEFINEDVALUE;

      mWindDirection[0] = '\0';
      mWindSpeedMpS = UNDEFINEDVALUE;
    }

    void print(Print *p) {
    	p->print("magic byte: ");
			p->print(mMagicByte);
			p->println(mMagicByte==MAGICBYTE?" correct":" wrong");

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

#if USE_WIND_REED||USE_WIND_AS5600
			if (mWindDirection[0]) {
					p->print("wind direction: ");
					p->println(mWindDirection);
			}

			if (mWindSpeedMpS!=UNDEFINEDVALUE) {
				p->print("wind speed: ");
				p->print(mWindSpeedMpS, 1);
				p->println(" m/s");
			}
#endif

			p->print("checksum: ");
			p->print(mCRC16);
			p->println(mCRC16==crc16()?" correct":" wrong");
    }

#define STRING_WORKAROUND 1

#if STRING_WORKAROUND
		const char *jsonLine(const char *name, bool valid, float value, int precision, bool closingComma = true) {
			static char buffer[128];

			if (valid)
				snprintf(buffer, 128, "\t\"%s\" : %.*f%s\n", name, precision, value, closingComma?",":"");
			else
				snprintf(buffer, 128, "\t\"%s\" : \"%s\"%s\n", name, STRINGNOTINITIALIZED, closingComma?",":"");

			return buffer;
		}
#endif

    String json(String linePrefix = "") {

      String json = linePrefix + "{\n";

#if STRING_WORKAROUND
			json += linePrefix + jsonLine("raindelta", mDeltaRainMM!=UNDEFINEDVALUE, mDeltaRainMM, 1);
			json += linePrefix + jsonLine("temperature", mTemperatureDegreeCelsius!=UNDEFINEDVALUE, mTemperatureDegreeCelsius, 1);
			json += linePrefix + jsonLine("humidity", mHumidityPercent!=UNDEFINEDVALUE, mHumidityPercent, 1);
			json += linePrefix + jsonLine("pressure", mPressureHPA!=UNDEFINEDVALUE, mPressureHPA, 1);

      if (mWindDirection[0])
        json += linePrefix + "\t\"winddirection\" : \"" + String(mWindDirection) +"\",\n";
      else
        json += linePrefix + "\t\"winddirection\" : \"" STRINGNOTINITIALIZED "\",\n";

			json += linePrefix + jsonLine("windspeed", mWindSpeedMpS!=UNDEFINEDVALUE, mWindSpeedMpS, 1, false);
#else
      if (mDeltaRainMM!=UNDEFINEDVALUE)
        json += linePrefix + "\t\"raindelta\" : " + String(mDeltaRainMM, 1) +",\n";
      else
        json += linePrefix + "\t\"raindelta\" : \"" STRINGNOTINITIALIZED "\",\n";

      if (mTemperatureDegreeCelsius!=UNDEFINEDVALUE)
        json += linePrefix + "\t\"temperature\" : " + String(mTemperatureDegreeCelsius, 1) +",\n";
      else
        json += linePrefix + "\t\"temperature\" : \"" STRINGNOTINITIALIZED "\",\n";

      if (mHumidityPercent!=UNDEFINEDVALUE)
        json += linePrefix + "\t\"humidity\" : " + String(mHumidityPercent, 1) +",\n";
      else
        json += linePrefix + "\t\"humidity\" : \"" STRINGNOTINITIALIZED "\",\n";

      if (mPressureHPA!=UNDEFINEDVALUE)
        json += linePrefix + "\t\"pressure\" : " + String(mPressureHPA, 1) +",\n";
      else
        json += linePrefix + "\t\"pressure\" : \"" STRINGNOTINITIALIZED "\",\n";

      if (mWindDirection[0])
        json += linePrefix + "\t\"winddirection\" : \"" + String(mWindDirection) +"\",\n";
      else
        json += linePrefix + "\t\"winddirection\" : \"" STRINGNOTINITIALIZED "\",\n";

      if (mWindSpeedMpS!=UNDEFINEDVALUE)
        json += linePrefix + "\t\"windspeed\" : " + String(mWindSpeedMpS, 1) +"\n";
      else
        json += linePrefix + "\t\"windspeed\" : \"" STRINGNOTINITIALIZED "\"\n";
#endif

      json += linePrefix + "}";

      return json;
    }

  protected:

    byte *magicByteAddr() {
      return &mMagicByte;
    }

    uint16_t *crc16Addr() {
      return &mCRC16;
    }
};

#endif // _WEATHERPACKET_H_
