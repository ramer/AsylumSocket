// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Config.h"

#define INTERVAL_BUTTON_DEBOUNCE	    100
#define INTERVAL_BUTTON_IDLE	        200
#define INTERVAL_LED_SETUP	          500
#define INTERVAL_LED_SMARTCONFIG      250
#define INTERVAL_STATE_PUBLISH		    200
#define DEBOUNCE_AVERAGE           		1000

enum buttonstates { RELEASED = 0, DOWN = 1, DOWNIDLE = 2, PRESSED = 4, UP = 8, UPIDLE = 16 };

class Device
{
public:
  Device(String prefix, byte event, byte action);
  virtual ~Device();

  virtual void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config);

  virtual void update();
  virtual void updateState(ulong state_new);
  virtual void invertState();
  virtual void handlePayload(String topic, String payload);

  virtual void subscribe();
  virtual void publishState(String topic, ulong statepayload, bool *ptr_state_published);
  virtual void publishStatus();

  //void onUpdateState(std::function<void(ulong)> onUpdateStateCallback);
  
  ulong state;
  ulong state_old;

  String uid_prefix;
  String uid;
  String mqtt_topic_status;
  String mqtt_topic_setup;
  String mqtt_topic_reboot;
  String mqtt_topic_pub;
  String mqtt_topic_sub;

protected:
  virtual void generateUid();
  virtual void loadState();
  virtual void saveState();
  virtual buttonstates buttonState(byte pin, bool *ptr_pin_event_laststate, float *ptr_pin_average, ulong *ptr_pin_event_time);

  //std::function<void(ulong)> updateStateCallback;

  PubSubClient * _mqttClient;
  Config * _config;

  bool  state_published = false;
  ulong	state_publishedtime = 0;

  byte  pin_event;
  byte  pin_action;
  bool  pin_event_laststate = false;
  float pin_event_average;
  ulong pin_event_time = 0;
};

#endif