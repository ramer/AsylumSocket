
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void httpserver_setuphandlers() {
  httpServer.serveStatic("/", SPIFFS, "/").setDefaultFile("setup.html").setAuthentication(config["locallogin"].c_str(), config["localpassword"].c_str());
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
  for (auto &itemDefault : configDefault) {
    if (!request->hasParam(itemDefault.first, true)) {
      Serial.printf("API configuration does not have (%s) key, use default value (%s) \n", itemDefault.first.c_str(), itemDefault.second.c_str());
      config[itemDefault.first] = itemDefault.second;
    }
    else {
      config[itemDefault.first] = request->getParam(itemDefault.first, true)->value();
    }
  }

  Config::save(); 

  request->send(200, "text/html", "Configuration saved.");
  has_new_config = true;
}

void handleApiConfig(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["uid"] = device.uid;
  for (auto &item : config) {
    root[item.first] = item.second;
  }

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


