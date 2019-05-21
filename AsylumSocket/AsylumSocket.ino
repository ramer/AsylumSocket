/*
 Name:		AsylumSocket.ino
 Created:	18.07.2018 14:11:41
 Author:	Sorokovikov Vitaliy
*/

//ESP8266
//Flash size: 1M (64K SPIFFS)

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "src/Config.h"

// GLOBAL FIRMWARE CONFIGURATION

#define DEVICE_TYPE  8

//    0 - Socket
//    1 - reserved
//    2 - Motor
//    3 - Dimmer
//    4 - Strip
//    5 - Encoder
//    6 - Remote2
//    7 - Touch-T1
//    8 - AnalogSensor

// DEFINES

#define DEBUG						              true
#define DEBUG_CORE					          false
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

int8_t mode = -1; // 0 - regular , 1 - smart config , 2 - setup

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

DNSServer				dnsServer;
WiFiClient			wifiClient;
PubSubClient		mqttClient(wifiClient);
AsyncWebServer  httpServer(PORT_HTTP);
IPAddress				wifi_AP_IP  (192, 168, 4, 1);
IPAddress				wifi_AP_MASK(255, 255, 255, 0);
Config          config;

#if DEVICE_TYPE == 0                    // IMPORTANT: use Generic ESP8285 Module
  #include "src/Socket.h"
  Socket device(0, 12);                 // event, action
#elif DEVICE_TYPE == 2
  #include "src/Motor.h"
  Motor device(0, 12, 14);              // event, direction, action
#elif DEVICE_TYPE == 4
  #include "src/Strip.h"
  #define PIN_LED 12                    // redefine - we need GPIO 13 for LEDs
  Strip device(0, 13);                  // event, direction, action
#elif DEVICE_TYPE == 5
  #include "src/Encoder.h"
  Encoder device(0, 14, 12, 13);        // event, action, A, B
#elif DEVICE_TYPE == 7                  // IMPORTANT: use Generic ESP8285 Module
  #include "src/TouchT1.h"
  TouchT1 device(0, 12, 9, 5, 10, 4);   // event, action, event2, action2, event3, action3
#elif DEVICE_TYPE == 8                  // IMPORTANT: use Amperka WiFi Slot
  #include "src/AnalogSensor.h"
  #define PIN_MODE	A5	                // inverted
  #define PIN_LED   A2                  // inverted
  AnalogSensor device(A6, A2);          // sensor, action
#else
  #include "src/Device.h"
  Device device(0, 12);                 // event, action,
#endif
  
void setup() {
#if DEBUG == true
  Serial.begin(74880);
  Serial.setDebugOutput(DEBUG_CORE);
#endif
  
  debug("\n\n\n");
  debug("Chip started. \n");
	debug("Sketch size: %u \n", ESP.getSketchSize());

  debug("Setting WiFi power to %f ... ", WIFI_POWER);
  WiFi.setOutputPower(WIFI_POWER);
  debug("done \n");

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

  debug("Initializing device \n");
  device.initialize(&mqttClient, &config);
  debug("Initialized: %s \n", device.uid.c_str());

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

        config.cur_conf["apssid"] = WiFi.SSID();
        config.cur_conf["apkey"] = WiFi.psk();
          
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

        if (config.cur_conf["apssid"].length() != 0) {
          debug("Connecting to access point: %s , password: %s , attempt: %u \n", config.cur_conf["apssid"].c_str(), config.cur_conf["apkey"].c_str(), attempts_connection_wifi);
          WiFi.begin(config.cur_conf["apssid"].c_str(), config.cur_conf["apkey"].c_str());
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

					debug("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config.cur_conf["mqttserver"].c_str(), device.uid.c_str(), config.cur_conf["mqttlogin"].c_str(), config.cur_conf["mqttpassword"].c_str());

					if (mqttClient.connect(device.uid.c_str(), config.cur_conf["mqttlogin"].c_str(), config.cur_conf["mqttpassword"].c_str())) {
						debug("connected, state = %i \n", mqttClient.state());
						
            device.subscribe();
            mqtt_sendstatus();
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
  device.update();
  blynk();

} // loop

//#if DEVICE_TYPE == 0
//#include "src/Socket.h"
//Socket device(0, 12);
//#elif DEVICE_TYPE == 2
//#define DEVICE_PREFIX				"Motor"
//#define PIN_EVENT					  0
//#define PIN_ACTION					12
//#define PIN_ACTION_DIR  		13
//#define PIN_LED				      14
//#define INTERVAL_MOTOR		  5000
//#elif DEVICE_TYPE == 3
//#define DEVICE_PREFIX				"Dimmer"
//#include <Wire.h>
//#define ADDRESS_I2C_SLAVE		(0x1)
//#elif DEVICE_TYPE == 4
//#define DEVICE_PREFIX				"Strip"
//#include <Adafruit_NeoPixel.h>
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LEDCOUNT, PIN_ACTION, NEO_GRB + NEO_KHZ800); //rgb
//#define PIN_ACTION					13
//#define PIN_LED				      14
//#define INTERVAL_STRIP_FRAME	  10
//#define STRIP_LEDCOUNT          121
//#elif DEVICE_TYPE == 5
//#define DEVICE_PREFIX				"Encoder"
//#define PIN_EVENT					  0
//#define PIN_A					      12
//#define PIN_B					      13
//#define PIN_ACTION					14
//#define PIN_LED				      2
//volatile byte encoderstate = 0;
//#elif DEVICE_TYPE == 6
//#define DEVICE_PREFIX				"Remote2"
//#define PIN_EVENT					  0
//#define PIN_EVENT2					2
//#define TOPIC1				      "Touch1-e079/pub"
//#define TOPIC2				      "Encoder-c9b8/pub"
//#define PIN_LED				      14
//#elif DEVICE_TYPE == 7        // IMPORTANT: use Generic ESP8285 Module
//#include "src/TouchT1.h"
//TouchT1 device(0, 12, 9, 5, 10, 4);
//#else
//#define DEVICE_PREFIX				"Device"
//#endif



//#if DEVICE_TYPE == 0 // Socket
//#elif DEVICE_TYPE == 2 // Motor
//  pinMode(PIN_EVENT, INPUT);
//  pinMode(PIN_ACTION, OUTPUT);	    digitalWrite(PIN_ACTION, LOW);		// default initial value
//  pinMode(PIN_ACTION_DIR, OUTPUT);	digitalWrite(PIN_ACTION_DIR, LOW);	// default initial value
//#elif DEVICE_TYPE == 3 // Dimmer
//  debug("Joining I2C bus ... ");
//  Wire.begin(0, 2);        // join i2c bus (address optional for master)
//  debug("done \n");
//#elif DEVICE_TYPE == 4 // Strip
//  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
//  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
//#elif DEVICE_TYPE == 5 // Encoder
//  pinMode(PIN_EVENT, INPUT);
//  pinMode(PIN_A, INPUT);
//  pinMode(PIN_B, INPUT);
//  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
//	pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, LOW);	// default initial value
//  attachInterrupt(PIN_A, doEncoder, CHANGE);
//#elif DEVICE_TYPE == 6 // Remote2
//  pinMode(PIN_EVENT, INPUT);
//  pinMode(PIN_EVENT2, INPUT);
//  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, LOW);	// default initial value
//#elif DEVICE_TYPE == 7 // Touch1
//#endif