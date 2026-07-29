#include <DFRobot_OzoneSensor.h>
#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define Ozone_IICAddress 0x73
#define SAMPLE_NUMBER 20

DFRobot_OzoneSensor Ozone;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Inisialisasi sensor Ozon...");

  while (!Ozone.begin(Ozone_IICAddress)) {
    Serial.println("Koneksi I2C gagal! Periksa kabel/alamat.");
    delay(1000);
  }
  Serial.println("Sensor Ozon berhasil terhubung!");
  Ozone.setModes(MEASURE_MODE_PASSIVE);
}

void loop() {
  int16_t ozoneConcentration = Ozone.readOzoneData(SAMPLE_NUMBER);
  Serial.print("Konsentrasi Ozon: ");
  Serial.print(ozoneConcentration);
  Serial.println(" PPB");
  delay(1000);
}