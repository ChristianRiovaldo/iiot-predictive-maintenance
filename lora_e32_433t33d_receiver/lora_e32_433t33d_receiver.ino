#include <Arduino.h>
#include <ArduinoJson.h>

HardwareSerial loraSerial(1); 

#define RXD_PIN 4  // Terhubung ke TXD E32
#define TXD_PIN 5  // Terhubung ke RXD E32

void setup() {
  Serial.begin(115200); 
  delay(1000);

  loraSerial.begin(9600, SERIAL_8N1, RXD_PIN, TXD_PIN);

  Serial.println("--- RX JSON Dummy Sensor Ready ---");
}

void loop() {
  if (loraSerial.available()) {
    // Baca string sampai menemukan \n
    String incomingJson = loraSerial.readStringUntil('\n');
    
    // Alokasikan memori dokumen JSON
    JsonDocument doc;

    // Deserialisasi (Urai) String JSON
    DeserializationError error = deserializeJson(doc, incomingJson);

    if (error) {
      Serial.print("[RX Error] Gagal parse JSON: ");
      Serial.println(error.f_str());
      return;
    }

    float suhu = doc["suhu"];
    float kelembaban = doc["kelembaban"];
    int jarak = doc["jarak"];
    float no2 = doc["no2"];
    float barometer = doc["barometer"];

    // Tampilkan hasil pembacaan
    Serial.println("\n========== DATA SENSOR DITERIMA ==========");
    Serial.print("Suhu       : "); Serial.print(suhu); Serial.println(" °C");
    Serial.print("Kelembaban : "); Serial.print(kelembaban); Serial.println(" %");
    Serial.print("Jarak      : "); Serial.print(jarak); Serial.println(" cm");
    Serial.print("NO2        : "); Serial.print(no2, 3); Serial.println(" ppm");
    Serial.print("Barometer  : "); Serial.print(barometer); Serial.println(" hPa");
    Serial.println("==========================================");
  }
}