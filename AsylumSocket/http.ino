
void httpserver_setuphandlers() {
  httpServer.serveStatic("/", SPIFFS, "/").setDefaultFile("setup.html");
  httpServer.serveStatic("/setup.html", SPIFFS, "/setup.html");
  httpServer.serveStatic("/upload.html", SPIFFS, "/upload.html");
  httpServer.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

  httpServer.on("/test_dim", HTTP_POST, handleTestDim);
  httpServer.on("/submit", HTTP_POST, handleConfigSave);
  httpServer.on("/api_config", HTTP_GET, handleApiConfig);
  httpServer.on("/update", HTTP_POST, handleUpdate, handleFileUpload);
  
  httpServer.onNotFound(handleRedirect);
}

void handleUpdate(AsyncWebServerRequest *request) {
  has_new_update = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", has_new_update ? "<HTML><BODY><H3>Firmware updated</h3></BODY></HTML>" : "<HTML><BODY><H3>Firmware update failure</h3></BODY></HTML>");
  response->addHeader("Connection", "close");
  request->send(response);
}

void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("Upload started: %s \n", filename.c_str());
    Update.runAsync(true);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (Update.begin(maxSketchSpace)) {
      Serial.printf("Updating firmware ... ");
    } else {
      Serial.printf("Update begin failure: ");
      Update.printError(Serial);
    }
  }
  
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Serial.printf("write error: ");
      Update.printError(Serial);
      Serial.printf(" \n");
    }
  }

  if (final) {
    if (Update.end(true)) {
      Serial.printf("done. \n");
    }
    else {
      Serial.printf("Update end failure: ");
      Update.printError(Serial);
    }
  }
}

void handleTestDim(AsyncWebServerRequest *request) {
  int newvalue;
  if (request->hasParam("value", true)) {
    newvalue = request->getParam("value", true)->value().toInt();
    Serial.printf("Testing dimmer value (%i) \n", newvalue);
    request->send(200, "text/html", "");
  }
}

void handleConfigSave(AsyncWebServerRequest *request) {
  if (request->hasParam("state", true)) {
    config.state = request->getParam("state", true)->value().toInt();
  }
  if (request->hasParam("value", true)) {
    config.value = request->getParam("value", true)->value().toInt();
  }
  if (request->hasParam("description", true)) {
    request->getParam("description", true)->value().toCharArray(config.description, sizeof(config.description) - 1);
  }
  if (request->hasParam("mode", true)) {
    config.mode = request->getParam("mode", true)->value().toInt();
  }
  if (request->hasParam("type", true)) {
    config.type = request->getParam("type", true)->value().toInt();
  }
  if (request->hasParam("apssid", true)) {
    request->getParam("apssid", true)->value().toCharArray(config.apssid, sizeof(config.apssid) - 1);
  }
  if (request->hasParam("apkey", true) && strcmp(request->getParam("apkey", true)->value().c_str(), "          ") != 0) {
    request->getParam("apkey", true)->value().toCharArray(config.apkey, sizeof(config.apkey) - 1);
  }
  if (request->hasParam("locallogin", true)) {
    request->getParam("locallogin", true)->value().toCharArray(config.locallogin, sizeof(config.locallogin) - 1);
  }
  if (request->hasParam("localpassword", true) && strcmp(request->getParam("localpassword", true)->value().c_str(), "          ") != 0) {
    request->getParam("localpassword", true)->value().toCharArray(config.localpassword, sizeof(config.localpassword) - 1);
  }
  if (request->hasParam("mqttserver", true)) {
    request->getParam("mqttserver", true)->value().toCharArray(config.mqttserver, sizeof(config.mqttserver) - 1);
  }
  if (request->hasParam("mqttlogin", true)) {
    request->getParam("mqttlogin", true)->value().toCharArray(config.mqttlogin, sizeof(config.mqttlogin) - 1);
  }
  if (request->hasParam("mqttpassword", true) && strcmp(request->getParam("mqttpassword", true)->value().c_str(), "          ") != 0) {
    request->getParam("mqttpassword", true)->value().toCharArray(config.mqttpassword, sizeof(config.mqttpassword) - 1);
  }
  if (request->hasParam("dimmerrangefrom", true)) {
    config.extension1 = request->getParam("dimmerrangefrom", true)->value().toInt();
  }
  if (request->hasParam("dimmerrangeto", true)) {
    config.extension2 = request->getParam("dimmerrangeto", true)->value().toInt();
  }
  if (request->hasParam("dimmerstartimpulse", true)) {
    config.extension3 = request->getParam("dimmerstartimpulse", true)->value().toInt();
  }

  request->send(200, "text/html", "Configuration saved.");

  Serial.printf("Saving config ... ");
  saveConfig();
  Serial.printf("success \n");

  has_new_config = true;
}

void handleApiConfig(AsyncWebServerRequest *request) {
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
        
  request->send(200, "text/html", buffer);

  Serial.printf("HTTP-Server: config requested \n");
}

void handleRedirect(AsyncWebServerRequest *request) {
  Serial.printf("HTTP-Server: request redirected from: %s \n", request->url().c_str());
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
  response->addHeader("Location", String("http://") + (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + String("/setup.html"));
  request->send(response);
}
