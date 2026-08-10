/*
 * Reading E32 Configuration with Auto-Scan & Buffer Clear
 * Hardware: ESP32-C6
 *
 * Wiring:
 * M0   ----- 3.3V
 * M1   ----- 3.3V
 * TX   ----- GPIO 4 (RX ESP32-C6)
 * RX   ----- GPIO 5 (TX ESP32-C6)
 * VCC  ----- 5V
 * GND  ----- GND
 */

#include <HardwareSerial.h>
#define RX 4
#define TX 5

HardwareSerial E32(1);

void setup() {
  Serial.begin(115200);
  E32.begin(9600, SERIAL_8N1, RX, TX);
  delay(500);
  Serial.println("Membaca konfigurasi E32...");

  byte cmd[] = {0xC1, 0xC1, 0xC1};
  E32.write(cmd, 3);
  delay(200);

  if (E32.available() >= 6) {
    byte head   = E32.read();
    byte addh   = E32.read();
    byte addl   = E32.read();
    byte sped   = E32.read();
    byte chan   = E32.read();
    byte option = E32.read();

    Serial.println("\n===== HASIL =====");

    Serial.print("HEAD    : 0x");
    Serial.println(head, HEX);

    Serial.print("ADDH    : ");
    Serial.println(addh);

    Serial.print("ADDL    : ");
    Serial.println(addl);

    Serial.print("SPED    : 0x");
    Serial.println(sped, HEX);

    Serial.print("CHANNEL : ");
    Serial.println(chan);

    Serial.print("OPTION  : 0x");
    Serial.println(option, HEX);

  } else {
    Serial.println("Modul tidak merespon.");
  }
}

void loop() {
}