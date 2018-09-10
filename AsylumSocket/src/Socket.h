// Socket.h

#ifndef _SOCKET_h
#define _SOCKET_h

#include "ESP8266WiFi.h"
#include "Device.h"

class Socket : public Device
{
  public:
    Socket(byte pin_event, byte pin_action) : Device(pin_event, pin_action) {};
    void init(PubSubClient *mqttClient);
};

#endif

