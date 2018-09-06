
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void httpserver_setuphandlers() {
  httpServer.serveStatic("/", SPIFFS, "/").setDefaultFile("setup.html");
  httpServer.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

  httpServer.on("/submit", HTTP_POST, handleConfigSave);
  httpServer.on("/api_config", HTTP_GET, handleApiConfig);
  httpServer.on("/upload", HTTP_GET, handleUpload);
  httpServer.on("/update", HTTP_POST, handleUpdate, handleFileUpload);
  
  httpServer.onNotFound(handleRedirect);
}


void handleUpdate(AsyncWebServerRequest *request) {
  bool update_spiffs = false;

  if (request->hasParam("spiffs", true)) {
    update_spiffs = (request->getParam("spiffs", true)->value() == "1");
  }

  has_new_update = !update_spiffs && !Update.hasError();
  request->send_P(200, "text/html", Update.hasError() ? updatefailure_html : updatesuccess_html);
}

void handleUpload(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", upload_html);
}

void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  bool update_spiffs = false;

  if (request->hasParam("spiffs", true)) {
    update_spiffs = (request->getParam("spiffs", true)->value() == "1");
  }

  if (index == 0) {
    Serial.printf("Upload started: %s \n", filename.c_str());
    Update.runAsync(true);

    if (update_spiffs) {
      size_t spiffsSize = ((size_t)&_SPIFFS_end - (size_t)&_SPIFFS_start);
      if (Update.begin(spiffsSize, U_SPIFFS)) {
        Serial.printf("Updating SPIFFS ... ");
      }
      else {
        Serial.printf("Update SPIFFS begin failure: ");
        Update.printError(Serial);
      }
    } else {
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (Update.begin(maxSketchSpace)) {
        Serial.printf("Updating firmware ... ");
      }
      else {
        Serial.printf("Update firmware begin failure: ");
        Update.printError(Serial);
      }
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

void handleConfigSave(AsyncWebServerRequest *request) {
  if (request->hasParam("description", true)) {
    request->getParam("description", true)->value().toCharArray(config.description, sizeof(config.description) - 1);
  }
  if (request->hasParam("mode", true)) {
    config.mode = request->getParam("mode", true)->value().toInt();
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
  if (request->hasParam("extension1", true)) {
    request->getParam("extension1", true)->value().toCharArray(config.extension1, sizeof(config.extension1) - 1);
  }
  if (request->hasParam("extension2", true)) {
    request->getParam("extension2", true)->value().toCharArray(config.extension2, sizeof(config.extension2) - 1);
  }
  if (request->hasParam("extension3", true)) {
    request->getParam("extension3", true)->value().toCharArray(config.extension3, sizeof(config.extension3) - 1);
  }

  request->send(200, "text/html", "Configuration saved.");

  Serial.printf("Saving config ... ");
  saveConfig();
  Serial.printf("success \n");

  has_new_config = true;
}

void handleApiConfig(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["uid"] = uid;
  root["description"] = config.description;
  root["mode"] = config.mode;
  root["type"] = DEVICE_TYPE;
  root["apssid"] = config.apssid;
  root["apkey"] = "          ";
  root["locallogin"] = config.locallogin;
  root["localpassword"] = "          ";
  root["mqttserver"] = config.mqttserver;
  root["mqttlogin"] = config.mqttlogin;
  root["mqttpassword"] = "          ";
  root["extension1"] = config.extension1;
  root["extension2"] = config.extension2;
  root["extension3"] = config.extension3;

  JsonArray& networks = root.createNestedArray("networks");

  int n = WiFi.scanComplete();
  if (n > 0) {
    int8_t lastmax = 0;

    for (int j = 0; j < n; j++) {
      int8_t max = -100;
      
      for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(i) > max && WiFi.RSSI(i) < lastmax) { max = WiFi.RSSI(i); };
      }

      lastmax = max;

      for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(i) == max) {
          JsonObject& network = networks.createNestedObject();
          network["ssid"] = WiFi.SSID(i);
          network["name"] = (WiFi.encryptionType(i) == ENC_TYPE_NONE ? "🔓 " : "🔒 ") + get_quality(WiFi.RSSI(i)) + " &emsp; " + WiFi.SSID(i) + " &emsp; " + (WiFi.isHidden(i) ? "👁" : " ");
        }
      }
    }
  }

  root.printTo(*response);
  request->send(response);

  Serial.printf("HTTP-Server: config requested \n");
}

void handleRedirect(AsyncWebServerRequest *request) {
  Serial.printf("HTTP-Server: request redirected from: %s \n", request->url().c_str());
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
  response->addHeader("Location", String("http://") + (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + String("/setup.html"));
  request->send(response);
}


