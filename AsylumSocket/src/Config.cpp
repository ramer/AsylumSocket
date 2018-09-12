// Config.cpp

#include "Config.h"

Config::Config() {}

bool Config::load()
{
  Serial.printf("Reading Configuration file (%s) ... ", String(CONFIG_FILENAME).c_str());
  
  File file = SPIFFS.open(CONFIG_FILENAME, "r");
  if (!file)
  {
    Serial.printf("failed: does not exists or can't be open. Using default configuration \n");
    config = configDefault;
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
    config = configDefault;
    file.close();
    return false;
  }
  for (auto &itemDefault : configDefault) {
    if (!root.containsKey(itemDefault.first)) {
      Serial.printf("Configuration file (%s) does not have (%s) key, use default value (%s)", file.name(), itemDefault.first.c_str(), itemDefault.second.c_str());
      config.insert(itemDefault);
    }
    else {
      config[itemDefault.first] = root[itemDefault.first].as<String>();
    }
  }
  file.close();
  return true;
}

void Config::save()
{
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

  Serial.printf(" - description:         %s \n", config["description"].c_str());
  Serial.printf(" - mode:                %u \n", config["mode"].toInt());
  Serial.printf(" - apssid:              %s \n", config["apssid"].c_str());
  Serial.printf(" - apkey:               %s \n", config["apkey"].c_str());
  Serial.printf(" - locallogin:          %s \n", config["locallogin"].c_str());
  Serial.printf(" - localpassword:       %s \n", config["localpassword"].c_str());
  Serial.printf(" - mqttserver:          %s \n", config["mqttserver"].c_str());
  Serial.printf(" - mqttlogin:           %s \n", config["mqttlogin"].c_str());
  Serial.printf(" - mqttpassword:        %s \n", config["mqttpassword"].c_str());
  Serial.printf(" - onboot:              %u \n", config["onboot"].toInt());
  Serial.printf(" - onboardled:          %u \n", config["onboardled"].toInt());
  Serial.printf(" - extension 1 / 2 / 3: %s / %s / %s \n", config["extension1"].c_str(), config["extension2"].c_str(), config["extension3"].c_str());
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  for (auto &item : config) {
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

//String Config::getAsSting() {
//  String r = "";
//  for (auto &item : config) {
//    r += item.first + ":" + item.second + "\n";
//  }
//  return r;
//}