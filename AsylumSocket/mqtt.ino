

void mqtt_check_value_published() {
	if (!mqtt_state_published && millis() - mqtt_state_publishedtime > INTERVAL_MQTT_PUBLISH) {
		mqtt_sendstate();
		mqtt_state_published = true;
		mqtt_state_publishedtime = millis();
	}
}

void mqtt_sendstate() {
	if (mqttClient.connected()) {
		mqttClient.publish(mqtt_topic_pub, String(state).c_str(), true);

    Serial.printf(" - message sent [%s] %s \n", mqtt_topic_pub, String(state).c_str());
		
    mqtt_state_published = true;
		mqtt_state_publishedtime = millis();
	}
}

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
    mqttClient.publish(mqtt_topic_status, payload, true);
    Serial.printf(" - message sent [%s] %s \n", mqtt_topic_status, payload);
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String pl = String((char*)payload);
	Serial.printf(" - message recieved [%s]: %s \n", topic, pl.c_str());

	if (strcmp(topic, mqtt_topic_sub) == 0) {
    ulong newvalue = pl.toInt();
	  Serial.printf(" - value recieved: %u \n", newvalue);

		update_state(newvalue);
		mqtt_sendstate();
	}

	if (strcmp(topic, mqtt_topic_setup) == 0) {
    set_mode(2);
	}

	if (strcmp(topic, mqtt_topic_reboot) == 0) {
		reboot();
	}
}
