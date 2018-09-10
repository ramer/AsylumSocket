// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include "ESP8266WiFi.h"
#include <PubSubClient.h>

#define INTERVAL_EVENT_DEBOUNCE	      100
#define INTERVAL_LED_SETUP	          500
#define INTERVAL_LED_SMARTCONFIG      250
#define INTERVAL_STATE_PUBLISH		    200

class Device
{
public:
  Device(byte pin_event, byte pin_action);
  void initialize(String device_prefix, PubSubClient *mqttClient);
  void check_buttons();
  void update_state(ulong state_new);
  void invert_state();

  void handlePayload(char * topic, String payload);
  void publishstate();
  void check_published();

  ulong state;
  char * uid;
  char * mqtt_topic_pub;
  char * mqtt_topic_sub;
  char * mqtt_topic_status;
  char * mqtt_topic_setup;
  char * mqtt_topic_reboot;
  char * mqtt_topic_erase;

protected:
  void generateglobaltopics(String id);
  void generatetopics(String id);

  PubSubClient * _mqttClient;
  bool mqtt_state_published = false;
  ulong	mqtt_state_publishedtime = 0;
  byte _pin_event;
  byte _pin_action;
  ulong _state;
  bool laststate_event = false;
  ulong time_event = 0;
};

#endif