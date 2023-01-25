#include <EEPROM.h>
void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  Serial.println("EEPROM START");
  // put your setup code here, to run once:
  for (int i = 0; i < 512; i++) {
    EEPROM.writeString(i, "");
  }
  EEPROM.commit();
  Serial.println("EEPROM END");
  delay(1000);
  // for loop untuk menampilkan data eeprom
  Serial.println("Serial Print START");
  for (int i = 0; i < 512; i++) {
    Serial.print(i+":");
    Serial.println(EEPROM.readString(i));
  }
  Serial.println("Serial Print END");
}

void loop() {
  
}
