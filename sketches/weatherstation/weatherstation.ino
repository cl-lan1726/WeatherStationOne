
//  system libraries
#include <limits.h>
#include <math.h>

//  temperature sensor
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

//  project
#include "StationConfig.h"
#include "WeatherReport.h"

/****************************************************************************************************
  rain gauge
 ****************************************************************************************************/

static volatile unsigned int numRainBuckets = 0;
static volatile bool rainBucketOperational = true; // bucket is *not* horizontal permanently
static unsigned int lastNumRainBucketsReported = 0;

/****************************************************************************************************
  utility functions
 ****************************************************************************************************/

//  start serial reporting
static bool serialStarted = false;

static void startSerial() {
  if (!serialStarted) {
   //  start serial monitor connection
    Serial.begin(115200);
    delay(500);
    serialStarted = true;
  }
}

/****************************************************************************************************
  sensor handling functions

  overview:
    - the wind vane uses 8 reed contacts, its state is polled once per report interval; the 8 contacts
      are available multiplexed by three bits
    - the anemometer uses a single reed contact, pulses are counted continuously by an interrupt and
      speed is derived once per report interval
    - the rain gauge uses a single reed contact, a pulse is notified by a GPIO interrupt, a counter
      is increased, and the result is sent once per report interval
    - temperature, barometric pressure, humidity are measure using a Bosch BME280, its state is polled
      once per report interval
    - OPEN: luminescence
    - OPEN: ground humidity (Bodenfeuchte)

 ****************************************************************************************************/

//  wind vane / direction
#if USE_WIND_AS5600
static void propagateWindDirection(WeatherReport &report) {

#if DEBUG
  static bool reported = false;
  if (!reported) {
    startSerial();
    Serial.println("retrieving wind vane data...");
    reported = true;
  }
#endif // DEBUG

  int rawValue = analogRead(WIND_VANE_PIN);

  //  while the AS5600 allows read outs in degrees using the I2C or
  //  PWM interfaces, we use the analog plus A2D interface. It is
  //  the default set for the chip and allows us to use the bigger
  //  soldering points; the mapping is good enough to derive one of
  //  the 16 directions

  //  map 22.5 degree segments starting with "N" to raw values
  //  this mapping works around the non-linearity of A2D conversion
  //  in addition, it minimized the "blind" spot between 4095 / 0
  //  the best way possible
  static int raw4direction[] =
    {
      4095, 0, 197, 460, 704, 951, 1223, 1484,
      1743, 1936, 2186, 2410, 2732, 3020, 3363, 3744
    };

  //  find best match according to raw value
  int best_i = 0;
  int best_diff = 4096;
  for (int i = 0; i<16; i++) {
    int diff = 2048 - abs(abs(raw4direction[i]-rawValue)%4096 - 2048);
    if (diff<best_diff) {
      best_diff = diff;
      best_i = i;
    }
  }

  //  set result
  static const char *directions[] =
    {
      "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };

  report.setWindDirection(directions[best_i]);
}
#endif // USE_WIND_AS5600

#if USE_WIND_REED
static void propagateWindDirection(WeatherReport &report) {

#if DEBUG
  static bool reported = false;
  if (!reported) {
    startSerial();
    Serial.println("retrieving wind vane data...");
    reported = true;
  }
#endif // DEBUG

  static struct {
    const char *windDirection;
    uint8_t pattern[2];
  } windDirectionPattern [] = {
    //  single bit, two bit, three and four bit pattern
    //  to cope with different reed and magnet characteristics
    { "N",    { 0b00000001, 0b10000011 } },
    { "NNE",  { 0b00000011, 0b10000111 } },
    { "NE",   { 0b00000010, 0b00000111 } },
    { "ENE",  { 0b00000110, 0b00001111 } },
    { "E",    { 0b00000100, 0b00001110 } },
    { "ESE",  { 0b00001100, 0b00011110 } },
    { "SE",   { 0b00001000, 0b00011100 } },
    { "SSE",  { 0b00011000, 0b00111100 } },
    { "S",    { 0b00010000, 0b00111000 } },
    { "SSW",  { 0b00110000, 0b01111000 } },
    { "SW",   { 0b00100000, 0b01110000 } },
    { "WSW",  { 0b01100000, 0b11110000 } },
    { "W",    { 0b01000000, 0b11100000 } },
    { "WNW",  { 0b11000000, 0b11100001 } },
    { "NW",   { 0b10000000, 0b11000001 } },
    { "NNW",  { 0b10000001, 0b11000011 } }
  };

  //  collect states
  const char *result = NULL;
  uint8_t directions = 0;

#if DEBUG
  Serial.print("active directions: ");
#endif // DEBUG

  for (int i = 0; i<8; i++) {
    //  select one of 8 vane contacts
    digitalWrite(WIND_VANE_S0, i&0b00000001?HIGH:LOW);
    digitalWrite(WIND_VANE_S1, i&0b00000010?HIGH:LOW);
    digitalWrite(WIND_VANE_S2, i&0b00000100?HIGH:LOW);
    delay(10); // this fixes issues with wrong digitalRead() results below

    //  read and store in directions
    if (digitalRead(WIND_VANE_Z)) {
      directions = directions|(0b1<<i);
#if DEBUG
      Serial.print(windDirectionPattern[i*2].windDirection);
      Serial.print(" ");
#endif
    }
  }

  //  Check for all pin pattern...
  for (int i=0; i<16; i++) {
    for (int j=0; j<2; j++) {
      uint8_t pattern = windDirectionPattern[i].pattern[j];
      if (directions==pattern)
        result = windDirectionPattern[i].windDirection;
    }
  }

#if DEBUG
  Serial.print("-> ");
  Serial.println(result);
#endif

  if (result)
    report.setWindDirection(result);
  else {
    static bool reported = false;
    if (!reported) {
      startSerial();
      if (!directions)
        Serial.println("no main wind vane found");
      else
        Serial.println("invalid vane pattern found");
      reported = true;
    }
  }
}
#endif // USE_WIND_REED

#if USE_RAIN
const double gaugeDiameter = 106; // mm
const double gaugeArea = M_PI*(gaugeDiameter/2)*(gaugeDiameter/2); // mm2

#define RAIN_DEBOUNCE_MS 50
static void IRAM_ATTR handleRainState() {
  //  called by GPIO interrupt on rain pulse; debounce since reed contacts bounce
  static unsigned long lastInterruptMillis = 0;
  unsigned long now = millis();
  if (now-lastInterruptMillis<RAIN_DEBOUNCE_MS)
    return;
  lastInterruptMillis = now;

  if (rainBucketOperational)
    numRainBuckets++;
}

static void propagateRain(WeatherReport &report) {

  //  sanity check - the rain bucket pin should be LOW here; in case it is not
  //  the bucket is probably in horizontal position
  rainBucketOperational = digitalRead(RAIN_PIN)==LOW;

#if DEBUG
  if (rainBucketOperational)
    Serial.println("bucket o.k., pulse collection operational");
  else
    Serial.println("bucket in horizontal position, disabling pulse collection");
#endif // DEBUG

  //  to calculate mm from buckets
  unsigned int numDeltaBuckets = 0;

  if (numRainBuckets>lastNumRainBucketsReported)
    // in case a value has been reset...
    numDeltaBuckets = numRainBuckets-lastNumRainBucketsReported;

  double deltaRainMM = numDeltaBuckets*DEFAULT_BUCKET_TRIGGER_VOLUME/gaugeArea;

#if DEBUG
  if (numDeltaBuckets>0) {
    Serial.print("adding ");
    Serial.print(numDeltaBuckets);
    Serial.print(" buckets equaling ");
    Serial.print(deltaRainMM, 2);
    Serial.println("mm rain");
  }
#endif // DEBUG

  //  rainMM is the rain in mm we got since last time propagateRain has been called
  report.setDeltaRain(deltaRainMM);

  //  memorize current rain buckets to allow a delta calculation for the next call
  lastNumRainBucketsReported = numRainBuckets;
}

#endif // USE_RAIN

#if USE_TEMPERATURE

//  temperature/barometric/humidity sensor
static void propagateTemperatureEtAll(WeatherReport &report) {

#if DEBUG
  static bool reported = false;
  if (!reported) {
    startSerial();
    Serial.println("retrieving BME280 data...");
    reported = true;
  }
#endif

  Adafruit_BME280 bme;
  if (!bme.begin(0x76)) {
#if DEBUG
    static bool notFoundReported = false;
    if (!notFoundReported) {
      startSerial();
      Serial.println("no temperature sensor found...");
      notFoundReported = true;
    }
#endif
  } else {
    int numRetries = 10;

    do {
      float temperature = bme.readTemperature();
      float pressure = bme.readPressure();
      float humidity = bme.readHumidity();

      if (temperature!=NAN && pressure!=NAN && humidity!=NAN) {
        report.addTemperature(temperature, pressure/100.0f, humidity);
        break;
      }

      numRetries--;
    } while (numRetries);
  }
}

#endif // USE_TEMPERATURE

static unsigned long startSampling; // initialized  in setup()
static int windSpeedCounts = 0;
static void handleWindSpeed() {
  windSpeedCounts++;
#if DEBUG
  Serial.print("increased wind speed count to ");
  Serial.println(windSpeedCounts);
#endif // DEBUG
}

#if USE_WIND_REED||USE_WIND_AS5600

static void propagateWindSpeed(WeatherReport &report) {
  unsigned long speedSampleTime = millis();
  float secondsPassed = (speedSampleTime-startSampling)/1000.0f;

  if (secondsPassed>=1.0f) {
    //  at least one second sampled, derive wind speed

    //  derive wind speed measured from number of rotations: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5948875/
    float windSpeedMpS = DEFAULT_WINDSPEED_FACTOR*windSpeedCounts/NUM_COUNTS_PER_TURN/secondsPassed;

    Serial.print("wind speed measured at ");
    Serial.print(DEFAULT_MEASUREMENT_HEIGHT, 1);
    Serial.print(" m: ");
    Serial.print(windSpeedMpS, 1);
    Serial.println(" m/s");

    windSpeedCounts = 0; // reset

    //  measurements made on a height (reference) different to height 10m need a compensation:
    //    v(h) = vref/ln(href/z0)*ln(h/z0)
    //  with
    //    vref = windSpeedMpS
    //    href = DEFAULT_MEASUREMENT_HEIGHT
    //    z0 =
    //    h = 10.0

    const float z0 = 0.1; // https://www.igwindkraft.at/kinder/windkurs/windpowerweb/de/stat/unitsw.htm#roughness
    windSpeedMpS = windSpeedMpS/log(DEFAULT_MEASUREMENT_HEIGHT/z0)*log(10.0/z0);

    Serial.print("wind speed at 10 meter height: ");
    Serial.print(windSpeedMpS, 1);
    Serial.println(" m/s");

    report.setWindSpeed(windSpeedMpS);
  } else
    report.setWindSpeed(0);
}

#endif // USE_WIND_REED||USE_WIND_AS5600

/****************************************************************************************************
  main functions
 ****************************************************************************************************/

WeatherReport report;

void setup() {

#if DEBUG
  startSerial();
#endif // DEBUG

  //  configure signaling LEDs
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

#if USE_RAIN
  //  configure rain pin and attach a real interrupt (always-on, no more deep sleep wakeup)
  pinMode(RAIN_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), handleRainState, RISING);
#endif // USE_RAIN

  //  setup wind vane
 #if USE_WIND_REED
  pinMode(WIND_VANE_S0, OUTPUT);
  pinMode(WIND_VANE_S1, OUTPUT);
  pinMode(WIND_VANE_S2, OUTPUT);
  pinMode(WIND_VANE_Z, INPUT_PULLDOWN);
#endif // USE_WIND_REED

#if USE_WIND_REED||USE_WIND_AS5600
  //  setup anemometer
  pinMode(WINDSPEED_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(WINDSPEED_PIN), handleWindSpeed, RISING);
#endif // USE_WIND_REED||USE_WIND_AS5600

#if DEBUG
  Serial.println("finished setup, continuing to loop()");
#endif // DEBUG

  //  set starting millis for the current report accumulation window
  startSampling = millis();
}

void loop() {

  //  sample all sensors and send a report once per DEFAULT_SECONDS_BETWEEN_REPORTS interval;
  //  running continuously (no deep sleep) means the wind speed interrupt counts pulses across
  //  the whole interval rather than just a brief post-wakeup window
  if (millis()-startSampling>DEFAULT_SECONDS_BETWEEN_REPORTS*MS2S_FACTOR) {

#if USE_WIND_AS5600||USE_WIND_REED
    propagateWindSpeed(report);
    propagateWindDirection(report);
#endif // USE_WIND_AS5600||USE_WIND_REED

#if USE_TEMPERATURE
    propagateTemperatureEtAll(report);
#endif // USE_TEMPERATURE

#if USE_RAIN
    propagateRain(report);
#endif // USE_RAIN

    //  send report...
    report.send();

    digitalWrite(LED_PIN, LOW); //  turn LED off

    //  start a fresh report accumulation window (also resets low-pass filtered values);
    //  keeps the WiFi/MQTT connection alive rather than reconnecting every interval
    report.reset();
    startSampling = millis();
  }

  delay(100);
}
