
// ESP8266
// Flash size: 1M (64K SPIFFS)

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

#include "src/Config.h"
#include "src/Device.h"

#include "src/devices/Socket.h"
#include "src/devices/TouchT1.h"
#include "src/devices/Motor.h"
#include "src/devices/Strip.h"
#include "src/devices/Encoder.h"
#include "src/devices/AnalogSensor.h"

// GLOBAL FIRMWARE CONFIGURATION

#define DEVICE_TYPE_SOCKET
#define DEVICE_TYPE_TOUCHT1
//
//#define DEVICE_TYPE_MOTOR
//#define DEVICE_TYPE_STRIP
//#define DEVICE_TYPE_ENCODER
//
//#define DEVICE_TYPE_ANALOGSENSOR

//    10 - Socket
//    11 - TouchT1

//    20 - Motor
//    21 - Strip
//    22 - Encoder

//    30 - AnalogSensor

// DEFINES

#define DEBUG
#define DEBUG_CORE

#define WIFI_POWER                    20.5
#define PORT_DNS					            53
#define PORT_HTTP					            80
#define PIN_MODE			                0	  // inverted
#define PIN_LED                       13  // inverted
#define INTERVAL_SETUP		            10000
#define INTERVAL_MODE_TIMEOUT         600000
#define INTERVAL_CONNECTION_WIFI	    5000
#define INTERVAL_CONNECTION_MQTT	    5000
#define ATTEMPTS_CONNECTION_WIFI      120

// DECLARATIONS

int8_t mode = -1;   // 0 - regular , 1 - smart config , 2 - setup

bool laststate_mode = false;
bool laststate_led = false;

bool is_updating = false;
bool has_new_update = false;
bool has_new_config = false;

ulong time_mode = 0;
ulong time_mode_set = 0;
ulong time_led = 0;

ulong	time_connection_wifi = 0;
ulong	time_connection_mqtt = 0;
byte attempts_connection_wifi = 0;

String uid;

DNSServer				dnsServer;
WiFiClient			wifiClient;
PubSubClient		mqttClient(wifiClient);
AsyncWebServer  httpServer(PORT_HTTP);
IPAddress				wifi_AP_IP  (192, 168, 4, 1);
IPAddress				wifi_AP_MASK(255, 255, 255, 0);
Config          config;

std::vector<Device*> devices;

  
void setup() {
#ifdef DEBUG
  Serial.begin(74880);
#ifdef DEBUG_CORE
  Serial.setDebugOutput(true);
#endif
#endif
  
  debug("\n\n\n");
  debug("Chip started. \n");
	debug("Sketch size: %u \n", ESP.getSketchSize());

  debug("Setting WiFi power to %f ... ", WIFI_POWER);
  WiFi.setOutputPower(WIFI_POWER);
  debug("done \n");

  debug("Generating primary UID ... ");
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);
  uid = "ESP-";
  for (int i = sizeof(MAC_array) - 2; i < sizeof(MAC_array); ++i) uid += String(MAC_array[i], HEX);
  debug("%s \n", uid.c_str());

	debug("Configuring pins ... ");
	pinMode(PIN_MODE, INPUT);
  pinMode(PIN_LED, OUTPUT);	  digitalWrite(PIN_LED, HIGH);	    // default initial value
  debug("done \n");
  
  debug("Mounting SPIFFS ... ");
  if (SPIFFS.begin()) {
    debug("success \n");
  }
  else {
    debug("error \n");
  }

  bool configured = config.loadConfig();

#if (defined DEVICE_TYPE_SOCKET                         && defined ARDUINO_ESP8266_ESP01)
  devices.push_back(new Socket("Socket", 0, 12));       // event, action
#endif
#if (defined DEVICE_TYPE_TOUCHT1                        && defined ARDUINO_ESP8266_ESP01)
  devices.push_back(new TouchT1("TouchT1", 0, 12, 9, 5, 10, 4));   // event, action, event2, action2, event3, action3
#endif

// IMPORTANT: use Generic ESP8266 Module
#if (defined DEVICE_TYPE_MOTOR                          && defined ARDUINO_ESP8266_GENERIC)    
  devices.push_back(new Motor("Motor", 0, 12, 14));              // event, direction, action
#endif
#if (defined DEVICE_TYPE_STRIP                          && defined ARDUINO_ESP8266_GENERIC)
  #define PIN_LED 12                                    // redefine - we need GPIO 13 for LEDs
  devices.push_back(new Strip("Strip", 0, 13));                  // event, direction, action
#endif
#if (defined DEVICE_TYPE_ENCODER                        && defined ARDUINO_ESP8266_GENERIC)
  devices.push_back(new Encoder("Encoder", 0, 14, 12, 13));        // event, action, A, B
#endif

// IMPORTANT: use Amperka WiFi Slot
#if (defined DEVICE_TYPE_ANALOGSENSOR                   && defined ARDUINO_AMPERKA_WIFI_SLOT)
  #define PIN_MODE	A5	                                // inverted
  #define PIN_LED   A2                                  // inverted
  devices.push_back(new AnalogSensor("AnalogSensor", A5, A2, A6));      // event, action, sensor
#endif 
  
  debug("Initializing devices \n");

  for (auto &d : devices) {
    d->initialize(&mqttClient, &config);
    debug("Initialized: %s \n", d->uid.c_str());
  }

  debug("Starting HTTP-server ... ");
  httpserver_setuphandlers();
  httpServer.begin();
  debug("started \n");

  if (configured) {
    set_mode(0);
  }
  else {
    set_mode(1);
  }
}

void loop() {

  // DO NOT DO ANYTHING WHILE UPDATING
  if (is_updating) return;

  if (mode == 1) {
    // LOOP IN SMART CONFIG MODE

    if (WiFi.smartConfigDone()) {

      debug("Connecting to access point: %s , password: %s ... ", WiFi.SSID().c_str(), WiFi.psk().c_str());

      switch (WiFi.waitForConnectResult())
      {
      case WL_CONNECTED:
        debug("connected, IP address: %s \n", WiFi.localIP().toString().c_str());

        config.current["apssid"] = WiFi.SSID();
        config.current["apkey"] = WiFi.psk();
          
        initializeSetupMode(); // cant use 'set_mode()' - we can't deinitialize smart config mode yet
        mode = 2; // crutch
        break;
      case WL_NO_SSID_AVAIL:
        debug("AP cannot be reached, SSID: %s \n", WiFi.SSID().c_str());
        break;
      case WL_CONNECT_FAILED:
        debug("incorrect password \n");
        break;
      case WL_IDLE_STATUS:
        debug("Wi-Fi is idle \n");
        break;
      default:
        debug("module is not configured in station mode \n");
        break;
      }

      // we have nothing to do if we can't connect to AP
    }

	} else if (mode == 2) {
    // LOOP IN SETUP MODE

    dnsServer.processNextRequest();

  } else {
		// LOOP IN REGULAR MODE

		if (WiFi.status() != WL_CONNECTED) {

			if (millis() - time_connection_wifi > INTERVAL_CONNECTION_WIFI) {
				time_connection_wifi = millis();
        attempts_connection_wifi++;
        if (attempts_connection_wifi >= 100) { set_mode(1); return; }

        if (config.current["apssid"].length() != 0) {
          debug("Connecting to access point: %s , password: %s , attempt: %u \n", config.current["apssid"].c_str(), config.current["apkey"].c_str(), attempts_connection_wifi);
          WiFi.begin(config.current["apssid"].c_str(), config.current["apkey"].c_str());
        } else {
          debug("Connecting to access point error: no SSID specified \n");
          set_mode(1); return;
        }
			}
		}
		else
		{
      attempts_connection_wifi = 0;

			if (!mqttClient.loop()) {

				if (!mqttClient.connected() && millis() - time_connection_mqtt > INTERVAL_CONNECTION_MQTT) {
					time_connection_mqtt = millis();

					debug("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config.current["mqttserver"].c_str(), uid.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str());

          if (mqttClient.connect(uid.c_str(), config.current["mqttlogin"].c_str(), config.current["mqttpassword"].c_str())) {
            debug("connected, state = %i \n", mqttClient.state());

            for (auto &d : devices) {
              d->subscribe();
              d->publishStatus();
            }

					}
					else 
					{
						debug ("failed, state = %i \n", mqttClient.state());
					}
				}
			}
		}
	}

	// LOOP ANYWAY

  check_newupdate();
  check_newconfig();
  check_mode();

  for (auto &d : devices) d->update();

  blynk();

} // loop