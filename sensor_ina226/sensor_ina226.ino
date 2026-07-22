// #include <WebServer.h>
// #include <ElegantOTA.h>
// #include <Wire.h>
#include <INA226_WE.h>

// const char* ssid = "Aldo";
// const char* password = "jangandihabisin";

// WebServer server(5000);
INA226_WE ina226(0x44);

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7);

  if (!ina226.init()) {
    Serial.println("INA226 tidak ditemukan");
    while (1);
  }
  Serial.println("INA226 siap");

  // Serial.print("Menghubungkan WiFi");
  // WiFi.begin(ssid, password);
  // while(WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // Serial.println();
  // Serial.print("IP: ");
  // Serial.println(WiFi.localIP());

  // server.on("/", []() {
  //   server.send(200, "text/plain", "ESP32 - INA226 & NO2\n\n" "Upload OTA: /update");
  // });

  // ElegantOTA.begin(&server);
  // server.begin();

  // Serial.println("Web Server Started");
  // Serial.println("OTA Ready");
}

void loop() {
  // server.handleClient();

  Serial.print("Current: ");
  Serial.print(ina226.getCurrent_mA());
  Serial.println(" mA");
  delay(500);
}
