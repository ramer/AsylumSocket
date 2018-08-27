

void update_state(ulong state_new) {

#if DEVICE_TYPE == 3
  i2c_sendvalue(state_new);
#elif DEVICE_TYPE == 4
  strip_updated_flag = false;
#else
  if (state_new == 0) {
    digitalWrite(PIN_ACTION, LOW);
  }
  else
  {
    digitalWrite(PIN_ACTION, HIGH);
  }
#endif

  state_previous = (state_new > 0 && state > 0) ? 0 : state ;
	state = state_new;
	Serial.printf(" - state changed to %u \n", state);
  
  mqtt_state_published = false;
}

void invert_state() {
  if (state == 0) {
    if (state_previous == 0) { state_previous = 1; }
    update_state(state_previous);
  }
  else {
    update_state(0);
  }
}

void set_mode(int8_t mode_new) {
  if (mode == -1 && mode_new == -1) { mode_new = 0; Serial.printf("Unspecified mode! /n"); } //impossible
  
  if (mode == 0) { deinitializeRegularMode(); }
  if (mode == 1) { deinitializeSmartConfigMode(); }
  if (mode == 2) { deinitializeSetupMode(); }

  if (mode_new == -1) {  // goto for next mode
    mode++;
    if (mode > 2) { mode = 0; }
  } else {
    mode = mode_new;
  }

  if (mode == 0) { initializeRegularMode(); }
  if (mode == 1) { initializeSmartConfigMode(); }
  if (mode == 2) { initializeSetupMode(); }
}

void resolve_identifiers() {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);

  String uid_temp = DEVICE_PREFIX;
  uid_temp += "-";

  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i){
    uid_temp += String(MAC_array[i], HEX);
  }  

  uid = new char[uid_temp.length() + 1]; strcpy(uid, uid_temp.c_str());

  String topic_temp;
  topic_temp = uid_temp + "/pub";
  mqtt_topic_sub = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_sub, topic_temp.c_str());
  topic_temp = uid_temp + "/sub";
  mqtt_topic_pub = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_pub, topic_temp.c_str());
  topic_temp = uid_temp + "/status";
  mqtt_topic_status = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_status, topic_temp.c_str());
  topic_temp = uid_temp + "/setup";
  mqtt_topic_setup = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_setup, topic_temp.c_str());
  topic_temp = uid_temp + "/reboot";
  mqtt_topic_reboot = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_reboot, topic_temp.c_str());
}


#if DEVICE_TYPE == 3
void i2c_sendvalue(uint16_t value) {
  Wire.beginTransmission(ADDRESS_I2C_SLAVE);
  Wire.write(value);
  
  switch (Wire.endTransmission())
  {
  case 0:
    Serial.printf(" - i2c sent (%u)", value);
    break;
  case 2:
    Serial.printf(" - i2c send (%u) error: address NAK, no slave answered", value);
    break;
  case 3:
    Serial.printf(" - i2c send (%u) error: data NAK, slave returned NAK, did not acknowledge reception of a byte", value);
    break;
  default:
    Serial.printf(" - i2c send (%u) unknown error", value);
    break;
  }
}
#endif

void blynk() {
  if (mode > 0 || laststate_led)
  {
    uint16_t interval = (mode == 1) ? INTERVAL_LED_SETUP : INTERVAL_LED_SMARTCONFIG;

    if (millis() - time_led > interval) {
      time_led = millis();
      laststate_led = !laststate_led;
      digitalWrite(PIN_LED, !laststate_led); // LED circuit inverted
    }
  }
}

void reboot() {
	if (mode == 1) {
		deinitializeSetupMode();
	}
  else if (mode == 2) {
    deinitializeSmartConfigMode();
  }
	else
	{
		deinitializeRegularMode();
	}
  
	Serial.printf("\n\nChip restarted.\n\n");
  
	ESP.restart();
}

