// Socket.cpp

#include "Socket.h"

void Socket::init(PubSubClient * mqttClient)
{
  Device::initialize("Socket", mqttClient);
}
