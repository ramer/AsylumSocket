// Encoder.cpp

#include "Encoder.h"

Encoder::Encoder(byte event, byte action, byte event2) : Device(event, action) {
  pin_event2 = event2;
};

void Encoder::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  // Device::initialize(ptr_mqttClient, ptr_config, prefix);     // fully overrided
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value
  pinMode(pin_event2, INPUT);

  generateUid(prefix);
  generateGlobalTopics();
  generateTopics();

  loadState();
}

void Encoder::update() {
  // process encoder
  if (digitalRead(pin_event) == digitalRead(pin_event2)) { // CCW
    if (encoderstate > 0) { 
      encoderstate--;
      updateState(encoderstate, &state, &state_old, &state_published, pin_action); saveState();
    }
  }
  else { // CW
    if (encoderstate < 255) { 
      encoderstate++; 
      updateState(encoderstate, &state, &state_old, &state_published, pin_action); saveState();
    }
  }
  
  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
}

void Encoder::updateState(ulong state_new, ulong *ptr_state, ulong *ptr_state_old, bool *ptr_state_published, byte pin) {
  if (ptr_state != 0 && ptr_state_old != 0 && ptr_state_published != 0) {
    // pointers passed
    *ptr_state_old = (state_new > 0 && *ptr_state > 0) ? 0 : *ptr_state;
    *ptr_state = state_new;
    analogWrite(pin, *ptr_state);
    *ptr_state_published = false;
  }
  else {
    // no pointers passed; take default states
    state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
    state = state_new;
    analogWrite(pin, state);
    state_published = false;
  }

  Serial.printf(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}
