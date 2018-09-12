// Config.h
// (ñ) hostmit
// https://github.com/hostmit/

#ifndef _CONFIG_h
#define _CONFIG_h

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <map>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VALIDATOR "x"

class Config {
public:
  Config();

  std::map<String, String> cur_conf;
  std::map<String, String> def_conf;

  bool loadConfig();
  void saveConfig();
  bool loadState();
  void saveState();

};

#endif