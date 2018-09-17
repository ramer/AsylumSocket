// Socket.cpp

#include "Socket.h"

Socket::Socket(byte event, byte action) : Device(event, action) {};

void Socket::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  Device::initialize(ptr_mqttClient, ptr_config, prefix);
}
