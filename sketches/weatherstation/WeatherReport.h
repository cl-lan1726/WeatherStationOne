//  project
#include <WeatherPacket.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "StationConfig.h"

#define TEMPERATURELOWPASS 0.1

//  this is the object all data is collected to
class WeatherReport {

  private:

    WeatherPacket mPacket;
    WiFiClient mWiFiClient;
    PubSubClient mMqttClient;

    void connectWiFi() {
      if (WiFi.status()==WL_CONNECTED)
        return;

      if (DEBUG)
        Serial.println("connecting to WiFi...");

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      while (WiFi.status()!=WL_CONNECTED) {
        delay(500);
        if (DEBUG)
          Serial.print(".");
      }

      if (DEBUG)
        Serial.println(" connected");
    }

    void connectMQTT() {
      mMqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);

      while (!mMqttClient.connected()) {
        if (DEBUG)
          Serial.println("connecting to MQTT broker...");

        if (!mMqttClient.connect(MQTT_CLIENT_ID)) {
          if (DEBUG) {
            Serial.print("MQTT connect failed, rc=");
            Serial.print(mMqttClient.state());
            Serial.println(", retrying in 2s");
          }
          delay(2000);
        }
      }
    }

  public:

    WeatherReport() : mMqttClient(mWiFiClient) {}

    //  resets the collected sensor data for a fresh report accumulation window,
    //  keeping the WiFi/MQTT connection alive across reports
    void reset() {
      mPacket = WeatherPacket();
    }

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

    void send() {
      connectWiFi();
      connectMQTT();

      String payload = mPacket.toJson();

      if (DEBUG) {
        Serial.println("publishing report...");
        mPacket.print(&Serial);
      }

      mMqttClient.publish(MQTT_TOPIC_DATA, payload.c_str());
      mMqttClient.loop();
    }
};
