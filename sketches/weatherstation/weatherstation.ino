
//  system libraries
#include <limits.h>
#include <math.h>

//  temperature/humidity/pressure sensor (LaskaKit outdoor meteo THP: SHT40 + BMP280)
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h>

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
    - wind direction and wind speed are each read from their own AS5600 magnetic encoder via its
      PWM OUT pin (no I2C); direction is polled once per report interval, speed is sampled
      continuously in the background and averaged once per report interval
    - the rain gauge uses a single reed contact, a pulse is notified by a GPIO interrupt, a counter
      is increased, and the result is sent once per report interval
    - temperature/humidity and barometric pressure are measured using a LaskaKit outdoor meteo THP
      board (Sensirion SHT40 + Bosch BMP280), polled once per report interval
    - OPEN: luminescence
    - OPEN: ground humidity (Bodenfeuchte)

 ****************************************************************************************************/

#if USE_WIND_AS5600

//  reads the AS5600's PWM duty cycle on the given pin and converts it to a raw angle (0..4095);
//  returns false if no valid PWM signal was detected (e.g. sensor not connected)
static bool readAS5600PwmAngle(int pin, int *rawAngle) {
  unsigned long highMicros = pulseIn(pin, HIGH, AS5600_PWM_PULSE_TIMEOUT_US);
  unsigned long lowMicros = pulseIn(pin, LOW, AS5600_PWM_PULSE_TIMEOUT_US);

  if (highMicros==0||lowMicros==0)
    return false; // timed out, no signal

  float dutyPercent = 100.0f*highMicros/(highMicros+lowMicros);
  if (dutyPercent<AS5600_PWM_MIN_DUTY_PERCENT)
    dutyPercent = AS5600_PWM_MIN_DUTY_PERCENT;
  if (dutyPercent>AS5600_PWM_MAX_DUTY_PERCENT)
    dutyPercent = AS5600_PWM_MAX_DUTY_PERCENT;

  *rawAngle = (int) ((dutyPercent-AS5600_PWM_MIN_DUTY_PERCENT)/(AS5600_PWM_MAX_DUTY_PERCENT-AS5600_PWM_MIN_DUTY_PERCENT)*4095.0f+0.5f);
  return true;
}

//  wind vane / direction
static void propagateWindDirection(WeatherReport &report) {

#if DEBUG
  static bool reported = false;
  if (!reported) {
    startSerial();
    Serial.println("retrieving wind vane data...");
    reported = true;
  }
#endif // DEBUG

  int rawAngle;
  if (readAS5600PwmAngle(WIND_VANE_PIN, &rawAngle))
    report.setWindDirectionDegrees(rawAngle*360.0f/4096.0f);
#if DEBUG
  else {
    static bool notFoundReported = false;
    if (!notFoundReported) {
      startSerial();
      Serial.println("no wind direction PWM signal found...");
      notFoundReported = true;
    }
  }
#endif // DEBUG
}

#endif // USE_WIND_AS5600

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

//  temperature/humidity/barometric pressure: LaskaKit outdoor meteo THP board
//  (Sensirion SHT40 for temperature+humidity, Bosch BMP280 for pressure), both on the
//  shared I2C bus (SDA_PIN/SCL_PIN)
static Adafruit_SHT4x sht4;
static Adafruit_BMP280 bmp;
static bool temperatureSensorsInitialized = false;

static void propagateTemperatureEtAll(WeatherReport &report) {

#if DEBUG
  static bool reported = false;
  if (!reported) {
    startSerial();
    Serial.println("retrieving SHT40/BMP280 data...");
    reported = true;
  }
#endif

  if (!temperatureSensorsInitialized) {
    temperatureSensorsInitialized = sht4.begin(&Wire) && bmp.begin(0x76);
    if (temperatureSensorsInitialized) {
      sht4.setPrecision(SHT4X_HIGH_PRECISION);
      sht4.setHeater(SHT4X_NO_HEATER);
    }
#if DEBUG
    else {
      startSerial();
      Serial.println("no temperature sensor found...");
    }
#endif
  }

  if (temperatureSensorsInitialized) {
    int numRetries = 10;

    do {
      sensors_event_t humidityEvent, temperatureEvent;
      sht4.getEvent(&humidityEvent, &temperatureEvent);
      float pressure = bmp.readPressure();

      if (!isnan(temperatureEvent.temperature) && !isnan(pressure) && !isnan(humidityEvent.relative_humidity)) {
        report.addTemperature(temperatureEvent.temperature, pressure/100.0f, humidityEvent.relative_humidity);
        break;
      }

      numRetries--;
    } while (numRetries);
  }
}

#endif // USE_TEMPERATURE

static unsigned long startSampling; // initialized  in setup()

#if USE_WIND_AS5600

//  wind speed is derived from the AS5600's continuously accumulated rotation rather than a
//  pulse count, since the PWM angle read gives an absolute position instead of a per-turn pulse;
//  call this frequently from loop() (not just once per report) so full rotations aren't missed
static float windSpeedAccumulatedDegrees = 0;
static int lastWindSpeedRawAngle = -1; // -1 = not sampled yet
static unsigned long lastWindSpeedSampleMillis = 0;
#define WINDSPEED_SAMPLE_INTERVAL_MS 75

static void sampleWindSpeedAS5600() {
  unsigned long now = millis();
  if (now-lastWindSpeedSampleMillis<WINDSPEED_SAMPLE_INTERVAL_MS)
    return;
  lastWindSpeedSampleMillis = now;

  int rawAngle;
  if (!readAS5600PwmAngle(WINDSPEED_PIN, &rawAngle))
    return; // no signal this sample, skip

  if (lastWindSpeedRawAngle>=0) {
    int delta = rawAngle-lastWindSpeedRawAngle;
    //  handle wraparound at the 0/4095 boundary, taking the shorter path
    if (delta>2048)
      delta -= 4096;
    else if (delta<-2048)
      delta += 4096;

    windSpeedAccumulatedDegrees += abs(delta)*360.0f/4096.0f;
  }

  lastWindSpeedRawAngle = rawAngle;
}

static void propagateWindSpeed(WeatherReport &report) {
  unsigned long speedSampleTime = millis();
  float secondsPassed = (speedSampleTime-startSampling)/1000.0f;

  if (secondsPassed>=1.0f) {
    //  at least one second sampled, derive wind speed from accumulated rotations

    //  derive wind speed measured from number of rotations: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5948875/
    float rotationsPerSecond = (windSpeedAccumulatedDegrees/360.0f)/secondsPassed;
    float windSpeedMpS = DEFAULT_WINDSPEED_FACTOR*rotationsPerSecond;

    Serial.print("wind speed measured at ");
    Serial.print(DEFAULT_MEASUREMENT_HEIGHT, 1);
    Serial.print(" m: ");
    Serial.print(windSpeedMpS, 1);
    Serial.println(" m/s");

    windSpeedAccumulatedDegrees = 0; // reset

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

#endif // USE_WIND_AS5600

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

#if USE_WIND_AS5600
  //  setup wind vane and anemometer PWM input pins (AS5600 OUT pins)
  pinMode(WIND_VANE_PIN, INPUT);
  pinMode(WINDSPEED_PIN, INPUT);
#endif // USE_WIND_AS5600

#if USE_TEMPERATURE
  Wire.begin(SDA_PIN, SCL_PIN);
#endif // USE_TEMPERATURE

#if DEBUG
  Serial.println("finished setup, continuing to loop()");
#endif // DEBUG

  //  set starting millis for the current report accumulation window
  startSampling = millis();
}

void loop() {

#if USE_WIND_AS5600
  //  keep integrating wind speed rotation continuously, not just once per report
  sampleWindSpeedAS5600();
#endif // USE_WIND_AS5600

  //  sample all sensors and send a report once per DEFAULT_SECONDS_BETWEEN_REPORTS interval;
  //  running continuously (no deep sleep) means wind speed is integrated across the whole
  //  interval rather than just a brief post-wakeup window
  if (millis()-startSampling>DEFAULT_SECONDS_BETWEEN_REPORTS*MS2S_FACTOR) {

#if USE_WIND_AS5600
    propagateWindSpeed(report);
    propagateWindDirection(report);
#endif // USE_WIND_AS5600

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
