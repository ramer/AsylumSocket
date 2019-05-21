// AnalogSensor.h

#ifndef _ANALOGSENSOR_h
#define _ANALOGSENSOR_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

#define INTERVAL_DELAY_SENSOR  5000
#define INTERVAL_DELAY_BLINK   50

class AnalogSensor : public Device
{
public:
  AnalogSensor(byte event, byte action);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "AnalogSensor");
  
  void update();
  void updateState(ulong state_new);

protected:
  ulong time_sensor;
};

#endif

