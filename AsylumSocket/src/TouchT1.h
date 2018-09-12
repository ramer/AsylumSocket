// TouchT1.h

#ifndef _TOUCHT1_h
#define _TOUCHT1_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

class TouchT1 : public Device
{
public:
  TouchT1(byte event, byte action, byte event2, byte action2, byte event3, byte action3);

  //void onUpdatedState(std::function<void(ulong, ulong, ulong)> onUpdatedStateCallback);

  void initialize(PubSubClient *ptr_mqttClient, String prefix = "TouchT1");
  void checkButtons();
  void handlePayload(char * topic, String payload);
  void subscribe();
  void checkPublished();

  ulong state2;
  ulong state2_old;
  ulong state3;
  ulong state3_old;

  String mqtt_topic_pub2;
  String mqtt_topic_pub3;
  String mqtt_topic_sub2;
  String mqtt_topic_sub3;

protected:
  //std::function<void(ulong, ulong, ulong)> updatedStateCallback;

  void generateTopics();

  byte pin_event2;
  byte pin_action2;
  byte pin_event3;
  byte pin_action3;
  bool pin_event_laststate2 = false;
  bool pin_event_laststate3 = false;
};

#endif

