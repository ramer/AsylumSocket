// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#define CONFIG_FILENAME "/config.json"

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <map>

std::map<String, String> config;
std::map<String, String> configDefault = {
  { "state", "0" },
  { "state2", "0" },
  { "state3", "0" },
  { "state4", "0" },
  { "description", "" },
  { "mode", "0" },
  { "apssid", "" },
  { "apkey", "" },
  { "locallogin", "" },
  { "localpassword", "" },
  { "mqttserver", "" },
  { "mqttlogin", "" },
  { "mqttpassword", "" },
  { "onboot", "0" },
  { "onboardled", "0" },
  { "extension1", "" },
  { "extension2", "" },
  { "extension3", "" },
};

class Config {
public:
  Config();
  
  static bool load();
  static void save();
};

#endif