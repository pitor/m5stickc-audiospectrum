#include<M5Stack.h>
#include<SD.h>
#include<Preferences.h>
#include "setup.h"

#include <ArduinoJson.h>

const char *filename = "/setup.json";
char wifi_ap[100];
char wifi_pass[100];

bool hasSDCardSetup() {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  file.close();
  return true;
} 

bool readConfigFromSD() {
  // Deserialize the JSON document
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println("Failed to read file, using default configuration");

  // Copy values from the JsonDocument to the Config
  int port = doc["port"] | 2731;
  
  const char* json_wifi_ap = doc["wifi_ap"];
  if(json_wifi_ap != NULL) 
    strlcpy(wifi_ap, json_wifi_ap, sizeof(wifi_ap));
  else {
    Serial.println("No access point defined config file");
    return false;
  }

  const char* json_wifi_pass = doc["wifi_pass"];
  if(json_wifi_pass != NULL)
    strlcpy(wifi_pass, json_wifi_pass, sizeof(wifi_pass));
  else {
    Serial.println("No wifi password defined config file");
    return false;
  }
  return true;
}


setupStatus setupRead() {
  bool hasSetup = hasSDCardSetup();
  if(hasSetup) {
    M5.Lcd.fillScreen(WHITE);
    delay(1000);
    M5.Lcd.fillScreen(BLUE);
    bool sdReadStatus = readConfigFromSD();
    return sdReadStatus ? CPSetup_ReadFromSD : CPSetup_FailedReadingFromSD;
  }
  else
    M5.Lcd.fillScreen(RED);
  delay(5000);
  return CPSetup_OK;
}
