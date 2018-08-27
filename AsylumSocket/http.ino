
void httpserver_setuphandlers() {
  //httpServer.on("/setup.html", []() {
  //  if (!httpServer.authenticate("admin", "admin")) {
  //    return httpServer.requestAuthentication();
  //  }
  //});

  httpServer.serveStatic("/setup.html", SPIFFS, "/setup.html");
  httpServer.serveStatic("/upload.html", SPIFFS, "/upload.html");
  httpServer.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

  httpServer.on("/test_dim", handleTestDim);
  httpServer.on("/submit", handleConfigSave);
  httpServer.on("/api_config", handleApiConfig);
  httpServer.on("/hotspot-detect.html", handleIOS);  //Apple captive portal.

  httpServer.onNotFound(handleRedirect);
}

void handleRedirect() {
  Serial.printf("HTTP-Server: request redirected from: %s \n", httpServer.uri().c_str());

  httpServer.sendHeader("Location", String("http://") + httpServer.client().localIP().toString() + String("/setup.html"), true);
  httpServer.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  httpServer.client().stop(); // Stop is needed because we sent no content length
}

void handleConfigSave() {
  config.state = httpServer.arg("state").toInt();
  config.value = httpServer.arg("value").toInt();
  httpServer.arg("description").toCharArray(config.description, sizeof(config.description) - 1);
  config.mode = httpServer.arg("mode").toInt();
  config.type = httpServer.arg("type").toInt();
  httpServer.arg("apssid").toCharArray(config.apssid, sizeof(config.apssid) - 1);
  if (strcmp(httpServer.arg("apkey").c_str(), "          ") != 0) { httpServer.arg("apkey").toCharArray(config.apkey, sizeof(config.apkey) - 1); }
  httpServer.arg("locallogin").toCharArray(config.locallogin, sizeof(config.locallogin) - 1);
  if (strcmp(httpServer.arg("localpassword").c_str(), "          ") != 0) { httpServer.arg("localpassword").toCharArray(config.localpassword, sizeof(config.localpassword) - 1); }
  httpServer.arg("mqttserver").toCharArray(config.mqttserver, sizeof(config.mqttserver) - 1);
  httpServer.arg("mqttlogin").toCharArray(config.mqttlogin, sizeof(config.mqttlogin) - 1);
  if (strcmp(httpServer.arg("mqttpassword").c_str(), "          ") != 0) { httpServer.arg("mqttpassword").toCharArray(config.mqttpassword, sizeof(config.mqttpassword) - 1); }
  config.extension1 = httpServer.arg("dimmerrangefrom").toInt();
  config.extension2 = httpServer.arg("dimmerrangeto").toInt();
  config.extension3 = httpServer.arg("dimmerstartimpulse").toInt();

  httpServer.send(200, "text/html", "Configutation saved. Rebooting."); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  delay(1000);
  
  Serial.printf("Saving config ... ");
  saveConfig();
  Serial.printf("success \n");
 
  deinitializeSetupMode();
  initializeRegularMode();
  mode = 0;
}

void handleTestDim() {
  int newvalue;
  newvalue = httpServer.arg("value").toInt();
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  httpServer.client().stop(); // Stop is needed because we sent no content length

  Serial.printf("Testing dimmer value (%i) \n", newvalue);
}

void handleApiConfig() {
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  char buffer[1024];
  root["uid"] = uid;
  root["description"] = config.description;
  root["mode"] = config.mode;
  root["type"] = config.type;
  root["apssid"] = config.apssid;
  root["apkey"] = "          ";
  root["locallogin"] = config.locallogin;
  root["localpassword"] = "          ";
  root["mqttserver"] = config.mqttserver;
  root["mqttlogin"] = config.mqttlogin;
  root["mqttpassword"] = "          ";
  root["dimmerrangefrom"] = config.extension1;
  root["dimmerrangeto"] = config.extension2;
  root["dimmerstartimpulse"] = config.extension3;
  root.printTo(buffer, sizeof(buffer));
        
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", buffer);

  Serial.printf("HTTP-Server: config requested \n");
}

void handleIOS() {
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");

  Serial.printf("HTTP-Server: IOS handled by 'SUCCESS': %s \n", httpServer.uri().c_str());
}


