// TouchT1.cpp

#include "TouchT1.h"

TouchT1::TouchT1(byte event, byte action, byte event2, byte action2, byte event3, byte action3) : Device(event, action) {
  pin_event2 = event2;
  pin_action2 = action2;
  pin_event3 = event3;
  pin_action3 = action3;
};

//void TouchT1::onUpdatedState(std::function<void(ulong, ulong, ulong)> onUpdateStateCallback) {
//  updatedStateCallback = onUpdateStateCallback;
//}

void TouchT1::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  Device::initialize(ptr_mqttClient, ptr_config, prefix);

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value
  pinMode(pin_event2, INPUT);
  pinMode(pin_action2, OUTPUT);	digitalWrite(pin_action2, LOW);		// default initial value
  pinMode(pin_event3, INPUT);
  pinMode(pin_action3, OUTPUT);	digitalWrite(pin_action3, LOW);		// default initial value

  generateUid(prefix);
  generateGlobalTopics();
  generateTopics();
}

void TouchT1::generateTopics() {
  mqtt_topic_sub = uid + "/pub";
  mqtt_topic_pub = uid + "/sub";
  mqtt_topic_sub2 = uid + "/pub2";
  mqtt_topic_pub2 = uid + "/sub2";
  mqtt_topic_sub3 = uid + "/pub3";
  mqtt_topic_pub3 = uid + "/sub3";
}

void TouchT1::update() {
  // process buttons
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(&state, &state_old, &state_published, pin_action); }
  if (buttonPressed(pin_event2, &pin_event_laststate2)) { invertState(&state2, &state_old2, &state_published2, pin_action2); }
  if (buttonPressed(pin_event3, &pin_event_laststate3)) { invertState(&state3, &state_old3, &state_published3, pin_action3); }

  // check state published
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
  if (_mqttClient->connected() && !state_published2 && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub2, state2, &state_published2); }
  if (_mqttClient->connected() && !state_published3 && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub3, state3, &state_published3); }
}

void TouchT1::handlePayload(char* topic, String payload) {
  if (strcmp(topic, mqtt_topic_sub.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value invert command recieved \n");

      invertState(&state, &state_old, &state_published, pin_action);
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value recieved: %u \n", newvalue);

      updateState(newvalue, &state, &state_old, &state_published, pin_action);
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
  }
  if (strcmp(topic, mqtt_topic_sub2.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value2 invert command recieved \n");

      invertState(&state2, &state_old2, &state_published2, pin_action2);
      publishState(mqtt_topic_pub2, state2, &state_published2); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value2 recieved: %u \n", newvalue);

      updateState(newvalue, &state2, &state_old2, &state_published2, pin_action2);
      publishState(mqtt_topic_pub2, state2, &state_published2); // force
    }
  }
  if (strcmp(topic, mqtt_topic_sub3.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value3 invert command recieved \n");

      invertState(&state3, &state_old3, &state_published3, pin_action3);
      publishState(mqtt_topic_pub3, state3, &state_published3); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value3 recieved: %u \n", newvalue);

      updateState(newvalue, &state3, &state_old3, &state_published3, pin_action3);
      publishState(mqtt_topic_pub3, state3, &state_published3); // force
    }
  }
}

void TouchT1::subscribe() {
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
    _mqttClient->subscribe(mqtt_topic_sub2.c_str());
    _mqttClient->subscribe(mqtt_topic_sub3.c_str());
    _mqttClient->subscribe(mqtt_topic_setup.c_str());
    _mqttClient->subscribe(mqtt_topic_reboot.c_str());
  }
}
