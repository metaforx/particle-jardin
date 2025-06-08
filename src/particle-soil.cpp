#include "Particle.h"
#include "secrets.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);


const int AirValue = 3280;   
const int WaterValue = 1640;  
int soilMoistureValue = 0;
const int measureHours[] = {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}; // Hours to measure soil moisture (24-hour format)
const int TIMEZONE_OFFSET = 2;
int hoursToNext = 24;

void setup() {
  Serial.begin(9600);
  WiFi.setCredentials(WIFI_SSID, WIFI_PASSWORD);
  Particle.variable("soilMoistureValue", &soilMoistureValue, INT);
}

void loop() {
  waitUntil(Particle.connected);
  Particle.syncTime();
  waitUntil(Particle.syncTimeDone);

  int currentHour = (Time.hour() + TIMEZONE_OFFSET) % 24;
  bool shouldMeasure = false;

  for (size_t i = 0; i < sizeof(measureHours)/sizeof(measureHours[0]); i++) {
    if (currentHour == measureHours[i]) {
      shouldMeasure = true;
      break;
    }
  }

  if (shouldMeasure) {
    soilMoistureValue = analogRead(A0);

    // Prepare JSON payload as a string
    char data[128];
    snprintf(data, sizeof(data),
      "{\"value\":%d,\"sensor_id\":\"%s\",\"sensor_name\":\"%s\",\"sensor_type\":\"%s\"}",
      soilMoistureValue,
      Particle.deviceID().c_str(), 
      "jardin-soil-1",  
      "moisture"  
    );

    // Log the payload to serial
    Serial.println("Publishing JSON payload:");
    Serial.println(data);

    // Publish the JSON string for the webhook
    Particle.publish("jardin-data", data, PRIVATE);
    delay(10000); // Allow time for publish

  } else {
    Serial.printf("Skipping measurement at hour: %d\n", currentHour);
  }

  for (size_t i = 0; i < sizeof(measureHours)/sizeof(measureHours[0]); i++) {
      int diff = (measureHours[i] - currentHour + 24) % 24;
      if (diff > 0 && diff < hoursToNext) {
          hoursToNext = diff;
      }
  }
  if (hoursToNext == 24) hoursToNext = (measureHours[0] - currentHour + 24) % 24; // fallback

  int secondsToNextMeasurement = hoursToNext * 3600 - (Time.minute() * 60 + Time.second());
  if (secondsToNextMeasurement <= 0) secondsToNextMeasurement = 60; // Avoid zero or negative sleep

  Serial.printf("secondsToNextMeasurement: %d\n", secondsToNextMeasurement);
  System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::ULTRA_LOW_POWER).duration(secondsToNextMeasurement * 1s)); //sleep for s(econds)
}