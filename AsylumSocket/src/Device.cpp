// Device.cpp

#include "Device.h"

Device::Device(byte event, byte action) {
  pin_event = event;
  pin_action = action;
}

void Device::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  Serial.println("Device::initialize");
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value

  generateUid(prefix);
  generateGlobalTopics();
  generateTopics();

  loadState();
}

void Device::generateUid(String prefix) {
  Serial.println("Device::generateUid");
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);

  prefix += "-";

  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i) {
    prefix += String(MAC_array[i], HEX);
  }
  uid = prefix;
}

void Device::generateGlobalTopics() {
  Serial.println("Device::generateGlobalTopics");
  mqtt_topic_status = uid + "/status";
  mqtt_topic_setup = uid + "/setup";
  mqtt_topic_reboot = uid + "/reboot";
}

void Device::generateTopics(){
  Serial.println("Device::generateTopics");
  mqtt_topic_sub = uid + "/pub";
  mqtt_topic_pub = uid + "/sub";
}

void Device::update() {
  // process buttons
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(); saveState(); }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
}

void Device::updateState(ulong state_new) {
  Serial.println("Device::updateState");
  state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
  state = state_new;
  digitalWrite(pin_action, (state == 0 ? LOW : HIGH));
  state_published = false;

  Serial.printf(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void Device::invertState() {
  Serial.println("Device::invertState");
  if (state == 0) {
    if (state_old == 0) { state_old = 1; }
    updateState(state_old);
  }
  else {
    updateState(0);
  }
}

void Device::handlePayload(String topic, String payload) {
  Serial.println("Device::handlePayload");
  if (topic == mqtt_topic_sub) {
    if (payload == "-1") {
      Serial.printf(" - value invert command recieved \n");

      invertState(); saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
    else {
      ulong newvalue = payload.toInt();
      Serial.printf(" - value recieved: %u \n", newvalue);

      updateState(newvalue); saveState();
      publishState(mqtt_topic_pub, state, &state_published); // force
    }
  }
}

void Device::subscribe() {
  Serial.println("Device::subscribe");
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
    _mqttClient->subscribe(mqtt_topic_setup.c_str());
    _mqttClient->subscribe(mqtt_topic_reboot.c_str());
  }
}

void Device::publishState(String topic, ulong statepayload, bool *ptr_state_published) {
  Serial.println("Device::publishState");
  if (_mqttClient->connected()) {
    _mqttClient->publish(topic.c_str(), String(statepayload).c_str(), true);

    Serial.printf(" - message sent [%s] %s \n", topic.c_str(), String(statepayload).c_str());

    *ptr_state_published = true;
    state_publishedtime = millis();
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

void Device::loadState() {
  Serial.println("Device::loadState");
  if (!_config) return;
  byte onboot = _config->cur_conf["onboot"].toInt();
  if (onboot == 0) {
    // stay off
    Serial.printf(" - onboot: stay off \n");
    updateState(0);
  }
  else if (onboot == 1) {
    // turn on
    Serial.printf(" - onboot: turn on \n");
    updateState(1);
  }
  else if (onboot == 2) {
    // saved state
    std::map<String, String> states = _config->loadState();
    Serial.printf(" - onboot: last state: %u \n", states["state"].toInt());
    updateState(states["state"].toInt());
  }
  else if (onboot == 3) {
    // inverted saved state
    std::map<String, String> states = _config->loadState();
    if (states["on"].toInt() == 0) {
      Serial.printf(" - onboot: inverted state: %u \n", states["state"].toInt());
      updateState(states["state"].toInt());
      saveState();
    }
    else {
      Serial.printf(" - onboot: inverted state: 0 \n");
      updateState(0);
      saveState();
    }
  }
}

void Device::saveState() {
  Serial.println("Device::saveState");
  if (!_config) return;
  byte onboot = _config->cur_conf["onboot"].toInt();
  if (onboot == 0 || onboot == 1) return;

  bool on = state;

  std::map<String, String> states_old = _config->loadState();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["on"] = (int)on;
  if (on) {
    root["state"] = state;
  }
  else {
    root["state"] = states_old["state"];
  }
  _config->saveState(root);
}

//void Device::onUpdateState(std::function<void(ulong)> onUpdateStateCallback) {
//  updateStateCallback = onUpdateStateCallback;
//}
