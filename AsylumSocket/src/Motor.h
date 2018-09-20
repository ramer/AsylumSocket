// Motor.h

#ifndef _MOTOR_h
#define _MOTOR_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

#define INTERVAL_MOTOR  5000

class Motor : public Device
{
public:
  Motor(byte event, byte action, byte action2);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "Motor");
  void updateState(ulong state_new);

protected:
  byte pin_action2;
};

#endif
