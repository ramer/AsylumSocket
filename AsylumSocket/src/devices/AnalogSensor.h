// AnalogSensor.h

#ifdef ARDUINO_AMPERKA_WIFI_SLOT

#ifndef _ANALOGSENSOR_h
#define _ANALOGSENSOR_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "../Device.h"

#define INTERVAL_DELAY_SENSOR    3000
#define INTERVAL_DELAY_BLINK     50
#define INTERVAL_VALUE_THRESHOLD 5

class AnalogSensor : public Device
{
public:
  AnalogSensor(byte event, byte action, byte sensor);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "AnalogSensor");
  
  void update();
  void updateState(ulong state_new);

protected:
  byte pin_sensor;
  ulong time_sensor;
  ulong zero_state;
};

#endif

#endif
