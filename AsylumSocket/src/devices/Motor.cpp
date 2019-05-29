// Motor.cpp

#ifdef ARDUINO_ESP8266_GENERIC

#include "Motor.h"

Motor::Motor(String prefix, byte event, byte action, byte action2) : Device(prefix, event, action) {
  pin_action2 = action2;
};

void Motor::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config) {
  pinMode(pin_action2, OUTPUT);	digitalWrite(pin_action2, LOW);		// default initial value

  Device::initialize(ptr_mqttClient, ptr_config);
}

void Motor::updateState(ulong state_new) {
  state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
  state = state_new;
  digitalWrite(pin_action2, (state == 0 ? LOW : HIGH));
  state_published = false;

  digitalWrite(pin_action, (state == 0 ? LOW : HIGH));
  digitalWrite(pin_action2, HIGH);
  delay(INTERVAL_MOTOR);
  digitalWrite(pin_action2, LOW);

  debug(" - state changed to %u \n", state_new);
}

#endif