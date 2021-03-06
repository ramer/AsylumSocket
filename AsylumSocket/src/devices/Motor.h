// Motor.h

#ifdef ARDUINO_ESP8266_GENERIC

#ifndef _MOTOR_h
#define _MOTOR_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "../Device.h"

#define INTERVAL_MOTOR  5000

class Motor : public Device
{
public:
  Motor(String prefix, byte event, byte action, byte action2);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config);
  void updateState(ulong state_new);

protected:
  byte pin_action2;
};

#endif

#endif