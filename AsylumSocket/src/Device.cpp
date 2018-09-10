// Device.cpp

#include "Device.h"
#include <PubSubClient.h>

Device::Device(byte pin_event, byte pin_action) {
  _pin_event = pin_event;
  _pin_action = pin_action;
}

void Device::initialize(String prefix, PubSubClient *mqttClient) {
  _mqttClient = mqttClient;

  pinMode(_pin_event, INPUT);
  pinMode(_pin_action, OUTPUT);	digitalWrite(_pin_action, LOW);		// default initial value

  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);

  prefix += "-";

  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i) {
    prefix += String(MAC_array[i], HEX);
  }

  uid = new char[prefix.length() + 1]; strcpy(uid, prefix.c_str());

  generateglobaltopics(prefix);
  generatetopics(prefix);
}

void Device::generateglobaltopics(String id) {
  String topic_temp;
  topic_temp = id + "/status";
  mqtt_topic_status = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_status, topic_temp.c_str());
  topic_temp = id + "/setup";
  mqtt_topic_setup = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_setup, topic_temp.c_str());
  topic_temp = id + "/reboot";
  mqtt_topic_reboot = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_reboot, topic_temp.c_str());
  topic_temp = id + "/erase";
  mqtt_topic_erase = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_erase, topic_temp.c_str());
}

void Device::generatetopics(String id){
  String topic_temp;
  topic_temp = id + "/pub";
  mqtt_topic_sub = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_sub, topic_temp.c_str());
  topic_temp = id + "/sub";
}

void Device::check_buttons() {
  if (digitalRead(_pin_event) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(_pin_event) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
}

void Device::update_state(ulong state_new) {
  _state = (state_new > 0 && state > 0) ? 0 : state;
  state = state_new;

  digitalWrite(_pin_action, (state == 0 ? LOW : HIGH));

  Serial.printf(" - state changed to %u \n", state_new);
  mqtt_state_published = false;
}

void Device::invert_state() {
  if (state == 0) {
    if (_state == 0) { _state = 1; }
    update_state(_state);
  }
  else {
    update_state(0);
  }
}

void Device::handlePayload(char* topic, String payload) {
  if (strcmp(topic, mqtt_topic_sub) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value invert command recieved \n");

      invert_state();
      publishstate(); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value recieved: %u \n", newvalue);

      update_state(newvalue);
      publishstate(); // force
    }
  }
}

void Device::publishstate() {
  if (_mqttClient->connected()) {
    _mqttClient->publish(mqtt_topic_pub, String(state).c_str(), true);

    Serial.printf(" - message sent [%s] %s \n", mqtt_topic_pub, String(state).c_str());

    mqtt_state_published = true;
    mqtt_state_publishedtime = millis();
  }
}

void Device::check_published() {
  if (!mqtt_state_published && millis() - mqtt_state_publishedtime > INTERVAL_STATE_PUBLISH) {
    publishstate();
    mqtt_state_published = true;
    mqtt_state_publishedtime = millis();
  }
}

