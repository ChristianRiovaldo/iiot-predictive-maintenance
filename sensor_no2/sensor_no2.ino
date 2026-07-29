// // Board: ESP32C6 Dev Module
// #define NO2_PIN 2

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
// }

// void loop() {
//   int raw = analogRead(NO2_PIN);
//   float voltage = (raw / 4095.0) * 3.3;

//   Serial.print("RAW ADC: ");
//   Serial.print(raw);
//   Serial.print(" | Voltage: ");
//   Serial.print(voltage, 3);
//   Serial.println(" V");

//   delay(1000);
// }

#include <WiFi.h>
#include <PubSubClient.h>
#define NO2_PIN 2

const char* ssid = "Aldo";
const char* password = "jangandihabisin";
const char* mqtt_server = "10.71.250.145";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {

    String clientId = "ESP32C6-NO2-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("Connecting MQTT... ");

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  analogReadResolution(12);
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();
  
  int raw = analogRead(NO2_PIN);
  float voltage = (raw / 4095.0) * 3.3;
  char payload[128];

  snprintf(
    payload,
    sizeof(payload),
    "{\"raw\":%d,\"voltage\":%.3f}",
    raw,
    voltage
  );

  client.publish("sensor/no2", payload);

  Serial.print("RAW ADC: ");
  Serial.print(raw);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | Published: ");
  Serial.println(payload);

  delay(1000);
}