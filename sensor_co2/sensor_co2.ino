#include <Arduino.h>
#include <MHZ19.h>

#define RXD2_PIN 4  // GPIO 4 terhubung ke TX sensor MH-Z19B
#define TXD2_PIN 5  // GPIO 5 terhubung ke RX sensor MH-Z19B

HardwareSerial mhzSerial(1);
MHZ19 myMHZ19;

unsigned long timerPembacaan = 0;

void setup() {
  Serial.begin(115200);
  mhzSerial.begin(9600, SERIAL_8N1, RXD2_PIN, TXD2_PIN);
  myMHZ19.begin(mhzSerial);
  myMHZ19.autoCalibration();
  Serial.println(F("Pemanasan sensor MH-Z19B... Mohon tunggu beberapa detik."));
}

void loop() {
  if (millis() - timerPembacaan >= 1000) {
    int co2 = myMHZ19.getCO2();
    int temp = myMHZ19.getTemperature();

    if (myMHZ19.errorCode == RESULT_OK) {
      Serial.print(F("Kadar CO2  : "));
      Serial.print(co2);
      Serial.println(F(" ppm"));

      Serial.print(F("Suhu Sensor: "));
      Serial.print(temp);
      Serial.println(F(" °C"));
      Serial.println(F("------------------------------"));
    } else {
      Serial.print(F("Gagal membaca sensor! Kode Eror: "));
      Serial.println(myMHZ19.errorCode);
    }

    timerPembacaan = millis();
  }
}