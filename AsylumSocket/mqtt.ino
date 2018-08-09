

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
		Serial.print(" - message sent ["); Serial.print(mqtt_topic_pub); Serial.print("]: ");
		Serial.println(state);
		mqtt_value_published = true;
		mqtt_value_publishedtime = millis();
	}
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
	Serial.print(" - message recieved ["); Serial.print(topic); Serial.print("]: ");
	char buffer[length];
	for (int i = 0; i < length; i++) {
		buffer[i] = (char)payload[i];
		Serial.print((char)payload[i]);
	}
	Serial.println();

	if (strcmp(topic, mqtt_topic_sub) == 0) {
		int newvalue = atoi(buffer);
		Serial.print(" - value recieved: ");
		Serial.println(newvalue, DEC);

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
