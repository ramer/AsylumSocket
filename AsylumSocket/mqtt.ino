
#if DEVICE_TYPE == 6 // Remote2
void mqtt_sendcommand(char* topic) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, "-1", true);
    debug(" - message sent [%s] %s \n", topic, "-1");
  }
}
#endif

void mqtt_sendstatus() {
  if (mqttClient.connected()) {
    char payload[MQTT_MAX_PACKET_SIZE];

    uint8_t mac_int[6];
    WiFi.macAddress(mac_int);
    String mac_str = "";
    for (int i = 0; i < sizeof(mac_int); ++i) {
      mac_str += String(mac_int[i], HEX);
    }

    snprintf(payload, sizeof(payload), "{\"Type\":%u,\"MAC\":\"%s\",\"IP\":\"%s\"}", DEVICE_TYPE, mac_str.c_str(), WiFi.localIP().toString().c_str());
    mqttClient.publish(device.mqtt_topic_status.c_str(), payload, true);
    debug(" - message sent [%s] %s \n", device.mqtt_topic_status.c_str(), payload);
  }
}

void mqtt_callback(char* tp, byte* pl, unsigned int length) {
  pl[length] = '\0';
  String payload = String((char*)pl);
  String topic = String(tp);

	debug(" - message recieved [%s]: %s \n", topic.c_str(), payload.c_str());

  device.handlePayload(topic, payload);

	if (topic == device.mqtt_topic_setup) {
    debug(" - setup mode command recieved \n");
    set_mode(2);
	}

	if (topic == device.mqtt_topic_reboot) {
    debug(" - reboot command recieved \n");
    reboot();
	}
}
