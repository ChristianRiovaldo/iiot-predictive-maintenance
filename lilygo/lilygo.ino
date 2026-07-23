// BOARD: T-Beam
#include <Wire.h>
#include <XPowersLib.h>
#include <RadioLib.h>
#define AXP_SDA 21
#define AXP_SCL 22
#define LORA_CS 18
#define LORA_DIO0 26
#define LORA_RST 23
#define LORA_DIO1 33

XPowersAXP2101 PMU;
SX1278 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);
int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LilyGO T-Beam v1.2");

  Wire.begin(AXP_SDA, AXP_SCL);
  if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, AXP_SDA, AXP_SCL)) {
    Serial.println("I2C PMU AXP2101 tidak ditemukan!");
    while (true);
  }

  PMU.setALDO3Voltage(3300);
  PMU.enableALDO3();
  Serial.println("Daya LoRa Aktif!");

  Serial.print("Inisialisasi RadioLib 433 MHz\n");
  int state = radio.begin(433.0, 250.0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("BERHASIL!");
  } else {
    Serial.print("GAGAL! Kode Error: ");
    Serial.println(state);
    while (true);
  }
}

void loop() {
  String dataTransmit = "Data ke-" + String(counter);
  Serial.print("\nMengirim: ");
  Serial.println(dataTransmit);

  unsigned long startTime = millis();
  int state = radio.transmit(dataTransmit);
  unsigned long transmitTime = millis() - startTime;

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Status: Terkirim! (Waktu: ");
    Serial.print(transmitTime);
    Serial.println(" ms)");
  } else {
    Serial.print("Status: Gagal terkirim! Error: ");
    Serial.println(state);
  }

  counter++;
  delay(2000);
}