
void update_state(uint16_t newstate) {
  if (newstate == 0) {
	  digitalWrite(pin_relay, LOW);
	  state = newstate;
	  Serial.println(" - relay turned off");
  }
  else
  {
	  digitalWrite(pin_relay, HIGH);
	  state = newstate;
	  Serial.println(" - relay turned on");
  }
  
  mqtt_value_published = false;
}

boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

String getId() {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);

  String uid_temp = uid_prefix;
  uid_temp += "-";
  
  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i){
    uid_temp += String(MAC_array[i], HEX);
  }  

  return uid_temp;
}

//void i2c_sendvalue(byte value) {
//  Wire.beginTransmission(i2cslaveaddress);
//  Wire.write(value);
//  uint8_t err_code = Wire.endTransmission();
//
//  /*
//  err will be one of:
//   0: success
//   2: address NAK, No slave answered
//   3: data NAK, Slave returned NAK, did not acknowledge reception of a byte
//   4: other error, here is were The Arduino Library sticks any other error.
//       like bus_arbitration_lost,
//  */
//
//  if (err_code == 0) {
//    Serial.print(" - i2c sent: ");
//    Serial.println(value);
//  } else {
//    Serial.print(" - i2c error, code: ");
//    Serial.println(err_code);
//  }
//}

void reboot() {
	if (setup_mode) {
		deinitializeSetupMode();
	}
	else
	{
		deinitializeRegularMode();
	}
  
	Serial.println();
	Serial.println("Chip restarting...");
	Serial.println();
  
	ESP.restart();
}

