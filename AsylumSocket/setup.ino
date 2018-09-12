
void initializeSetupMode() {
    Serial.printf("\nEntering setup mode.\n\n");

  if (!WiFi.smartConfigDone()) {
    Serial.printf("Starting WiFi client ... ");
    WiFi.mode(WIFI_STA);
    Serial.printf("started \n");

    Serial.printf("Scanning WiFi networks ... ");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true);
    Serial.printf("done. Found %u networks \n", n);

    Serial.printf("Starting access point ... ");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
    WiFi.softAP(device.uid.c_str());
    delay(500);
    Serial.printf("started (%s) \n", WiFi.softAPIP().toString().c_str());
  }

	Serial.printf("Starting DNS-server ... ");
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(PORT_DNS, "*", WiFi.softAPIP());
	Serial.printf("started \n");
}

void deinitializeSetupMode() {
  Serial.printf("Closing DNS-server ... ");
  dnsServer.stop();
  Serial.printf("closed \n");

	Serial.print("Closing access point ... ");
	if (WiFi.softAPdisconnect(true)) {
		Serial.printf("success \n");
	}
	else {
		Serial.printf("error \n");
	}
  Serial.printf("Closing WiFi client ... ");
  if (WiFi.disconnect(true)) {
    Serial.printf("closed \n");
  }
  else {
    Serial.printf("error \n");
  }
}

void initializeSmartConfigMode() {
  Serial.printf("\nEntering Smart Config mode.\n\n");

  Serial.printf("Starting WiFi client ... ");
  WiFi.mode(WIFI_STA);
  Serial.printf("started \n");

  Serial.printf("Scanning WiFi networks ... ");
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true);
  Serial.printf("done. Found %u networks \n", n);

  Serial.printf("Starting Smart Config listener ... ");
  if (WiFi.beginSmartConfig()) {
    Serial.printf("started \n");
  }
  else {
    Serial.printf("error \n");
  }
}

void deinitializeSmartConfigMode() {
  Serial.printf("Closing Smart Config listener ... ");
  if (WiFi.stopSmartConfig()) {
    Serial.printf("closed \n");
  }
  else {
    Serial.printf("error \n");
  }

  Serial.printf("Closing WiFi client ... ");
  if (WiFi.disconnect(true)) {
    Serial.printf("closed \n");
  }
  else {
    Serial.printf("error \n");
  }
}

void initializeRegularMode() {
	Serial.printf("\nEntering regular mode.\n\n");

  Serial.printf("Starting WiFi client ... ");
  WiFi.mode(WIFI_STA);
  Serial.printf("started \n");

  Serial.printf("Scanning WiFi networks ... ");
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true);
  Serial.printf("done. Found %u networks \n", n);

	Serial.printf("Configuring MQTT-client ... ");
	mqttClient.setServer(config["mqttserver"].c_str(), 1883);
	mqttClient.setCallback(mqtt_callback);
	Serial.printf("configured \n");
}

void deinitializeRegularMode() {
	if (mqttClient.connected()) {
		Serial.printf("Closing MQTT-connection ... ");
		mqttClient.disconnect();
		Serial.printf("closed \n");
	}

  if (WiFi.isConnected()) {
		Serial.printf("Disconnecting from access point ... ");
		if (WiFi.disconnect(true)) {
			Serial.printf("success \n");
		}
		else {
			Serial.printf("error \n");
		}
	}
}