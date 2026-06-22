// DFRobot MEMS NO2 Sensor SEN0547 (Analog Output)
// ESP32 / ESP32-C6

#define NO2_PIN 2   // pilih pin ADC yang kamu pakai

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12); // ESP32 default 0-4095
}

void loop() {
  int raw = analogRead(NO2_PIN);

  // konversi ke voltage
  float voltage = (raw / 4095.0) * 3.3;

  Serial.print("RAW ADC: ");
  Serial.print(raw);

  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  delay(1000);
}