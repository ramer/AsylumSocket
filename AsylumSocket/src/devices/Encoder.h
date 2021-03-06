// Encoder.h

#ifdef ARDUINO_ESP8266_GENERIC

#ifndef _ENCODER_h
#define _ENCODER_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "../Device.h"

#define ENCODER_STEP  1

class Encoder : public Device
{
public:
  Encoder(String prefix, byte event, byte action, byte eventA, byte eventB);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config);

  void updateState(ulong state_new);
  void invertState();

  void doEncoder();

protected:
  byte pin_eventA;
  byte pin_eventB;

  volatile byte seqA;
  volatile byte seqB;

  volatile bool pin_event_laststateA = false;
  volatile bool pin_event_laststateB = false;

  //const char *bit_rep[16] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };
};

static Encoder *encoderinstance;
static void EncoderInterruptFunc() {
  encoderinstance->doEncoder();
}

#endif

#endif