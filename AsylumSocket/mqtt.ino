
#if DEVICE_TYPE == 6 // Remote2
void mqtt_sendcommand(char* topic) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, "-1", true);
    Serial.printf(" - message sent [%s] %s \n", topic, "-1");
  }
}
#endif

void mqtt_sendstatus() {
  if (mqttClient.connected()) {
    char payload[64];

    uint8_t mac_int[6];
    WiFi.macAddress(mac_int);
    String mac_str = "";
    for (int i = 0; i < sizeof(mac_int); ++i) {
      mac_str += String(mac_int[i], HEX);
    }

    sprintf(payload, "{\"Type\":%u,\"MAC\":\"%s\",\"IP\":\"%s\"}", DEVICE_TYPE, mac_str.c_str(), WiFi.localIP().toString().c_str());
    mqttClient.publish(device.mqtt_topic_status.c_str(), payload, true);
    Serial.printf(" - message sent [%s] %s \n", device.mqtt_topic_status.c_str(), payload);
  }
}

void mqtt_callback(char* topic, byte* pl, unsigned int length) {
  pl[length] = '\0';
  String payload = String((char*)pl);
	Serial.printf(" - message recieved [%s]: %s \n", topic, payload.c_str());

  device.handlePayload(topic, payload);

	if (strcmp(topic, device.mqtt_topic_setup.c_str()) == 0) {
    Serial.printf(" - setup mode command recieved \n");
    set_mode(2);
	}

	if (strcmp(topic, device.mqtt_topic_reboot.c_str()) == 0) {
    Serial.printf(" - reboot command recieved \n");
    reboot();
	}

  if (strcmp(topic, device.mqtt_topic_erase.c_str()) == 0) {
    Serial.printf(" - erase config command recieved \n");
    //eeprom_erase();
  }
}
