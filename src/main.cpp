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
#define WIFI_SSID "Kanec's S24 Ultra" // NOTE: Please delete this value before submitting assignment
#define WIFI_PASSWORD "22440000" // // NOTE: Please delete this value before submitting assignment

// Azure IoT Hub configuration
#define SAS_TOKEN "SharedAccessSignature sr=btHub1.azure-devices.net%2Fdevices%2Fbtesp32&sig=tY8rXX1sSeP1%2Fo4NFhh10pNi3xBcZM2MhR1xxTWyfsw%3D&se=1757023071"
// Root CA certificate for Azure IoT Hub
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEtjCCA56gAwIBAgIQCv1eRG9c89YADp5Gwibf9jANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0yMjA0MjgwMDAwMDBaFw0zMjA0MjcyMzU5NTlaMEcxCzAJBgNVBAYTAlVT\n" \
"MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xGDAWBgNVBAMTD01TRlQg\n" \
"UlMyNTYgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMiJV34o\n" \
"eVNHI0mZGh1Rj9mdde3zSY7IhQNqAmRaTzOeRye8QsfhYFXSiMW25JddlcqaqGJ9\n" \
"GEMcJPWBIBIEdNVYl1bB5KQOl+3m68p59Pu7npC74lJRY8F+p8PLKZAJjSkDD9Ex\n" \
"mjHBlPcRrasgflPom3D0XB++nB1y+WLn+cB7DWLoj6qZSUDyWwnEDkkjfKee6ybx\n" \
"SAXq7oORPe9o2BKfgi7dTKlOd7eKhotw96yIgMx7yigE3Q3ARS8m+BOFZ/mx150g\n" \
"dKFfMcDNvSkCpxjVWnk//icrrmmEsn2xJbEuDCvtoSNvGIuCXxqhTM352HGfO2JK\n" \
"AF/Kjf5OrPn2QpECAwEAAaOCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYD\n" \
"VR0OBBYEFAyBfpQ5X8d3on8XFnk46DWWjn+UMB8GA1UdIwQYMBaAFE4iVCAYlebj\n" \
"buYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcD\n" \
"AQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8v\n" \
"b2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRp\n" \
"Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcnQwQgYDVR0fBDswOTA3\n" \
"oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v\n" \
"dEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwB\n" \
"AgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG9w0BAQsFAAOCAQEAdYWmf+AB\n" \
"klEQShTbhGPQmH1c9BfnEgUFMJsNpzo9dvRj1Uek+L9WfI3kBQn97oUtf25BQsfc\n" \
"kIIvTlE3WhA2Cg2yWLTVjH0Ny03dGsqoFYIypnuAwhOWUPHAu++vaUMcPUTUpQCb\n" \
"eC1h4YW4CCSTYN37D2Q555wxnni0elPj9O0pymWS8gZnsfoKjvoYi/qDPZw1/TSR\n" \
"penOgI6XjmlmPLBrk4LIw7P7PPg4uXUpCzzeybvARG/NIIkFv1eRYIbDF+bIkZbJ\n"\
"QFdB9BjjlA4ukAg2YkOyCiB8eXTBi2APaceh3+uBLIgLk8ysy52g2U3gP7Q26Jlg\n" \
"q/xKzj3O9hFh/g==\n" \
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