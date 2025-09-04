#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "Wire.h"
#include <ArduinoJson.h>
#include <string>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"


#define RED_PIN 32
#define BUTTON_PIN 27

// Wi-Fi credentials
#define WIFI_SSID "" 
#define WIFI_PASSWORD ""

// Azure IoT Hub configuration
#define SAS_TOKEN ""
// Root CA certificate for Azure IoT Hub
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
" \
"-----END CERTIFICATE-----\n";



String iothubName = "btHub1"; //Your hub name (replace if needed)
String deviceName = "btesp32"; //Your device name (replace if needed)
String url = "https://" + iothubName + ".azure-devices.net/devices/" +
deviceName + "/messages/events?api-version=2021-04-12";

// Telemetry interval
#define TELEMETRY_INTERVAL 3000 // Send data every 3 seconds
uint8_t count = 0;
uint32_t lastTelemetryTime = 0;

// Speaker variables
AnalogAudioStream out;
BluetoothA2DPSink a2dp_sink(out);
volatile uint8_t g_volume_steps = 0;     // 0..127
float   g_volume_pct   = 0.0;  // 0.0..100.0



void setup_wifi(){
  Wire.begin();

  WiFi.mode(WIFI_STA);  
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  //Initialize DHT20 here

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void send_message(float volume){
    // Create JSON payload
  ArduinoJson::JsonDocument doc;
  doc["volume"] = volume;
  char buffer[256];
  serializeJson(doc, buffer, sizeof(buffer));

  // Send telemetry via HTTPS
  WiFiClientSecure client;
  client.setCACert(root_ca); // Set root CA certificate
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", SAS_TOKEN);
  int httpCode = http.POST(buffer);

  if (httpCode == 204) { // IoT Hub returns 204 No Content for successful telemetry
  Serial.println("Telemetry sent: " + String(buffer));
  } else {
  Serial.println("Failed to send telemetry. HTTP code: " + String(httpCode));
  }
  http.end();
}

void onVolumeChange(int v) {
    if (v < 0) v = 0;
    if (v > 127) v = 127;
    g_volume_steps = (uint8_t)v;
    g_volume_pct   = (v / 127.0f) * 100.0f;
    Serial.printf("Volume: %u (%.1f%%)\n", g_volume_steps, g_volume_pct);
    // send_message(0.0);
  }

void speaker_setup(){
    /*
    Wire Negative to ground
    Positive to GPIO 
    */
    a2dp_sink.set_volume(64);                  // 0..127 (~50% per AVRCP absolute volume)
    a2dp_sink.set_on_volumechange(onVolumeChange);
    a2dp_sink.start("J80");
    while (!a2dp_sink.is_connected()){
        Serial.print("waiting...");
        delay(1500);
    }
    Serial.println("\nConnected");

}
void setup() {
  Serial.begin(9600);
  delay(5000);
  pinMode(RED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  while (digitalRead(BUTTON_PIN) == HIGH){
    delay(1500);
    Serial.print("Sleeping");
  }
  Serial.print("\nTurned on!");
  setup_wifi();
  speaker_setup();
}

void warning_led(int ledOnOFF){
  digitalWrite(RED_PIN, ledOnOFF);
}

void loop(){
 int err = 0;

 if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
  lastTelemetryTime = millis();
  // Serial.println("Volume: ");
  // Serial.println(g_volume_pct);
  if (g_volume_pct > 70){
    warning_led(HIGH);
  }else{
    warning_led(LOW);
  }
 }
}
