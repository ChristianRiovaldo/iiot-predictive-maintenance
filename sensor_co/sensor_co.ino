#include "DFRobot_MultiGasSensor.h"
#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7

DFRobot_GAS_I2C gas(&Wire, 0x74);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);

  while(!gas.begin()) {
    Serial.println("Sensor tidak ditemukan!");
    delay(1000);
  }
  Serial.println("Sensor terhubung!");
}

void loop() {
  float ppm = gas.readGasConcentrationPPM();
  float temp = gas.readTempC();
  // float voltage = gas.readVolatageData();

  Serial.print("CO : ");
  Serial.print(ppm);
  Serial.println(" ppm");

  Serial.print("Temp : ");
  Serial.print(temp);
  Serial.println(" C");

  // Serial.print("Voltage : ");
  // Serial.print(voltage);
  // Serial.println(" mV");

  Serial.println("------------------");
  delay(1000);
}