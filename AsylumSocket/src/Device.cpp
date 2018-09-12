// Device.cpp

#include "Device.h"

Device::Device(byte event, byte action) {
  pin_event = event;
  pin_action = action;
}

//void Device::onUpdatedState(std::function<void(ulong)> onUpdateStateCallback) {
//  updatedStateCallback = onUpdateStateCallback;
//}

void Device::initialize(PubSubClient *ptr_mqttClient, String prefix) {
  _mqttClient = ptr_mqttClient;

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value

  generateUid(prefix);
  generateGlobalTopics();
  generateTopics();
}

void Device::generateUid(String prefix) {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);

  prefix += "-";

  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i) {
    prefix += String(MAC_array[i], HEX);
  }
  uid = prefix;
}

void Device::generateGlobalTopics() {
  mqtt_topic_status = uid + "/status";
  mqtt_topic_setup = uid + "/setup";
  mqtt_topic_reboot = uid + "/reboot";
  mqtt_topic_erase = uid + "/erase";
}

void Device::generateTopics(){
  mqtt_topic_sub = uid + "/pub";
  mqtt_topic_pub = uid + "/sub";
}

void Device::checkButtons() {
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(&state, &state_old, pin_action); }
}

void Device::updateState(ulong state_new, ulong *ptr_state, ulong *ptr_state_old, byte pin) {
  if (ptr_state != 0 && ptr_state_old != 0) {
    // pointers passed
    *ptr_state_old = (state_new > 0 && *ptr_state > 0) ? 0 : *ptr_state;
    *ptr_state = state_new;
    digitalWrite(pin, (*ptr_state == 0 ? LOW : HIGH));
  }
  else {
    // no pointers passed; take default states
    state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
    state = state_new;
    digitalWrite(pin, (state == 0 ? LOW : HIGH));
  }
  
  Serial.printf(" - state changed to %u \n", state_new);
  mqtt_state_published = false;
}

void Device::invertState(ulong *ptr_state, ulong *ptr_state_old, byte pin) {
  if (ptr_state != 0 && ptr_state_old != 0) {
    // pointers passed
    if (*ptr_state == 0) {
      if (*ptr_state_old == 0) { *ptr_state_old = 1; }
      updateState(*ptr_state_old, ptr_state, ptr_state_old, pin);
    }
    else {
      updateState(0, ptr_state, ptr_state_old, pin);
    }
  }
  else {
    // no pointers passed; take default states
    if (state == 0) {
      if (state_old == 0) { state_old = 1; }
      updateState(state_old, &state, &state_old, pin);
    }
    else {
      updateState(0, &state, &state_old, pin);
    }
  }
}

void Device::handlePayload(char* topic, String payload) {
  if (strcmp(topic, mqtt_topic_sub.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value invert command recieved \n");

      invertState(&state, &state_old, pin_action);
      publishState(mqtt_topic_pub, state); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value recieved: %u \n", newvalue);

      updateState(newvalue, &state, &state_old, pin_action);
      publishState(mqtt_topic_pub, state); // force
    }
  }
}

void Device::subscribe() {
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
    _mqttClient->subscribe(mqtt_topic_setup.c_str());
    _mqttClient->subscribe(mqtt_topic_reboot.c_str());
    _mqttClient->subscribe(mqtt_topic_erase.c_str());
  }
}

void Device::publishState(String t, ulong s) {
  if (_mqttClient->connected()) {
    _mqttClient->publish(t.c_str(), String(s).c_str(), true);

    Serial.printf(" - message sent [%s] %s \n", t.c_str(), String(s).c_str());

    mqtt_state_published = true;
    mqtt_state_publishedtime = millis();
  }
}

void Device::checkPublished() {
  if (!mqtt_state_published && millis() - mqtt_state_publishedtime > INTERVAL_STATE_PUBLISH) {
    publishState(mqtt_topic_pub, state);
    mqtt_state_published = true;
    mqtt_state_publishedtime = millis();
  }
}

bool Device::buttonPressed(byte pin, bool *laststate) {
  bool pressed = false;
  if (digitalRead(pin) == LOW && *laststate == false && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    *laststate = true;

    pressed = true;
  }
  if (digitalRead(pin) == HIGH && *laststate == true && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    *laststate = false;
  }
  return pressed;
}