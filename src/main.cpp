

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <addons/TokenHelper.h>

// WiFi credentials
const char* ssid = "UW MPSK";
const char* password = "}EnHv76D%V";

// Firebase project credentials
#define DATABASE_URL "https://techin514-final-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyBv3VISnijb9p1HwbtUMjp8YBggXMDjT9I"
#define USER_EMAIL "wyf_testing1@gmail.com"
#define USER_PASSWORD "123456"

// Firebase Data object
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// MPU6050 sensor instance
Adafruit_MPU6050 mpu;

unsigned long lastDataTime = 0; // Time when the last data was uploaded to Firebase
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

  // Set up Firebase config
  config.database_url = DATABASE_URL;
  config.api_key = API_KEY;
  config.token_status_callback = tokenStatusCallback;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Check for errors in Firebase connection using firebaseData
  if (Firebase.ready()) {
    Serial.println("Firebase connected");
  } else {
    Serial.print("Firebase connection failed: ");
    Serial.println(firebaseData.errorReason());
  }

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

  // Every 10 seconds, upload data to Firebase
  if (millis() - lastDataTime > dataInterval) {
    int stepFrequency = stepCount * 2; // Steps per minute
    float speed = (stepFrequency * stepStride / 1000.0) * 60.0; // Speed in km/h

    Serial.print("Step Frequency: ");
    Serial.print(stepFrequency);
    Serial.println(" steps/minute");
    Serial.print("Speed: ");
    Serial.print(speed);
    Serial.println(" km/h");

    if (Firebase.RTDB.setFloat(&firebaseData, "/walkingData/stepFrequency", stepFrequency) &&
        Firebase.RTDB.setFloat(&firebaseData, "/walkingData/speed", speed)) {
      Serial.println("Data uploaded successfully");
    } else {
      Serial.print("Failed to upload data: ");
      Serial.println(firebaseData.errorReason());
    }

    stepCount = 0; // Reset step count after uploading
    lastDataTime = millis(); // Update last data upload time
  }

  delay(1000); // Loop interval
}
