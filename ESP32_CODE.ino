#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Adafruit_BMP280 bmp; // I2C

#define WIFI_SSID "Taha"
#define WIFI_PASSWORD "87654321"
#define API_KEY "AIzaSyArTMrG0-1YHcttD5902oU4dM-jsSMkol4"
#define DATABASE_URL "https://datasample-5e3cd-default-rtdb.firebaseio.com/"
#define SAMPLING_INTERVAL 100 // in milliseconds, adjust based on your needs
#define NUM_SAMPLES 50 // number of samples to average for a heartbeat
 int i;

float heartbeat;
float skinTemp0;
float coreTemp0;
float ambientTemp0;
float heartRate0;
#define coreTemp 36   

#define skinTemp 39   

#define ambTemp 34   

#define DHTTYPE DHT11


float pressureReadings[NUM_SAMPLES];
int sampleIndex = 0;
unsigned long lastSampleTime = 0;


DHT TempCore(coreTemp, DHTTYPE);
DHT TempSkin(skinTemp, DHTTYPE);
DHT TempAmb(ambTemp, DHTTYPE);

void setupWiFi() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
   
    Serial.println("Connecting to WiFi");
    delay(500);
  
    Serial.print(".");
    delay(500);
    i++;
    if (i == 50) {
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected!");
 
  Serial.print("WiFi connected!");
  delay(2000);
}

void setupFirebase() {
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Authentication Successful");
  } else {
    Serial.println("Firebase Authentication Failed");
    i++;
    if (i == 50) {
      ESP.restart();
    }
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

float calculateAverage(float readings[], int numSamples) {
  float sum = 0;
  for (int i = 0; i < numSamples; i++) {
    sum += readings[i];
  }
  return sum / numSamples;
}

float calculateHeartbeat(float readings[], int numSamples) {
  // This is a very basic approach and may not be accurate
  // You should replace this with a proper algorithm to detect heartbeats
  int peaks = 0;
  for (int i = 1; i < numSamples - 1; i++) {
    if (readings[i] > readings[i - 1] && readings[i] > readings[i + 1]) {
      peaks++;
    }
  }
  // Convert peaks to bpm, assuming the sampling interval is 100ms
  float bpm = (peaks * 60000.0) / (SAMPLING_INTERVAL * numSamples);
  return bpm;
}

void setup() {
  setupWiFi();
  setupFirebase();
  TempCore.begin();
  TempSkin.begin();
  TempAmb.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  float SkinTemperature=TempSkin.readTemperature();
  float CoreTemperature=TempCore.readTemperature();
  float AmbientTemperature=TempAmb.readTemperature();
  if (millis() - lastSampleTime >= SAMPLING_INTERVAL) {
    lastSampleTime = millis();

    float pressure = bmp.readPressure();
    pressureReadings[sampleIndex] = pressure;
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

    if (sampleIndex == 0) {
      float avgPressure = calculateAverage(pressureReadings, NUM_SAMPLES);
      Serial.print("Average Pressure: ");
      Serial.print(avgPressure);
      Serial.println(" Pa");

       heartbeat = calculateHeartbeat(pressureReadings, NUM_SAMPLES);
      Serial.print("Estimated Heartbeat: ");
      Serial.print(heartbeat);
      Serial.println(" bpm");
    }
  }
  // String csvString = createCSV(SkinTemperature, CoreTemperature, AmbientTemperature, heartbeat);
   String skinTempStr = String(SkinTemperature, 2); // '2' indicates two decimal places
  String coreTempStr = String(CoreTemperature, 2);
  String ambientTempStr = String(AmbientTemperature, 2);
  String heartRateStr = String(heartbeat, 2);
  
  // Concatenate the strings with commas in between
  String csv = skinTempStr + "," + coreTempStr + "," + ambientTempStr + "," + heartRateStr;
  
    Firebase.RTDB.setString(&fbdo, "Data", csv);
  
}