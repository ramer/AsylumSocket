
void initializeSetupMode() {
    debug("\nEntering setup mode.\n\n");

  if (!WiFi.smartConfigDone()) {
    debug("Starting WiFi client ... ");
    WiFi.mode(WIFI_STA);
    debug("started \n");

    debug("Scanning WiFi networks ... ");
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true);
    debug("done. Found %u networks \n", n);

    debug("Starting access point ... ");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
    WiFi.softAP(device.uid.c_str());
    delay(500);
    debug("started (%s) \n", WiFi.softAPIP().toString().c_str());
  }

	debug("Starting DNS-server ... ");
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(PORT_DNS, "*", WiFi.softAPIP());
	debug("started \n");
}

void deinitializeSetupMode() {
  debug("Closing DNS-server ... ");
  dnsServer.stop();
  debug("closed \n");

	Serial.print("Closing access point ... ");
	if (WiFi.softAPdisconnect(true)) {
		debug("success \n");
	}
	else {
		debug("error \n");
	}
  debug("Closing WiFi client ... ");
  if (WiFi.disconnect(true)) {
    debug("closed \n");
  }
  else {
    debug("error \n");
  }
}

void initializeSmartConfigMode() {
  debug("\nEntering Smart Config mode.\n\n");

  debug("Starting WiFi client ... ");
  WiFi.mode(WIFI_STA);
  debug("started \n");

  debug("Scanning WiFi networks ... ");
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true);
  debug("done. Found %u networks \n", n);

  debug("Starting Smart Config listener ... ");
  if (WiFi.beginSmartConfig()) {
    debug("started \n");
  }
  else {
    debug("error \n");
  }
}

void deinitializeSmartConfigMode() {
  debug("Closing Smart Config listener ... ");
  if (WiFi.stopSmartConfig()) {
    debug("closed \n");
  }
  else {
    debug("error \n");
  }

  debug("Closing WiFi client ... ");
  if (WiFi.disconnect(true)) {
    debug("closed \n");
  }
  else {
    debug("error \n");
  }
}

void initializeRegularMode() {
	debug("\nEntering regular mode.\n\n");

  debug("Starting WiFi client ... ");
  WiFi.mode(WIFI_STA);
  debug("started \n");

  debug("Scanning WiFi networks ... ");
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true);
  debug("done. Found %u networks \n", n);

	debug("Configuring MQTT-client ... ");
	mqttClient.setServer(config.current["mqttserver"].c_str(), 1883);
	mqttClient.setCallback(mqtt_callback);
	debug("configured \n");
}

void deinitializeRegularMode() {
	if (mqttClient.connected()) {
		debug("Closing MQTT-connection ... ");
		mqttClient.disconnect();
		debug("closed \n");
	}

  if (WiFi.isConnected()) {
		debug("Disconnecting from access point ... ");
		if (WiFi.disconnect(true)) {
			debug("success \n");
		}
		else {
			debug("error \n");
		}
	}
}