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
  Serial.printf("Reading Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "r");
  if (!file)
  {
    Serial.printf("failed. Using default configuration \n");
    cur_conf = def_conf;
    return false;
  }
  else {
    Serial.printf("success \n");
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);
  //Serial.printf("\n-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF FILE\t\t------------\n\n");
  if (!root.success()) {
    Serial.printf("Parsing Configuration file (%s): failed. Using default configuration \n", file.name());
    cur_conf = def_conf;
    file.close();
    return false;
  }
  if (!root.containsKey("validator") || root["validator"] != CONFIG_VALIDATOR) { 
    Serial.printf("Validating Configuration file (%s): failed. Using default configuration \n", file.name());
    cur_conf = def_conf;
    file.close();
    return false;
  }

  for (auto &itemDefault : def_conf) {
    if (!root.containsKey(itemDefault.first)) {
      Serial.printf("Configuration file (%s) does not have (%s) key, using default value (%s)", file.name(), itemDefault.first.c_str(), itemDefault.second.c_str());
      cur_conf.insert(itemDefault);
    }
    else {
      cur_conf[itemDefault.first] = root[itemDefault.first].as<String>();
    }
  }
  file.close();

  return true;
}

bool Config::loadConfig(bool eeprom) {
  EEPROM.begin(SPI_FLASH_SEC_SIZE);
  uint16_t lng;
  EEPROM.get(0, lng);
  Serial.printf("Reading Configuration from EEPROM (%u of %u bytes) ... ", lng, SPI_FLASH_SEC_SIZE);
  char* buf = new char[lng];
  if (lng < 4096) {
    for (uint16_t i = 0; i < lng; i++) { buf[i] = char(EEPROM.read(i + 2)); }
    buf[lng] = '\x0';
    Serial.printf("success \n");
  } else {
    Serial.printf("failed: too large length \n");
    cur_conf = def_conf;
    EEPROM.end();
    return false;
  }
  EEPROM.end();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(String(buf));
  //Serial.printf("\n------------\t\tBEGIN OF JSON\t\t-----------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n------------\t\tEND OF JSON\t\t-----------\n\n");
  if (!root.success()) {
    Serial.printf("Parsing EEPROM failed. Using default configuration \n");
    cur_conf = def_conf;
    return false;
  }
  if (!root.containsKey("validator") || root["validator"] != CONFIG_VALIDATOR) {
    Serial.printf("Validating Configuration: failed. Using default configuration \n");
    cur_conf = def_conf;
    return false;
  }

  for (auto &itemDefault : def_conf) {
    if (!root.containsKey(itemDefault.first)) {
      Serial.printf("Configuration does not have (%s) key, using default value (%s)", itemDefault.first.c_str(), itemDefault.second.c_str());
      cur_conf.insert(itemDefault);
    }
    else {
      cur_conf[itemDefault.first] = root[itemDefault.first].as<String>();
    }
  }

  return true;
}

void Config::saveConfig() {
  Serial.printf("Saving Configuration file (%s) ... ", String(CONFIG_FILENAME_CONFIG).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME_CONFIG, "w");
  if (!file)
  {
    Serial.printf("failed: cannot be created \n");
    return;
  }
  else {
    Serial.printf("success \n");
  }
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["validator"] = CONFIG_VALIDATOR;

  for (auto &item : cur_conf) {
    root[item.first] = item.second;
  }
  //Serial.printf("\n-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF FILE\t\t------------\n\n");
  if (root.printTo(file) == 0)
  {
    Serial.printf("Writing Configuration file (%s): failed.", file.name());
  }

  file.close();
}

void Config::saveConfig(bool eeprom) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["validator"] = CONFIG_VALIDATOR;

  for (auto &item : cur_conf) {
    root[item.first] = item.second;
  }
  //Serial.printf("\n------------\t\tBEGIN OF EEPROM\t\t-----------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n------------\t\tEND OF EEPROM\t\t-----------\n\n");
  String json;
  Serial.printf("Saving Configuration to EEPROM ... ");
  uint16_t lng = root.printTo(json);
  EEPROM.begin(lng + 2);
  EEPROM.put(0, lng);
  unsigned char* buf = new unsigned char[lng];
  json.getBytes(buf, lng + 1);
  for (uint16_t i = 0; i < lng; i++) { EEPROM.write(i + 2, buf[i]); }
  if (EEPROM.commit()) {
    Serial.printf("success \n");
  }
  else {
    Serial.printf("error \n");
  }
  EEPROM.end();
}

std::map<String, String> Config::loadState()
{
  std::map<String, String> states;
  File file = SPIFFS.open(CONFIG_FILENAME_STATE, "r");
  if (!file) return states;
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);
  //Serial.printf("-------------\t\tBEGIN OF LOAD\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF LOAD\t\t------------\n");
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
  //Serial.printf("-------------\t\tBEGIN OF FILE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF FILE\t\t------------\n");
  root.printTo(file);
  file.close();
}
