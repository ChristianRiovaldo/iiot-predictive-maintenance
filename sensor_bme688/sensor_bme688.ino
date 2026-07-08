#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#define SDA_PIN 6
#define SCL_PIN 7

Adafruit_BME680 bme;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  if (!bme.begin(0x76)) {
    Serial.println("BME688 tidak terdeteksi!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
}

void loop() {
  if (!bme.performReading()) {
    Serial.println("Gagal membaca sensor!");
    delay(1000);
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(bme.temperature);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(bme.humidity);
  Serial.println("%");

  Serial.print("Pressure: ");
  Serial.print(bme.pressure);
  Serial.println(" Pa");

  Serial.print("Resistansi Gas: ");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" kΩ");

  Serial.println("===========================");
  delay(1000);
}