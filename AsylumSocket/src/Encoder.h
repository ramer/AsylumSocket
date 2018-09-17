// Encoder.h

#ifndef _ENCODER_h
#define _ENCODER_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

class Encoder : public Device
{
public:
  Encoder(byte event, byte action, byte event2);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "Encoder");
  void update();
  void updateState(ulong state_new, ulong * ptr_state, ulong * ptr_state_old, bool * ptr_state_published, byte pin);

  ulong encoderstate;

protected:
  byte pin_event2;
  bool pin_event_laststate2 = false;
};

#endif

