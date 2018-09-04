
/** Load WLAN credentials from EEPROM */

#define validator_char 'x'

void dumpConfig() {
  Serial.printf("Dump EEPROM (%u bytes): \n", sizeof(Config));
  Serial.printf("-------------\t\tBEGIN OF DUMP\t\t------------\n");
  char c;
  for (int i = 0; i < sizeof(Config); i++) {
    EEPROM.get(i, c);
    Serial.print(c);
    if ((i + 1) % 60 == 0 && i < sizeof(Config)) {Serial.printf("\n");}
  }
  Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n");
}

bool loadConfig() {
  EEPROM.get(0, config);
  if (config.validator != validator_char) {
    
    config.reserved[0] = 0;
    config.description[0] = 0;
    config.mode = 1;
    config.apssid[0] = 0;
    config.apkey[0] = 0;
    config.locallogin[0] = 0;
    config.localpassword[0] = 0;
    config.mqttserver[0] = 0;
    config.mqttlogin[0] = 0;
    config.mqttpassword[0] = 0;
    config.extension1[0] = 0;
    config.extension2[0] = 0;
    config.extension3[0] = 0;
    config.validator = 0;

    return false;
  } else {
    return true;
  }
}

void clearConfig() {
  for (int i = 0; i < sizeof(Config); i++) {
    EEPROM.write(i, 255);
  }
  EEPROM.commit();
}

void saveConfig() {
  clearConfig();
  config.validator = validator_char;
  EEPROM.put(0, config);
  EEPROM.commit();
}


