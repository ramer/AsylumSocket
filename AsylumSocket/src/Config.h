// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include "ESP8266WiFi.h"
#include <EEPROM.h>

class Config
{
  public:
    void dump();
    bool load();
    void save();
    void erase();

    struct ConfigStruct {
      ulong     state;
      ulong     state2;
      ulong     state3;
      ulong     state4;
      char		  description[128];
      byte    	mode;
      char		  apssid[32];
      char		  apkey[32];
      char		  locallogin[32];
      char		  localpassword[32];
      char		  mqttserver[256];
      char		  mqttlogin[32];
      char		  mqttpassword[32];
      byte    	onboot;
      byte    	onboardled;
      char    	extension1[32];
      char    	extension2[32];
      char    	extension3[32];
      byte		  validator;
    } c;
};

#endif

