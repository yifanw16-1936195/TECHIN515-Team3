#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// WiFi credentials
const char* ssid = "UW MPSK";
const char* password = "XXXXXX";

// Supabase configuration
const char* SUPABASE_URL = "https://rxckwzfakcepeoqmczqo.supabase.co";
const char* SUPABASE_KEY = "XXXXXX";

// MPU6050 sensor instance
Adafruit_MPU6050 mpu;

unsigned long lastDataTime = 0; // Time when the last data was uploaded to Supabase
const unsigned long dataInterval = 10000; // Data upload interval in milliseconds (now 10 seconds)
const float stepStride = 78.0; // Average stride length in cm

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

bool uploadToSupabase(int stepFrequency, float speed) {
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/step_frequency_data";
  
  http.begin(url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  // Create JSON payload
  String payload = "{\"step_frequency\":" + String(stepFrequency) + ",\"speed\":" + String(speed) + "}";

  // Send POST request
  int httpResponseCode = http.POST(payload);

  http.end();

  return httpResponseCode == 201;
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  static int stepCount = 0;
  static unsigned long lastStepTime = 0;

  float acceleration = sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);

  // Detect steps
  if (acceleration > 10.5 && millis() - lastStepTime > 250) {
    stepCount++;
    lastStepTime = millis();
  }

  // Every 10 seconds, upload data to Supabase
  if (millis() - lastDataTime > dataInterval) {
    int stepFrequency = stepCount * 2; // Steps per minute
    float speed = (stepFrequency * stepStride / 1000.0) * 60.0; // Speed in km/h

    Serial.print("Step Frequency: ");
    Serial.print(stepFrequency);
    Serial.println(" steps/minute");
    Serial.print("Speed: ");
    Serial.print(speed);
    Serial.println(" km/h");

    // Upload data to Supabase
    if (uploadToSupabase(stepFrequency, speed)) {
      Serial.println("Data uploaded successfully");
    } else {
      Serial.println("Failed to upload data");
    }

    stepCount = 0; // Reset step count after uploading
    lastDataTime = millis(); // Update last data upload time
  }

  delay(1000); // Loop interval
}