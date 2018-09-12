// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define INTERVAL_EVENT_DEBOUNCE	      100
#define INTERVAL_LED_SETUP	          500
#define INTERVAL_LED_SMARTCONFIG      250
#define INTERVAL_STATE_PUBLISH		    200

class Device
{
public:
  Device(byte event, byte action);

  //void onUpdatedState(std::function<void(ulong)> onUpdatedStateCallback);

  void initialize(PubSubClient *ptr_mqttClient, String prefix = "Device");
  void checkButtons();
  void updateState(ulong state_new, ulong *ptr_state = 0, ulong *ptr_state_old = 0, byte pin = 12); // 12 is default action value
  void invertState(ulong *ptr_state = 0, ulong *ptr_state_old = 0, byte pin = 12);
  void handlePayload(char* topic, String payload);
  void subscribe();
  void publishState(String t, ulong s);
  void checkPublished();

  ulong state;
  ulong state_old;

  String uid;
  String mqtt_topic_pub;
  String mqtt_topic_sub;
  String mqtt_topic_status;
  String mqtt_topic_setup;
  String mqtt_topic_reboot;
  String mqtt_topic_erase;

protected:
  //std::function<void(ulong)> updatedStateCallback;

  void generateUid(String prefix);
  void generateGlobalTopics();
  void generateTopics();
  bool buttonPressed(byte pin, bool * laststate);

  PubSubClient * _mqttClient;

  bool  mqtt_state_published = false;
  ulong	mqtt_state_publishedtime = 0;

  byte  pin_event;
  byte  pin_action;
  bool  pin_event_laststate = false;
  ulong pin_event_time = 0;
};

#endif