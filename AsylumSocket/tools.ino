

void update_state(uint16_t newstate) {
  if (newstate == 0) {
	  digitalWrite(PIN_ACTION, LOW);
	  state = newstate;
	  Serial.println(" - relay turned off");
  }
  else
  {
	  digitalWrite(PIN_ACTION, HIGH);
	  state = newstate;
	  Serial.println(" - relay turned on");
  }
  
  mqtt_value_published = false;
}

//boolean isIp(String str) {
//  for (int i = 0; i < str.length(); i++) {
//    int c = str.charAt(i);
//    if (c != '.' && (c < '0' || c > '9')) {
//      return false;
//    }
//  }
//  return true;
//}
//
//String toStringIp(IPAddress ip) {
//  String res = "";
//  for (int i = 0; i < 3; i++) {
//    res += String((ip >> (8 * i)) & 0xFF) + ".";
//  }
//  res += String(((ip >> 8 * 3)) & 0xFF);
//  return res;
//}

void ResolveIdentifiers() {
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
  topic_temp = uid_temp + "/setup";
  mqtt_topic_setup = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_setup, topic_temp.c_str());
  topic_temp = uid_temp + "/reboot";
  mqtt_topic_reboot = new char[topic_temp.length() + 1]; strcpy(mqtt_topic_reboot, topic_temp.c_str());
}

//void i2c_sendvalue(byte value) {
//  Wire.beginTransmission(ADDRESS_I2C_SLAVE);
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

/*
* SerialPrintf
* Реализует функциональность printf в Serial.print
* Применяется для отладочной печати
* Параметры как у printf
* Возвращает
*		0 - ошибка формата
*		отрицательное чило - нехватка памяти, модуль числа равен запрашиваемой памяти
*		положительное число - количество символов, выведенное в Serial
*/
//const size_t SerialPrintf(const char *szFormat, ...) {
//  va_list argptr;
//  va_start(argptr, szFormat);
//  char *szBuffer = 0;
//  const size_t nBufferLength = vsnprintf(szBuffer, 0, szFormat, argptr) + 1;
//  if (nBufferLength == 1) return 0;
//  szBuffer = (char *)malloc(nBufferLength);
//  if (!szBuffer) return -nBufferLength;
//  vsnprintf(szBuffer, nBufferLength, szFormat, argptr);
//  Serial.print(szBuffer);
//  free(szBuffer);
//  return nBufferLength - 1;
//}


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
  
	Serial.println();
	Serial.println("Chip restarted.");
	Serial.println();
  
	ESP.restart();
}

