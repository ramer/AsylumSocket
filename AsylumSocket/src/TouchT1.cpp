// TouchT1.cpp

#include "TouchT1.h"

TouchT1::TouchT1(byte event, byte action, byte event2, byte action2, byte event3, byte action3) : Device(event, action) {
  pin_event2 = event2;
  pin_action2 = action2;
  pin_event3 = event3;
  pin_action3 = action3;
};

void TouchT1::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  pinMode(pin_event2, INPUT);
  pinMode(pin_action2, OUTPUT);	digitalWrite(pin_action2, LOW);		// default initial value
  pinMode(pin_event3, INPUT);
  pinMode(pin_action3, OUTPUT);	digitalWrite(pin_action3, LOW);		// default initial value

  Device::initialize(ptr_mqttClient, ptr_config, prefix);
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
  if (buttonState(pin_event, &pin_event_laststate, &pin_event_time) == DOWN) { invertState(); saveState(); }
  if (buttonState(pin_event2, &pin_event_laststate2, &pin_event_time2) == DOWN) { invertState2(); saveState(); }
  if (buttonState(pin_event3, &pin_event_laststate3, &pin_event_time3) == DOWN) { invertState3(); saveState(); }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
  if (_mqttClient->connected() && !state_published2 && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub2, state2, &state_published2); }
  if (_mqttClient->connected() && !state_published3 && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub3, state3, &state_published3); }
}

void TouchT1::updateState2(ulong state_new) {
  state_old2 = (state_new > 0 && state_old2 > 0) ? 0 : state_old2;
  state2 = state_new;
  digitalWrite(pin_action2, (state_new == 0 ? LOW : HIGH));
  state_published2 = false;

  debug(" - state2 changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void TouchT1::invertState2() {
  if (state2 == 0) {
    if (state_old2 == 0) { state_old2 = 1; }
    updateState2(state_old2);
  }
  else {
    updateState2(0);
  }
}

void TouchT1::updateState3(ulong state_new) {
  state_old3 = (state_new > 0 && state_old3 > 0) ? 0 : state_old3;
  state3 = state_new;
  digitalWrite(pin_action3, (state_new == 0 ? LOW : HIGH));
  state_published3 = false;

  debug(" - state3 changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void TouchT1::invertState3() {
  if (state3 == 0) {
    if (state_old3 == 0) { state_old3 = 1; }
    updateState3(state_old3);
  }
  else {
    updateState3(0);
  }
}
void TouchT1::handlePayload(String topic, String payload) {
  if (topic == mqtt_topic_sub) {
    if (payload == "-1") {
      debug(" - value invert command recieved \n");

      invertState(); saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
    else {
      ulong newvalue = payload.toInt();
      debug(" - value recieved: %u \n", newvalue);

      updateState(newvalue); saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
  }
  if (topic == mqtt_topic_sub2) {
    if (payload == "-1") {
      debug(" - value2 invert command recieved \n");

      invertState2(); saveState();
      publishState(mqtt_topic_pub2, state2, &state_published2); // force
    }
    else {
      ulong newvalue = payload.toInt();
      debug(" - value2 recieved: %u \n", newvalue);

      updateState2(newvalue); saveState();
      publishState(mqtt_topic_pub2, state2, &state_published2); // force
    }
  }
  if (topic == mqtt_topic_sub3) {
    if (payload == "-1") {
      debug(" - value3 invert command recieved \n");

      invertState3(); saveState();
      publishState(mqtt_topic_pub3, state3, &state_published3); // force
    }
    else {
      ulong newvalue = payload.toInt();
      debug(" - value3 recieved: %u \n", newvalue);

      updateState3(newvalue); saveState();
      publishState(mqtt_topic_pub3, state3, &state_published3); // force
    }
  }
}

void TouchT1::subscribe() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
    _mqttClient->subscribe(mqtt_topic_sub2.c_str());
    _mqttClient->subscribe(mqtt_topic_sub3.c_str());
    _mqttClient->subscribe(mqtt_topic_setup.c_str());
    _mqttClient->subscribe(mqtt_topic_reboot.c_str());
  }
}

void TouchT1::loadState() {
  if (!_config) return;
  byte onboot = _config->cur_conf["onboot"].toInt();
  if (onboot == 0) {
    // stay off
    debug(" - onboot: stay off \n");
    updateState(0);
    updateState2(0);
    updateState3(0);
  }
  else if (onboot == 1) {
    // turn on
    debug(" - onboot: turn on \n");
    updateState(1);
    updateState2(1);
    updateState3(1);
  }
  else if (onboot == 2) {
    // saved state
    std::map<String, String> states = _config->loadState();
    debug(" - onboot: last state: %u %u %u \n", states["state"].toInt(), states["state2"].toInt(), states["state3"].toInt());
    updateState(states["state"].toInt());
    updateState2(states["state2"].toInt());
    updateState3(states["state3"].toInt());
  }
  else if (onboot == 3) {
    // inverted saved state
    std::map<String, String> states = _config->loadState();
    if (states["on"].toInt() == 0) {
      debug(" - onboot: inverted state: %u %u %u \n", states["state"].toInt(), states["state2"].toInt(), states["state3"].toInt());
      updateState(states["state"].toInt());
      updateState2(states["state2"].toInt());
      updateState3(states["state3"].toInt());
      saveState();
    }
    else {
      debug(" - onboot: inverted state: 0 0 0\n");
      updateState(0);
      updateState2(0);
      updateState3(0);
      saveState();
    }
  }
}

void TouchT1::saveState() {
  if (!_config) return;
  byte onboot = _config->cur_conf["onboot"].toInt();
  if (onboot == 0 || onboot == 1) return;

  bool on = state || state2 || state3;

  std::map<String, String> states_old = _config->loadState();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["on"] = (int)on;
  if (on) {
    root["state"] = state;
    root["state2"] = state2;
    root["state3"] = state3;
  }
  else {
    root["state"] = states_old["state"];
    root["state2"] = states_old["state2"];
    root["state3"] = states_old["state3"];
  }
  _config->saveState(root);
}

