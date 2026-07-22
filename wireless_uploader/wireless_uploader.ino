#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "Aldo";
const char* password = "jangandihabisin";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("ESP32_OTA");
  ArduinoOTA.setPassword("iiot2026");
  ArduinoOTA.begin();

  Serial.println("OTA Ready");
}

void loop() {
  ArduinoOTA.handle();
  Serial.println("Hello World 1");
  delay(1000)
}