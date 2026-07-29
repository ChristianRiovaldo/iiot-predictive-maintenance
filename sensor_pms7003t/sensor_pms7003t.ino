#include <PMS.h>
#define RXD1 4
#define TXD1 5

PMS pms(Serial1);
PMS::DATA data;

void setup() {
  Serial.begin(115200);
  Serial.println("Inisialisasi sensor PMS7003T...");
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);
}

void loop() {
  if (pms.read(data)) {
    Serial.println("------------ DATA KUALITAS UDARA ------------");
    Serial.print("PM 1.0: "); Serial.print(data.PM_AE_UG_1_0); Serial.println(" ug/m3");
    Serial.print("PM 2.5: "); Serial.print(data.PM_AE_UG_2_5); Serial.println(" ug/m3");
    Serial.print("PM 10.0: "); Serial.print(data.PM_AE_UG_10_0); Serial.println(" ug/m3");
    Serial.println("----------------------------------------\n");
  }

  delay(100);
}
