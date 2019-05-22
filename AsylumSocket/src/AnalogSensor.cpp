// AnalogSensor.cpp

#ifdef ARDUINO_AMPERKA_WIFI_SLOT

#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(byte event, byte action, byte sensor) : Device(event, action) {
  pin_sensor = sensor;
};

void AnalogSensor::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  // we need GND and VCC on theese pins to use button (MODE)
  pinMode(A3, OUTPUT); digitalWrite(A3, LOW);
  pinMode(A7, OUTPUT); digitalWrite(A7, HIGH);

  pinMode(pin_sensor, INPUT);
  
  zero_state = ptr_config->cur_conf["extension1"].toInt();
  
  Device::initialize(ptr_mqttClient, ptr_config, prefix);
}

void AnalogSensor::update() {
  // process buttons
  if (buttonState(pin_event, &pin_event_laststate, &pin_event_time) == DOWN) { 
    zero_state = state;
    _config->cur_conf["extension1"] = String(zero_state);
    _config->saveConfig();
    state_published = false;
  }

  // update state of sensor
  if (millis() - time_sensor > INTERVAL_DELAY_SENSOR) {
    time_sensor = millis();
    updateState(analogRead(pin_sensor));
  }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { 
    publishState(mqtt_topic_pub, state - zero_state, &state_published);
  
    //save published value
    state_old = state;

    //blink on publish
    digitalWrite(pin_action, LOW);
    delay(INTERVAL_DELAY_BLINK);
    digitalWrite(pin_action, HIGH);
  }
}

void AnalogSensor::updateState(ulong state_new) {
  if (state_new == state) { return; }

  state = state_new;

  // if new value exceed threshold then publish
  if ((state < state_old && state_old - state > INTERVAL_VALUE_THRESHOLD) || (state > state_old && state - state_old > INTERVAL_VALUE_THRESHOLD)) { state_published = false; }

  debug(" - state changed to %u \n", state_new);
}

#endif