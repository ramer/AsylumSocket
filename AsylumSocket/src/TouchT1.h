// TouchT1.h

#ifdef ARDUINO_ESP8266_ESP01

#ifndef _TOUCHT1_h
#define _TOUCHT1_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

class TouchT1 : public Device
{
public:
  TouchT1(byte event, byte action, byte event2, byte action2, byte event3, byte action3);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "TouchT1");
  void update();
  void updateState2(ulong state_new);
  void invertState2();
  void updateState3(ulong state_new);
  void invertState3();
  void handlePayload(String topic, String payload);
  void subscribe();

  ulong state2;
  ulong state_old2;
  ulong state3;
  ulong state_old3;

  String mqtt_topic_pub2;
  String mqtt_topic_pub3;
  String mqtt_topic_sub2;
  String mqtt_topic_sub3;

protected:
  void generateTopics();
  void loadState();
  void saveState();

  bool state_published2 = false;
  bool state_published3 = false;

  byte pin_event2;
  byte pin_event3;
  byte pin_action2;
  byte pin_action3;
  bool pin_event_laststate2 = false;
  bool pin_event_laststate3 = false;
  float pin_event_average2;
  float pin_event_average3;
  ulong pin_event_time2 = 0;
  ulong pin_event_time3 = 0;

};

#endif

#endif