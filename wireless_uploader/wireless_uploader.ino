#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>

const char* ssid = "Aldo";
const char* password = "jangandihabisin";
WebServer server(80);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {server.send(200, "text/plain", "ESP32 uploader\n\n" "OTA : /update");});
  ElegantOTA.begin(&server);
  server.begin();

  Serial.println("Web Server Started & OTA Ready");
}

void loop() {
  server.handleClient();
  delay(1000);
}