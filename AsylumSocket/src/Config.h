// Config.h
// (ñ) hostmit
// https://github.com/hostmit/

#ifndef _CONFIG_h
#define _CONFIG_h

#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <EEPROM.h>
#include <map>

extern "C" {
  #include "spi_flash.h"
}

#define CONFIG_FILENAME_CONFIG "/config.json"
#define CONFIG_FILENAME_STATE "/state.json"
#define CONFIG_VALIDATOR "x"

class Config {
public:
  Config();

  std::map<String, String> cur_conf;
  std::map<String, String> def_conf;

  bool loadConfig();
  //bool loadConfig(bool eeprom);
  void saveConfig();
  //void saveConfig(bool eeprom);
  std::map<String, String> loadState();
  void saveState(JsonObject &root);

};

#endif