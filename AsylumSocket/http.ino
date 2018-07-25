

void handleRedirect() {
  Serial.print("HTTP-Server redirected from: ");
  Serial.println(httpServer.uri());

  httpServer.sendHeader("Location", String("http://") + toStringIp(httpServer.client().localIP()) + String("/setup.html"), true);
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
  httpServer.arg("apkey").toCharArray(config.apkey, sizeof(config.apkey) - 1);
  httpServer.arg("locallogin").toCharArray(config.locallogin, sizeof(config.locallogin) - 1);
  httpServer.arg("localpassword").toCharArray(config.localpassword, sizeof(config.localpassword) - 1);
  httpServer.arg("mqttserver").toCharArray(config.mqttserver, sizeof(config.mqttserver) - 1);
  httpServer.arg("mqttlogin").toCharArray(config.mqttlogin, sizeof(config.mqttlogin) - 1);
  httpServer.arg("mqttpassword").toCharArray(config.mqttpassword, sizeof(config.mqttpassword) - 1);
  config.extension1 = httpServer.arg("dimmerrangefrom").toInt();
  config.extension2 = httpServer.arg("dimmerrangeto").toInt();
  config.extension3 = httpServer.arg("dimmerstartimpulse").toInt();

  httpServer.send(200, "text/html", "Configutation saved. Rebooting."); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  delay(1000);
  
  Serial.print("Saving config ... ");
  saveConfig();
  Serial.println(" success");
 
  deinitializeSetupMode();
  initializeRegularMode();
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

  Serial.print("Testing dimmer value ("); Serial.print(newvalue); Serial.print(") ... ");
  update_state(newvalue);
  Serial.println("done");
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
  root["locallogin"] = config.locallogin;
  root["mqttserver"] = config.mqttserver;
  root["mqttlogin"] = config.mqttlogin;
  root["dimmerrangefrom"] = config.extension1;
  root["dimmerrangeto"] = config.extension2;
  root["dimmerstartimpulse"] = config.extension3;
  root.printTo(buffer, sizeof(buffer));
        
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", buffer);

  Serial.println("HTTP-Server api_config Responce:");
  Serial.println(buffer);
  Serial.println("HTTP-Server end of api_config");
}

void handleIOS() {
  Serial.print("HTTP-Server IOS handled by 'SUCCESS':");
  Serial.println(httpServer.uri());
  
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
}
