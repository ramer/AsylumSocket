// Config.cpp

#include "Config.h"

#define EEPROM_VALIDATOR 'x'

void Config::dump() {
  Serial.printf("Dump EEPROM (%u bytes): \n", sizeof(Config));
  Serial.printf("-------------\t\tBEGIN OF DUMP\t\t------------\n");
  char c;
  for (int i = 0; i < sizeof(Config); i++) {
    EEPROM.get(i, c);
    Serial.print(c);
    if ((i + 1) % 60 == 0 && i < sizeof(Config)) { Serial.printf("\n"); }
  }
  Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n");
}

bool Config::load() {
  EEPROM.get(0, c);
  if (c.validator != EEPROM_VALIDATOR) {
    c.state = 0;
    c.state2 = 0;
    c.state3 = 0;
    c.state4 = 0;
    c.description[0] = 0;
    c.mode = 0;
    c.apssid[0] = 0;
    c.apkey[0] = 0;
    c.locallogin[0] = 0;
    c.localpassword[0] = 0;
    c.mqttserver[0] = 0;
    c.mqttlogin[0] = 0;
    c.mqttpassword[0] = 0;
    c.onboot = 0;
    c.onboardled = 0;
    c.extension1[0] = 0;
    c.extension2[0] = 0;
    c.extension3[0] = 0;
    c.validator = 0;

    return false;
  }
  else {
    return true;
  }
}

void Config::save() {
  erase();
  c.validator = EEPROM_VALIDATOR;
  EEPROM.put(0, this);
  EEPROM.commit();
}

void Config::erase() {
  for (int i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, 255);
  }
  EEPROM.commit();
}
