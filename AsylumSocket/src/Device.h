// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Config.h"

#define INTERVAL_EVENT_DEBOUNCE	      100
#define INTERVAL_LED_SETUP	          500
#define INTERVAL_LED_SMARTCONFIG      250
#define INTERVAL_STATE_PUBLISH		    200

class Device
{
public:
  Device(byte event, byte action);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "Device");

  void update();
  void updateState(ulong state_new);
  void invertState();
  void handlePayload(String topic, String payload);

  void subscribe();
  void publishState(String topic, ulong statepayload, bool *ptr_state_published);

  //void onUpdateState(std::function<void(ulong)> onUpdateStateCallback);
  
  ulong state;
  ulong state_old;

  String uid;
  String mqtt_topic_pub;
  String mqtt_topic_sub;
  String mqtt_topic_status;
  String mqtt_topic_setup;
  String mqtt_topic_reboot;

protected:
  void generateUid(String prefix);
  void generateGlobalTopics();
  void generateTopics();
  bool buttonPressed(byte pin, bool * laststate);
  void loadState();
  void saveState();
  //std::function<void(ulong)> updateStateCallback;

  PubSubClient * _mqttClient;
  Config * _config;

  bool  state_published = false;
  ulong	state_publishedtime = 0;

  byte  pin_event;
  byte  pin_action;
  bool  pin_event_laststate = false;
  ulong pin_event_time = 0;
};

#endif