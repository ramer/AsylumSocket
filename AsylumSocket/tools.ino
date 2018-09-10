

void update_state(ulong state_new) {
  state_previous = (state_new > 0 && config.state > 0) ? 0 : config.state;
  config.state = state_new;

#if DEVICE_TYPE == 0 // Socket
  digitalWrite(PIN_ACTION, (config.state == 0 ? LOW : HIGH));
#elif DEVICE_TYPE == 2 // Motor
  digitalWrite(PIN_ACTION_DIR, (config.state == 0 ? LOW : HIGH));
  digitalWrite(PIN_ACTION, HIGH);
  delay(INTERVAL_MOTOR);
  digitalWrite(PIN_ACTION, LOW);
#elif DEVICE_TYPE == 3 // Dimmer
  i2c_sendvalue(config.state);
#elif DEVICE_TYPE == 4 // Strip
  strip_updated_flag = false;
#elif DEVICE_TYPE == 5 // Encoder
  encoderstate = config.state;
  if (config.state >= 250) { digitalWrite(PIN_ACTION, HIGH); }
  else if (config.state > 5 && config.state < 250) { analogWrite(PIN_ACTION, config.state << 2); }  // esp8266 uses 10 bit PWM
  else { digitalWrite(PIN_ACTION, LOW); }
#elif DEVICE_TYPE == 7 // Touch1
  digitalWrite(PIN_ACTION, (config.state == 0 ? LOW : HIGH));
#endif
  
  if (config.onboot == 2 || config.onboot == 3) { save_eeprom(); }

	Serial.printf(" - state changed to %u \n", config.state);
  mqtt_state_published = false;
}

void invert_state() {
  if (config.state == 0) {

#if DEVICE_TYPE == 5 // Encoder
    if (state_previous < 5) { state_previous = 255; }
#else
    if (state_previous == 0) { state_previous = 1; }
#endif

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

  time_mode_set = millis();
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
  topic_temp = uid_temp + "/erase";
  mqtt_topic_erase = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_erase, topic_temp.c_str());
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
  if (mode > 0)
  {
    uint16_t interval = (mode == 1) ? INTERVAL_LED_SETUP : INTERVAL_LED_SMARTCONFIG;

    if (millis() - time_led > interval) {
      time_led = millis();
      laststate_led = !laststate_led;
      digitalWrite(PIN_LED, !laststate_led); // LED circuit inverted
    }
  } else {
    if (millis() - time_led > INTERVAL_LED_SETUP) {
      time_led = millis();
      digitalWrite(PIN_LED, (config.onboardled == 0 ? HIGH : LOW)); // LED circuit inverted
    }
  }
}

void check_newupdate() {
  if (has_new_update) {
    has_new_update = false;
    Serial.printf("\n\nChip restarting.\n\n");
    delay(1000);
    ESP.restart();
  }
}
void check_newconfig() {
  if (has_new_config) {
    has_new_config = false;
    set_mode(0);
  }
}

void check_mode() {
  if (digitalRead(PIN_MODE) == LOW && laststate_mode == false)
  {
    time_mode = millis();
    laststate_mode = true;
  }
  if (digitalRead(PIN_MODE) == HIGH && laststate_mode == true)
  {
    time_mode = millis();
    laststate_mode = false;
  }
  if (laststate_mode == true && millis() - time_mode > INTERVAL_SETUP)
  {
    time_mode = millis();
    Serial.printf("Mode button pressed for %u ms \n", INTERVAL_SETUP);

    set_mode(-1); // next
  }
  if (mode != 0 && millis() - time_mode_set > INTERVAL_MODE_TIMEOUT) { 
    Serial.printf("Setup timeout (%u ms) \n", INTERVAL_MODE_TIMEOUT);

    set_mode(0);
  }
}

String get_quality(int32_t rssi) {
  switch ((rssi + 110) / 10)
  {
  case 5:
    return "▇";
    break;
  case 4:
    return "▆";
    break;
  case 3:
    return "▅";
    break;
  case 2:
    return "▃";
    break;
  default:
    return "▁";
    break;
  }
}

void reboot() {
  if (mode == 0) { deinitializeRegularMode(); }
  if (mode == 1) { deinitializeSmartConfigMode(); }
  if (mode == 2) { deinitializeSetupMode(); }
  
	Serial.printf("\n\nChip restarted.\n\n");
  
  delay(3000);

	ESP.restart();
}

