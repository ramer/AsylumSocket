// Socket.h

#ifndef _SOCKET_h
#define _SOCKET_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Device.h"

class Socket : public Device
{
public:
  Socket(byte pin_event, byte pin_action);

  //void onUpdatedState(std::function<void(ulong)> onUpdatedStateCallback);

  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "Socket");

protected:
  //std::function<void(ulong)> updatedStateCallback;
};

#endif

