//  project
#include <WeatherPacket.h>

#define TEMPERATURELOWPASS 0.1

//  this is the object all data is collected to
class WeatherReport {

  private:

    WeatherPacket mPacket;

  public:

    WeatherReport() {}

    //  sensor collects an amount of rain in mm
    void setDeltaRain(double rainMM) {
      mPacket.mDeltaRainMM = rainMM;
    }

    bool hasTemperature() {
      return mPacket.mTemperatureDegreeCelsius!=UNDEFINEDVALUE;
    }

    void addTemperature(float temperatureDegreeCelsius,
      float pressureHPA, float humidityPercent) {

      if (!hasTemperature())
        mPacket.mTemperatureDegreeCelsius = temperatureDegreeCelsius;
      else
        mPacket.mTemperatureDegreeCelsius = mPacket.mTemperatureDegreeCelsius*(1-TEMPERATURELOWPASS)+temperatureDegreeCelsius*TEMPERATURELOWPASS;

      if (mPacket.mPressureHPA==UNDEFINEDVALUE)
        mPacket.mPressureHPA = pressureHPA;
      else
        mPacket.mPressureHPA = mPacket.mPressureHPA*(1-TEMPERATURELOWPASS)+pressureHPA*TEMPERATURELOWPASS;

      if (mPacket.mHumidityPercent==UNDEFINEDVALUE)
        mPacket.mHumidityPercent = humidityPercent;
      else
        mPacket.mHumidityPercent = mPacket.mHumidityPercent*(1-TEMPERATURELOWPASS)+humidityPercent*TEMPERATURELOWPASS; 
    }

    void setWindSpeed(float windSpeedMpS) {
      mPacket.mWindSpeedMpS = windSpeedMpS;
    }

    bool hasWindDirection() {
      return *mPacket.mWindDirection!='\0';
    }

    void setWindDirection(const char *windDirection) {
      strcpy(mPacket.mWindDirection, windDirection);
    }

    //  TODO(PR #4): transmission is temporarily disabled while HC-12 is removed;
    //  MQTT publishing over WiFi replaces this in a follow-up PR.
    void send() {
      if (DEBUG) {
        Serial.println("report ready (transmission not yet implemented):");
        mPacket.print(&Serial);
      }
    }
};
