#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ElegantOTA.h>

WebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFiManager wm;
  bool res = wm.autoConnect("ESP32 Hotspot Config", "12345678");

  if (!res) {
    Serial.println("Gagal terhubung ke WiFi!");
  } else {
    Serial.println("Berhasil Terhubung ke WiFi!");
    Serial.print("IP Address ESP32: ");
    Serial.println(WiFi.localIP());
  }

  server.on("/", []() {
    server.send(200, "text/plain", "Halo! ESP32 Siap. Akses /update untuk ElegantOTA.");
  });

  ElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP Server & ElegantOTA Ready!");
}

void loop() {
  server.handleClient();  
  ElegantOTA.loop();
  Serial.println("Hello World");
  delay(1000);
}