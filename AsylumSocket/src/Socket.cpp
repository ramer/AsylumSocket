// Socket.cpp

#include "Socket.h"

Socket::Socket(byte pin_event, byte pin_action) : Device(pin_event, pin_action) {};

//void Socket::onUpdatedState(std::function<void(ulong)> onUpdateStateCallback) {
//  updatedStateCallback = onUpdateStateCallback;
//}

void Socket::initialize(PubSubClient *ptr_mqttClient, String prefix) {
  Device::initialize(ptr_mqttClient, prefix);
}
