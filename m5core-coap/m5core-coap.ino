#include <M5Stack.h>
#include "setup.h"
#include <Preferences.h>
int ch;

void setup() {

  M5.begin();

  Serial.begin(115200);

  setupRead();

  File file = SD.open("/setup.json");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  M5.Lcd.print("Read from file: ");
  while (file.available()) {
    ch = file.read();
    Serial.write(ch);
    M5.Lcd.write(ch);
  }
}

void loop() {

}