// Config.cpp

#include "Config.h"

Config::Config() {
  def_conf = {
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
    { "extension3", "" }
  };
}

bool Config::loadConfig() {
  Serial.printf("Reading Configuration file (%s) ... ", String(CONFIG_FILENAME).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME, "r");
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

  Serial.printf("-------------\t\tBEGIN OF DUMP\t\t------------\n");
  root.prettyPrintTo(Serial);
  Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n");

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
  Serial.printf("Saving Configuration file (%s) ... ", String(CONFIG_FILENAME).c_str());

  File file = SPIFFS.open(CONFIG_FILENAME, "w");
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

  Serial.printf("-------------\t\tBEGIN OF DUMP\t\t------------\n");
  root.prettyPrintTo(Serial);
  Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n");

  if (root.printTo(file) == 0)
  {
    Serial.printf("Writing Configuration file (%s): failed.", file.name());
  }

  file.close();
}

bool Config::loadState()
{
  return false;
}

void Config::saveState()
{
}

//String getAsSting() {
//  String r = "";
//  for (auto &item : config) {
//    r += item.first + ":" + item.second + "\n";
//  }
//  return r;
//}
