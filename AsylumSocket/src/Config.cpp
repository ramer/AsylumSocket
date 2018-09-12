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
  //Serial.printf("\n-------------\t\tBEGIN OF DUMP\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n\n");
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
  //Serial.printf("\n-------------\t\tBEGIN OF DUMP\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n\n");
  if (root.printTo(file) == 0)
  {
    Serial.printf("Writing Configuration file (%s): failed.", file.name());
  }

  file.close();
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
  //Serial.printf("-------------\t\tBEGIN OF SAVE\t\t------------\n");
  //root.prettyPrintTo(Serial);
  //Serial.printf("\n-------------\t\tEND OF SAVE\t\t------------\n");
  root.printTo(file);
  file.close();
}
