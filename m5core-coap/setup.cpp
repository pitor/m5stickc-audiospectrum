#include<M5Stack.h>
#include<SD.h>
#include<Preferences.h>
#include "setup.h"

#include <ArduinoJson.h>

const char *filename = "/setup.json";
char wifi_ap[100];
char wifi_pass[100];

Preferences preferences;

bool hasSDCardSetup() {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  file.close();
  return true;
} 

bool writeValuesToPreferences() {
  preferences.begin("coapapp", false);
  preferences.putString("wifi-ap", String(wifi_ap));
  preferences.putString("wifi-pass", String(wifi_pass));
  preferences.end();
}

setupStatus readConfigFromSD() {
  // Deserialize the JSON document
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return CPSetup_FailedReadingFromSD;
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
    return CPSetup_FailedReadingFromSD;
  }

  const char* json_wifi_pass = doc["wifi_pass"];
  if(json_wifi_pass != NULL)
    strlcpy(wifi_pass, json_wifi_pass, sizeof(wifi_pass));
  else {
    Serial.println("No wifi password defined config file");
    Serial.print("WifiAP:");
    Serial.println(wifi_ap);
    Serial.print("WifiPass:");
    Serial.println(wifi_pass);

    return CPSetup_FailedReadingFromSD;
  }

  writeValuesToPreferences();
  return CPSetup_ReadFromSD;
}

setupStatus readValuesFromPreferences() {
  preferences.begin("coapapp", true);
  String pWifiAP = preferences.getString("wifi-ap");
  if(pWifiAP == NULL)
    return CPSetup_FailedReadingFromFlash;
  pWifiAP.toCharArray(wifi_ap, sizeof(wifi_ap));

  String pWifiPass = preferences.getString("wifi-pass");
  if(pWifiAP == NULL)
    return CPSetup_FailedReadingFromFlash;
   pWifiAP.toCharArray(wifi_pass, sizeof(wifi_pass));
  
  preferences.end();
  return CPSetup_ReadFromFlash;
}

setupStatus setupRead() {
  bool hasSetup = hasSDCardSetup();
  if(hasSetup) {
    M5.Lcd.fillScreen(WHITE);
    delay(1000);
    M5.Lcd.fillScreen(BLUE);
    return readConfigFromSD();
  }
  M5.Lcd.fillScreen(BLACK);
  delay(1000);
  return readValuesFromPreferences();
}
