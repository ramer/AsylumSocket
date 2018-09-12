//
///** Load WLAN credentials from EEPROM */
//
//#define EEPROM_VALIDATOR 'x'
//
//struct Config {
//  ulong     state;
//  ulong     state2;
//  ulong     state3;
//  ulong     state4;
//  char		  description[128];
//  byte    	mode;
//  char		  apssid[32];
//  char		  apkey[32];
//  char		  locallogin[32];
//  char		  localpassword[32];
//  char		  mqttserver[256];
//  char		  mqttlogin[32];
//  char		  mqttpassword[32];
//  byte    	onboot;
//  byte    	onboardled;
//  char    	extension1[32];
//  char    	extension2[32];
//  char    	extension3[32];
//  byte		  validator;
//} config;
//
//void eeprom_dump() {
//  Serial.printf("Dump EEPROM (%u bytes): \n", sizeof(Config));
//  Serial.printf("-------------\t\tBEGIN OF DUMP\t\t------------\n");
//  char c;
//  for (int i = 0; i < sizeof(Config); i++) {
//    EEPROM.get(i, c);
//    Serial.print(c);
//    if ((i + 1) % 60 == 0 && i < sizeof(Config)) {Serial.printf("\n");}
//  }
//  Serial.printf("\n-------------\t\tEND OF DUMP\t\t------------\n");
//}
//
//bool eeprom_load() {
//  EEPROM.get(0, config);
//  if (config.validator != EEPROM_VALIDATOR) {
//    
//    config.state = 0;
//    config.state2 = 0;
//    config.state3 = 0;
//    config.state4 = 0;
//    config.description[0] = 0;
//    config.mode = 0;
//    config.apssid[0] = 0;
//    config.apkey[0] = 0;
//    config.locallogin[0] = 0;
//    config.localpassword[0] = 0;
//    config.mqttserver[0] = 0;
//    config.mqttlogin[0] = 0;
//    config.mqttpassword[0] = 0;
//    config.onboot = 0;
//    config.onboardled = 0;
//    config.extension1[0] = 0;
//    config.extension2[0] = 0;
//    config.extension3[0] = 0;
//    config.validator = 0;
//
//    return false;
//  } else {
//    return true;
//  }
//}
//
//void eeprom_save() {
//  eeprom_erase();
//  config.validator = EEPROM_VALIDATOR;
//  EEPROM.put(0, config);
//  EEPROM.commit();
//}
//
//void eeprom_erase() {
//  for (int i = 0; i < sizeof(Config); i++) {
//    EEPROM.write(i, 255);
//  }
//  EEPROM.commit();
//}
//
//
