// Config.cpp

#include "Config.h"

Config::Config() {
  def_conf = {
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
    { "extension3", "" }
  };
}

bool Config::loadConfig() {
  debug("Reading Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "r");
  if (!file)
  {
    debug("failed. Using default configuration \n");
    cur_conf = def_conf;
    return false;
  }
  else {
    debug("success \n");
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);
  //debug("\n-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\t\tEND OF FILE\t\t------------\n\n");
  if (!root.success()) {
    debug("Parsing Configuration file (%s): failed. Using default configuration \n", file.name());
    cur_conf = def_conf;
    file.close();
    return false;
  }
  if (!root.containsKey("validator") || root["validator"] != CONFIG_VALIDATOR) { 
    debug("Validating Configuration file (%s): failed. Using default configuration \n", file.name());
    cur_conf = def_conf;
    file.close();
    return false;
  }

  for (auto &itemDefault : def_conf) {
    if (!root.containsKey(itemDefault.first)) {
      debug("Configuration file (%s) does not have (%s) key, using default value (%s)", file.name(), itemDefault.first.c_str(), itemDefault.second.c_str());
      cur_conf.insert(itemDefault);
    }
    else {
      cur_conf[itemDefault.first] = root[itemDefault.first].as<String>();
    }
  }
  file.close();

  return true;
}

//bool Config::loadConfig(bool eeprom) {
//  EEPROM.begin(SPI_FLASH_SEC_SIZE);
//  uint16_t lng;
//  EEPROM.get(0, lng);
//  debug("Reading Configuration from EEPROM (%u of %u bytes) ... ", lng, SPI_FLASH_SEC_SIZE);
//  char* buf = new char[lng];
//  if (lng < 4096) {
//    for (uint16_t i = 0; i < lng; i++) { buf[i] = char(EEPROM.read(i + 2)); }
//    buf[lng] = '\x0';
//    debug("success \n");
//  } else {
//    debug("failed: too large length \n");
//    cur_conf = def_conf;
//    EEPROM.end();
//    return false;
//  }
//  EEPROM.end();
//
//  DynamicJsonBuffer jsonBuffer;
//  JsonObject &root = jsonBuffer.parseObject(String(buf));
//  //debug("\n------------\t\tBEGIN OF JSON\t\t-----------\n");
//  //root.prettyPrintTo(Serial);
//  //debug("\n------------\t\tEND OF JSON\t\t-----------\n\n");
//  if (!root.success()) {
//    debug("Parsing EEPROM failed. Using default configuration \n");
//    cur_conf = def_conf;
//    return false;
//  }
//  if (!root.containsKey("validator") || root["validator"] != CONFIG_VALIDATOR) {
//    debug("Validating Configuration: failed. Using default configuration \n");
//    cur_conf = def_conf;
//    return false;
//  }
//
//  for (auto &itemDefault : def_conf) {
//    if (!root.containsKey(itemDefault.first)) {
//      debug("Configuration does not have (%s) key, using default value (%s)", itemDefault.first.c_str(), itemDefault.second.c_str());
//      cur_conf.insert(itemDefault);
//    }
//    else {
//      cur_conf[itemDefault.first] = root[itemDefault.first].as<String>();
//    }
//  }
//
//  return true;
//}

void Config::saveConfig() {
  debug("Saving Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "w");
  if (!file)
  {
    debug("failed: cannot be created \n");
    return;
  }
  else {
    debug("success \n");
  }
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["validator"] = CONFIG_VALIDATOR;

  for (auto &item : cur_conf) {
    root[item.first] = item.second;
  }
  //debug("\n-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\t\tEND OF FILE\t\t------------\n\n");
  if (root.printTo(file) == 0)
  {
    debug("Writing Configuration file (%s): failed.", file.name());
  }

  file.close();
}

//void Config::saveConfig(bool eeprom) {
//  DynamicJsonBuffer jsonBuffer;
//  JsonObject &root = jsonBuffer.createObject();
//
//  root["validator"] = CONFIG_VALIDATOR;
//
//  for (auto &item : cur_conf) {
//    root[item.first] = item.second;
//  }
//  //debug("\n------------\t\tBEGIN OF EEPROM\t\t-----------\n");
//  //root.prettyPrintTo(Serial);
//  //debug("\n------------\t\tEND OF EEPROM\t\t-----------\n\n");
//  String json;
//  debug("Saving Configuration to EEPROM ... ");
//  uint16_t lng = root.printTo(json);
//  EEPROM.begin(lng + 2);
//  EEPROM.put(0, lng);
//  unsigned char* buf = new unsigned char[lng];
//  json.getBytes(buf, lng + 1);
//  for (uint16_t i = 0; i < lng; i++) { EEPROM.write(i + 2, buf[i]); }
//  if (EEPROM.commit()) {
//    debug("success \n");
//  }
//  else {
//    debug("error \n");
//  }
//  EEPROM.end();
//}

std::map<String, String> Config::loadState()
{
  std::map<String, String> states;
  File file = SPIFFS.open(CONFIG_FILENAME_STATE, "r");
  if (!file) return states;
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);
  //debug("-------------\t\tBEGIN OF LOAD\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\t\tEND OF LOAD\t\t------------\n");
    if (!root.success()) { file.close(); return states; }
  for (auto &jsonPair : root) {
    states.insert(std::pair<String, String>(String(jsonPair.key), String(jsonPair.value.asString())));
  }
  file.close();
  return states;
}

void Config::saveState(JsonObject &root)
{
  File file = SPIFFS.open(CONFIG_FILENAME_STATE, "w");
  if (!file) return;
  //debug("-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //debug("\n-------------\t\tEND OF FILE\t\t------------\n");
  root.printTo(file);
  file.close();
}
