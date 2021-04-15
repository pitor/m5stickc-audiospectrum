#include <M5Stack.h>
#include "setup.h"
#include <Preferences.h>
int ch;

void setup() {

  M5.begin();

  Serial.begin(115200);

  setupStatus status = setupRead();
  if(status == CPSetup_ReadFromSD) {
    M5.Lcd.println("Read from file");
    M5.Lcd.println("Remove SD-card and reset");
    return;
  }

  if(status == CPSetup_ReadFromFlash) {
    M5.Lcd.println("Read from file");
    M5.Lcd.println("Ready to proceed");
    return;
  }
  M5.Lcd.println("Something might have failed");
}

void loop() {

}