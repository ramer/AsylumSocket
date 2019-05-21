// AnalogSensor.cpp

#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(byte event, byte action) : Device(event, action) {

};

void AnalogSensor::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  // we need GND and VCC on theese pins to use button (MODE)
  pinMode(A3, OUTPUT); digitalWrite(A3, LOW);
  pinMode(A7, OUTPUT); digitalWrite(A7, HIGH);

  Device::initialize(ptr_mqttClient, ptr_config, prefix);
}

void AnalogSensor::update() {
  // update state of sensor
  if (millis() - time_sensor > INTERVAL_DELAY_SENSOR) {
    time_sensor = millis();
    updateState(analogRead(pin_event));
  }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { 
    publishState(mqtt_topic_pub, state, &state_published);
  
    //blink on publish
    digitalWrite(pin_action, LOW);
    delay(INTERVAL_DELAY_BLINK);
    digitalWrite(pin_action, HIGH);
  }
}

void AnalogSensor::updateState(ulong state_new) {
  if (state_new == state) return;

  state = state_new;
  state_published = false;

  debug(" - state changed to %u \n", state_new);
}