#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <Update.h>
#include "DHT.h"

// === Pin Definitions ===
#define DHTPIN 23
#define DHTTYPE DHT22
#define MQ135PIN 34
#define LED_PIN 2  // Onboard LED
#define CURRENT_FIRMWARE_VERSION "0"

const char* versionCheckURL = "https://d340-111-68-109-251.ngrok-free.app/static/version.txt";

// === Server URLs ===
const char* serverDataURL = "https://d340-111-68-109-251.ngrok-free.app/api/data/";
const char* firmwareURL  = "https://d340-111-68-109-251.ngrok-free.app/static/firmware.ino.bin";

// === Global Objects & Variables ===
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastSend = 0;
const long interval = 10000;  

// OTA check timer
unsigned long lastOTACheck = 0;
const long otaInterval = 60000;  

// MQ-135 Calibration
const float RL_VALUE = 10.0;
const float ADC_RESOLUTION = 4095.0;
const float VREF = 3.3;
const float CLEAN_AIR_FACTOR = 3.6;
float R0 = 10.0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();

  WiFiManager wm;
  bool res = wm.autoConnect("ESP32-OTA-Setup", "password123");

  if (!res) {
    Serial.println(" WiFi Failed. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println(" Connected to WiFi");
  Serial.println(" IP: " + WiFi.localIP().toString());

  Serial.println(" Calibrating MQ-135 Sensor...");
  delay(5000);
  R0 = calibrateSensor();
  Serial.println(" Calibration Done. R0 = " + String(R0, 2));
}

void loop() {
  blinkLED();

  if (millis() - lastSend >= interval) {
    lastSend = millis();
    sendSensorData();
  }

  if (millis() - lastOTACheck >= otaInterval) {
    lastOTACheck = millis();
    httpOTAUpdate();
  }
}

// === Blink LED ===
void blinkLED() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  delay(2000);
}

// === OTA Firmware Update ===
void httpOTAUpdate() {
  Serial.println(" Checking for firmware update...");

  HTTPClient http;
  http.begin(versionCheckURL);
  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    String remoteVersion = http.getString();
    remoteVersion.trim();
    Serial.println(" Remote Version: " + remoteVersion);

    Serial.println(" Current Local Version: " + String(CURRENT_FIRMWARE_VERSION));

    if (remoteVersion != CURRENT_FIRMWARE_VERSION) {
      Serial.println(" Update available!");
      performOTA();
    } else {
      Serial.println(" Already up to date.");
    }
  } else {
    Serial.println(" Version check failed: " + String(code));
  }

  http.end();
}

void performOTA() {
  HTTPClient http;
  http.begin(firmwareURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    WiFiClient& client = http.getStream();

    if (!Update.begin(contentLength)) {
      Serial.println(" Not enough space for OTA");
      return;
    }

    size_t written = Update.writeStream(client);

    if (written == contentLength) {
      Serial.println(" OTA written: " + String(written) + " bytes");
    } else {
      Serial.println(" OTA write mismatch");
    }

    if (Update.end() && Update.isFinished()) {
      Serial.println(" OTA complete. Rebooting...");
      blinkLED();
      delay(1000);
      ESP.restart();
    } else {
      Serial.println(" OTA Error #" + String(Update.getError()));
    }
  } else {
    Serial.println(" OTA HTTP GET failed: " + String(httpCode));
  }

  http.end();
}

// === Send Sensor Data to Server ===
void sendSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int airRaw = analogRead(MQ135PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println(" Failed to read from DHT22");
    return;
  }

  float airPPM = getAirQualityPPM(airRaw);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverDataURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temperature\": " + String(temp, 1) +
                     ", \"humidity\": " + String(hum, 1) +
                     ", \"airQuality\": " + String(airPPM, 2) + "}";

    Serial.println(" Sending: " + payload);

    int code = http.POST(payload);

    if (code > 0) {
      Serial.println(" Response: " + http.getString());
    } else {
      Serial.println(" POST Failed: " + http.errorToString(code));
    }

    http.end();
  } else {
    Serial.println(" WiFi not connected");
  }
}

// === MQ-135 Calibration ===
float calibrateSensor() {
  const int samples = 100;
  float rsSum = 0.0;

  for (int i = 0; i < samples; i++) {
    int adc = analogRead(MQ135PIN);
    float voltage = (adc / ADC_RESOLUTION) * VREF;
    float Rs = (VREF * RL_VALUE / voltage) - RL_VALUE;
    rsSum += Rs;
    delay(100);
  }

  return (rsSum / samples) / CLEAN_AIR_FACTOR;
}

// === Compute PPM from MQ-135 ===
float getAirQualityPPM(int raw) {
  float voltage = (raw / ADC_RESOLUTION) * VREF;
  float Rs = (VREF * RL_VALUE / voltage) - RL_VALUE;
  float ratio = Rs / R0;
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  return ppm;
}
