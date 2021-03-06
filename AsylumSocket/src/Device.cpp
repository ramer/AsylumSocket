// Device.cpp

#include "Device.h"

Device::Device(String prefix, byte event, byte action) {
  uid_prefix = prefix;
  pin_event = event;
  pin_action = action;
}

Device::~Device(){}

void Device::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config) {
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value

  pin_event_average = !digitalRead(pin_event);  // inverted initial value

  generateUid();

  loadState();
}

void Device::generateUid() {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);
  uid = uid_prefix + "-";
  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i) uid += String(MAC_array[i], HEX);
  mqtt_topic_status = uid + "/status";
  mqtt_topic_setup = uid + "/setup";
  mqtt_topic_reboot = uid + "/reboot";
  mqtt_topic_sub = uid + "/pub";
  mqtt_topic_pub = uid + "/sub";
}

void Device::update() {
  // process buttons
  if (buttonState(pin_event, &pin_event_laststate, &pin_event_average, &pin_event_time) == DOWN) { invertState(); saveState(); }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
}

void Device::updateState(ulong state_new) {
  state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
  state = state_new;
  digitalWrite(pin_action, (state == 0 ? LOW : HIGH));
  state_published = false;

  debug(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}

void Device::invertState() {
  if (state == 0) {
    if (state_old == 0) { state_old = 1; }
    updateState(state_old);
  }
  else {
    updateState(0);
  }
}

void Device::handlePayload(String topic, String payload) {
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
}

void Device::subscribe() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->subscribe(mqtt_topic_sub.c_str());
    _mqttClient->subscribe(mqtt_topic_setup.c_str());
    _mqttClient->subscribe(mqtt_topic_reboot.c_str());
  }
}

void Device::publishState(String topic, ulong statepayload, bool *ptr_state_published) {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    _mqttClient->publish(topic.c_str(), String(statepayload).c_str(), true);

    debug(" - message sent [%s] %s \n", topic.c_str(), String(statepayload).c_str());

    *ptr_state_published = true;
    state_publishedtime = millis();
  }
}

void Device::publishStatus() {
  if (!_mqttClient) return;
  if (_mqttClient->connected()) {
    char payload[MQTT_MAX_PACKET_SIZE];

    uint8_t mac_int[6];
    WiFi.macAddress(mac_int);
    String mac_str = "";
    for (int i = 0; i < sizeof(mac_int); ++i) {
      mac_str += String(mac_int[i], HEX);
    }

    snprintf(payload, sizeof(payload), "{\"MAC\":\"%s\",\"IP\":\"%s\"}", mac_str.c_str(), WiFi.localIP().toString().c_str());
    _mqttClient->publish(mqtt_topic_status.c_str(), payload, true);
      
    debug(" - message sent [%s] %s \n", mqtt_topic_status.c_str(), payload);
  }
}

buttonstates Device::buttonState(byte pin, bool *ptr_pin_laststate, float *ptr_pin_average, ulong *ptr_pin_time) {
  if (ptr_pin_laststate == 0 || ptr_pin_time == 0) return RELEASED;

  *ptr_pin_average = (*ptr_pin_average * (DEBOUNCE_AVERAGE - 1) + !digitalRead(pin)) / DEBOUNCE_AVERAGE;  // average inverted

  ulong pintimedelta = millis() - *ptr_pin_time;

  if (*ptr_pin_average > 0.8 && !*ptr_pin_laststate && pintimedelta > INTERVAL_BUTTON_DEBOUNCE)
  {
    *ptr_pin_time = millis(); *ptr_pin_laststate = true; return DOWN;
  }

  if (*ptr_pin_average > 0.8 && *ptr_pin_laststate)
  {
    return pintimedelta > INTERVAL_BUTTON_IDLE ? PRESSED : DOWNIDLE;
  }

  if (*ptr_pin_average < 0.2 && *ptr_pin_laststate && pintimedelta > INTERVAL_BUTTON_DEBOUNCE)
  {
    *ptr_pin_time = millis(); *ptr_pin_laststate = false; return UP;
  }

  if (*ptr_pin_average < 0.2 && !*ptr_pin_laststate)
  {
    return pintimedelta > INTERVAL_BUTTON_IDLE ? RELEASED : UPIDLE;
  }

  return RELEASED;
}

void Device::loadState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
  if (onboot == 0) {
    // stay off
    debug(" - onboot: stay off \n");
    updateState(0);
  }
  else if (onboot == 1) {
    // turn on
    debug(" - onboot: turn on \n");
    updateState(1);
  }
  else if (onboot == 2) {
    // saved state
    std::map<String, String> states = _config->loadState();
    debug(" - onboot: last state: %u \n", states["state"].toInt());
    updateState(states["state"].toInt());
  }
  else if (onboot == 3) {
    // inverted saved state
    std::map<String, String> states = _config->loadState();
    if (states["on"].toInt() == 0) {
      debug(" - onboot: inverted state: %u \n", states["state"].toInt());
      updateState(states["state"].toInt());
      saveState();
    }
    else {
      debug(" - onboot: inverted state: 0 \n");
      updateState(0);
      saveState();
    }
  }
}

void Device::saveState() {
  if (!_config) return;
  byte onboot = _config->current["onboot"].toInt();
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
