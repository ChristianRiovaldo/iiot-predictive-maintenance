#include <Arduino.h>
#include <ArduinoJson.h>

HardwareSerial loraSerial(1); 

#define RXD_PIN 4  // Terhubung ke TXD E32
#define TXD_PIN 5  // Terhubung ke RXD E32

void setup() {
  Serial.begin(115200); 
  delay(1000);

  loraSerial.begin(9600, SERIAL_8N1, RXD_PIN, TXD_PIN);
  Serial.println("--- TX JSON Dummy Sensor Ready ---");
}

void loop() {
  // 1. Buat data dummy sensor
  float suhu = 25.5 + (random(-20, 20) / 10.0);      // misal: 23.5 - 27.5 °C
  float kelembaban = 60.0 + (random(-50, 50) / 10.0); // misal: 55.0 - 65.0 %
  int jarak = random(10, 200);                       // misal: 10 - 200 cm
  float no2 = 0.02 + (random(0, 50) / 1000.0);       // misal: 0.020 - 0.070 ppm
  float barometer = 1013.25 + (random(-10, 10));     // misal: 1003 - 1023 hPa

  // 2. Buat dokumen JSON (ArduinoJson v7)
  JsonDocument doc;

  // 3. Isi data ke dokumen JSON
  doc["suhu"] = suhu;
  doc["kelembaban"] = kelembaban;
  doc["jarak"] = jarak;
  doc["no2"] = no2;
  doc["barometer"] = barometer;

  // 4. Konversi JSON ke String
  String jsonString;
  serializeJson(doc, jsonString);

  // 5. Kirim data JSON lewat LoRa (ditambah \n otomatis oleh println)
  loraSerial.println(jsonString);

  // Tampilkan di Serial Monitor lokal
  Serial.print("[TX] Data JSON Terkirim: ");
  Serial.println(jsonString);

  delay(2000); // Kirim setiap 3 detik
}