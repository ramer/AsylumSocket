
void initializeSetupMode() {
	Serial.println("Entering setup mode.");

	Serial.print("Starting access point ...");
	WiFi.softAPConfig(wifi_AP_IP, wifi_AP_IP, wifi_AP_MASK);
	WiFi.softAP(uid);
	delay(500);
	Serial.print(" started. IP address: ");
	Serial.println(WiFi.softAPIP());

	Serial.print("Starting DNS-server ...");
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(PORT_DNS, "*", wifi_AP_IP);
	Serial.println(" started");

	Serial.print("Mounting SPIFFS ...");
	if (SPIFFS.begin()) {
		Serial.println(" success");
	}
	else {
		Serial.println(" error");
	}

	Serial.print("Binding HTTP-updater ...");
	httpUpdater.setup(&httpServer);
	Serial.println(" done");

	Serial.print("Starting HTTP-server ...");
	httpServer.serveStatic("/setup.html", SPIFFS, "/setup.html");
	httpServer.serveStatic("/upload.html", SPIFFS, "/upload.html");
	httpServer.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

	httpServer.on("/test_dim", handleTestDim);
	httpServer.on("/submit", handleConfigSave);
	httpServer.on("/api_config", handleApiConfig);
	httpServer.on("/hotspot-detect.html", handleIOS);  //Apple captive portal.

	httpServer.onNotFound(handleRedirect);
	httpServer.begin();
	Serial.println(" started");

	setup_mode = true;
}

void deinitializeSetupMode() {
	Serial.print("Closing HTTP-server ...");
	httpServer.stop();
	Serial.println(" closed");

	Serial.print("Unmounting SPIFFS ...");
	SPIFFS.end();
	Serial.println(" success");

	Serial.print("Closing DNS-server ...");
	dnsServer.stop();
	Serial.println(" closed");

	Serial.print("Closing access point ...");
	if (WiFi.softAPdisconnect(true)) {
		Serial.println(" success");
	}
	else {
		Serial.println(" error");
	}

	setup_mode = false;
}

void initializeRegularMode() {
	Serial.println("Entering regular mode.");

	Serial.print("Configuring MQTT-client ...");
	mqttClient.setServer(config.mqttserver, 1883);
	mqttClient.setCallback(mqtt_callback);
	Serial.println(" configured");
}

void deinitializeRegularMode() {
	if (mqttClient.connected()) {
		Serial.print("Closing MQTT-connection ...");
		mqttClient.disconnect();
		Serial.println(" closed");
	}

	if (WiFi.isConnected()) {
		Serial.print("Disconnecting from access point ...");
		if (WiFi.disconnect(true)) {
			Serial.println(" success");
		}
		else {
			Serial.println(" error");
		}
	}
}