// Socket.cpp

#ifdef ARDUINO_ESP8266_ESP01

#include "Socket.h"

Socket::Socket(String prefix, byte event, byte action) : Device(prefix, event, action) {};

void Socket::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config) {
  Device::initialize(ptr_mqttClient, ptr_config);
}

#endif