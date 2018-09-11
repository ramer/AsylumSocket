// TouchT1.cpp

#include "TouchT1.h"

TouchT1::TouchT1(byte event, byte action, byte event2, byte action2, byte event3, byte action3) : Device(event, action) {
  pin_event2 = event2;
  pin_action2 = action2;
  pin_event3 = event3;
  pin_action3 = action3;
};

void TouchT1::init(PubSubClient * mqttClient)
{
  TouchT1::initialize("TouchT1", mqttClient);
}

void TouchT1::initialize(String prefix, PubSubClient *mqttClient) {
  _mqttClient = mqttClient;

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

void TouchT1::checkButtons() {
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(&state, &state_old, pin_action); }
  if (buttonPressed(pin_event2, &pin_event_laststate2)) { invertState(&state2, &state2_old, pin_action2); }
  if (buttonPressed(pin_event3, &pin_event_laststate3)) { invertState(&state3, &state3_old, pin_action3); }
}

void TouchT1::handlePayload(char* topic, String payload) {
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
  if (strcmp(topic, mqtt_topic_sub2.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value2 invert command recieved \n");

      invertState(&state2, &state2_old, pin_action2);
      publishState(mqtt_topic_pub2, state2); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value2 recieved: %u \n", newvalue);

      updateState(newvalue, &state2, &state2_old, pin_action2);
      publishState(mqtt_topic_pub2, state2); // force
    }
  }
  if (strcmp(topic, mqtt_topic_sub3.c_str()) == 0) {
    if (payload == "-1") {
      Serial.printf(" - value3 invert command recieved \n");

      invertState(&state3, &state3_old, pin_action3);
      publishState(mqtt_topic_pub3, state3); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value3 recieved: %u \n", newvalue);

      updateState(newvalue, &state3, &state3_old, pin_action3);
      publishState(mqtt_topic_pub3, state3); // force
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
    _mqttClient->subscribe(mqtt_topic_erase.c_str());
  }
}

void TouchT1::checkPublished() {
  if (!mqtt_state_published && millis() - mqtt_state_publishedtime > INTERVAL_STATE_PUBLISH) {
    publishState(mqtt_topic_pub, state);
    publishState(mqtt_topic_pub2, state2);
    publishState(mqtt_topic_pub3, state3);
    mqtt_state_published = true;
    mqtt_state_publishedtime = millis();
  }
}