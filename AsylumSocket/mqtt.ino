

void mqtt_check_value_published() {
	if (!mqtt_value_published && millis() - mqtt_value_publishedtime > INTERVAL_MQTT_PUBLISH) {
		mqtt_sendstatus();
		mqtt_value_published = true;
		mqtt_value_publishedtime = millis();
	}
}

void mqtt_sendstatus() {
	if (mqttClient.connected()) {
		mqttClient.publish(mqtt_topic_pub, String(state).c_str(), true);

    Serial.printf(" - message sent [%s] %s \n", mqtt_topic_pub, String(state).c_str());
		
    mqtt_value_published = true;
		mqtt_value_publishedtime = millis();
	}
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
	Serial.printf(" - message recieved [%s]: %s \n", topic, (char *)payload);

	if (strcmp(topic, mqtt_topic_sub) == 0) {
		int newvalue = atoi((char *)payload);
	  Serial.printf(" - value recieved: %s \n", newvalue);

		update_state(newvalue);
		mqtt_sendstatus();
	}

	if (strcmp(topic, mqtt_topic_setup) == 0) {
		deinitializeRegularMode();
		initializeSetupMode();
	}

	if (strcmp(topic, mqtt_topic_reboot) == 0) {
		reboot();
	}
}
