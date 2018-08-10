/*
 Name:		AsylumSocket.ino
 Created:	18.07.2018 14:11:41
 Author:	Sorokovikov Vitaliy
*/

//ESP8266
//Flash size: 1M (64K SPIFFS)

#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include <FS.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <stdio.h>

// DEFINES
#define DEVICE_TYPE         0

#define DEBUG						    true
#define DEBUG_CORE					false

#if DEVICE_TYPE == 0
#define DEVICE_PREFIX				"Socket"
#elif DEVICE_TYPE == 1
#define DEVICE_PREFIX				"Switch"
#elif DEVICE_TYPE == 2
#define DEVICE_PREFIX				"Motor"
#elif DEVICE_TYPE == 3
#define DEVICE_PREFIX				"Dimmer"
#define ADDRESS_I2C_SLAVE		(0x1)
#else
#define DEVICE_PREFIX				"Device"
#endif

#define PORT_DNS					  53
#define PORT_HTTP					  80

#define PIN_MODE			      0	  // inverted
#define PIN_EVENT					  0	  // inverted
#define PIN_ACTION					12	// normal
#define PIN_LED				      13	// inverted

#define INTERVAL_SETUP		        10000
#define INTERVAL_EVENT_DEBOUNCE	  100
#define INTERVAL_LED_SETUP	      500
#define INTERVAL_LED_SMARTCONFIG  250
#define INTERVAL_MQTT_PUBLISH		  200

#define INTERVAL_CONNECTION_WIFI	5000
#define INTERVAL_CONNECTION_MQTT	5000

//DECLARATIONS
//bool setup_mode = false;
byte mode; // 0 - regular , 1 - setup , 2 - smart config

char * uid;

char * mqtt_topic_pub;
char * mqtt_topic_sub;
char * mqtt_topic_setup;
char * mqtt_topic_reboot;

volatile bool	  mqtt_value_published = false;
volatile ulong	mqtt_value_publishedtime = 0;

bool laststate_mode = false;
bool laststate_event = false;
bool laststate_led   = false;

ulong time_mode = 0;
ulong time_event = 0;
ulong time_led = 0;

ulong	time_connection_wifi = 0;
ulong	time_connection_mqtt = 0;

volatile uint16_t state;  // main var, holds 
volatile uint16_t value;

DNSServer				        dnsServer;
WiFiClient				      wifiClient;
PubSubClient			      mqttClient(wifiClient);
ESP8266WebServer		    httpServer(PORT_HTTP);
ESP8266HTTPUpdateServer httpUpdater(DEBUG);
IPAddress				        wifi_AP_IP  (192, 168, 4, 1);
IPAddress				        wifi_AP_MASK(255, 255, 255, 0);

//EEPROM stored configuration
struct Config {
	char		  reserved[8] = "";
  uint16_t	type = DEVICE_TYPE;
  uint16_t	state;
	uint16_t	value;
	char		  description[128];
	uint16_t	mode; //reserved
	char		  apssid[32];
	char		  apkey[32];
	char		  locallogin[32];
	char		  localpassword[32];
	char		  mqttserver[256];
	char		  mqttlogin[32];
	char		  mqttpassword[32];
	uint16_t	extension1;
	uint16_t	extension2;
	uint16_t	extension3;
	byte		  validator;
};
Config config;

void setup() {
	if (DEBUG) { Serial.begin(74880); }
	Serial.setDebugOutput(DEBUG_CORE);
  
  delay(1000);

  Serial.printf("\n\n\n");

  ResolveIdentifiers();
  
	Serial.printf("Chip started (%s) \n", uid);
	Serial.printf("Sketch size: %u \n", ESP.getSketchSize());

	delay(1000);

#pragma region setup_pins
	Serial.printf("Configuring pins ... ");
	pinMode(PIN_MODE, INPUT);
	pinMode(PIN_EVENT, INPUT);
	pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
	pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
	Serial.printf("done \n");
#pragma endregion

#pragma region setup_i2c
#if DEVICE_TYPE == 3
  Serial.printf("Joining I2C bus ... ");
	Wire.begin(0, 2);        // join i2c bus (address optional for master)
	Serial.printf("done \n");
#endif
#pragma endregion

#pragma region setup_eeprom_and_config
  Serial.printf("Initializing EEPROM (%u bytes) ... ", sizeof(Config));
	EEPROM.begin(sizeof(Config));
	Serial.printf("done \n");

	Serial.printf("Loading config ... ");
	if (loadConfig()) {
		Serial.printf("success \n");

		Serial.printf(" - state:               %u \n", config.state);
		Serial.printf(" - value:               %u \n", config.value);
		Serial.printf(" - description:         %s \n", config.description);
		Serial.printf(" - mode:                %u \n", config.mode);
		Serial.printf(" - type:                %u \n", config.type);
		Serial.printf(" - apssid:              %s \n", config.apssid);
		Serial.printf(" - apkey:               %s \n", config.apkey);
		Serial.printf(" - locallogin:          %s \n", config.locallogin);
		Serial.printf(" - localpassword:       %s \n", config.localpassword);
		Serial.printf(" - mqttserver:          %s \n", config.mqttserver);
		Serial.printf(" - mqttlogin:           %s \n", config.mqttlogin);
		Serial.printf(" - mqttpassword:        %s \n", config.mqttpassword);
		Serial.printf(" - extension 1 / 2 / 3: %u / %u / %u \n", config.extension1, config.extension2, config.extension3);

    mode = 0;
    initializeRegularMode();
	}
	else {
		Serial.printf("error \n");

		dumpConfig();

    mode = 1;
		initializeSetupMode();
	}
#pragma endregion

}

void loop() {
	
  if (mode == 1) {
    // LOOP IN SETUP MODE

    dnsServer.processNextRequest();
    httpServer.handleClient();

  } 
  else if (mode == 2) {
    // LOOP IN SMART CONFIG MODE

    if (WiFi.smartConfigDone()) {

      Serial.printf("Connecting to access point: %s , password: %s ... ", WiFi.SSID().c_str(), WiFi.psk().c_str());

      switch (WiFi.waitForConnectResult())
      {
      case WL_CONNECTED:
        Serial.printf("connected, IP address: %s \n", WiFi.localIP().toString().c_str());
        break;
      case WL_NO_SSID_AVAIL:
        Serial.printf("AP cannot be reached, SSID: %s \n", WiFi.SSID().c_str());
        break;
      case WL_CONNECT_FAILED:
        Serial.printf("incorrect password \n");
        break;
      case WL_IDLE_STATUS:
        Serial.printf("Wi-Fi is idle \n");
        break;
      default:
        Serial.printf("module is not configured in station mode \n");
        break;
      }

      initializeSetupMode();
      mode = 1;
    }

	}
	else
	{
		// LOOP IN REGULAR MODE

		if (WiFi.status() != WL_CONNECTED) {

			if (millis() - time_connection_wifi > INTERVAL_CONNECTION_WIFI) {
				time_connection_wifi = millis();

				Serial.printf("Connecting to access point: %s , password: %s ... ", config.apssid, config.apkey);

				WiFi.begin(config.apssid, config.apkey);

				switch (WiFi.waitForConnectResult())
				{
				case WL_CONNECTED:
					Serial.printf("connected, IP address: %s \n", WiFi.localIP().toString().c_str());
					break;
				case WL_NO_SSID_AVAIL:
					Serial.printf("AP cannot be reached, SSID: %s \n", WiFi.SSID().c_str());
					break;
				case WL_CONNECT_FAILED:
					Serial.printf("incorrect password \n");
					break;
				case WL_IDLE_STATUS:
					Serial.printf("Wi-Fi is idle \n");
					break;
				default:
					Serial.printf("module is not configured in station mode \n");
					break;
				}
			}
		}
		else
		{

			if (!mqttClient.loop()) {

				if (!mqttClient.connected() && millis() - time_connection_mqtt > INTERVAL_CONNECTION_MQTT) {
					time_connection_mqtt = millis();

					Serial.printf("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config.mqttserver, uid, config.mqttlogin, config.mqttpassword);

					if (mqttClient.connect(uid, config.mqttlogin, config.mqttpassword)) {
						Serial.printf("connected, state = %i \n", mqttClient.state());
						
            mqttClient.subscribe(mqtt_topic_sub);
						mqttClient.subscribe(mqtt_topic_setup);
						mqttClient.subscribe(mqtt_topic_reboot);

						mqtt_sendstatus();
					}
					else 
					{
						Serial.printf ("failed, state = %i \n", mqttClient.state());
					}
				}
			}
			else {
				mqtt_check_value_published();
			}
		}
	}

	// LOOP ANYWAY

	if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
	{
		time_event = millis();
		laststate_event = true;

		if (state == 0) {
			update_state(1);
		}
		else {
			update_state(0);
		}

	}
	if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
	{
		time_event = millis();
		laststate_event = false;
	}


	if (digitalRead(PIN_MODE) == LOW && laststate_mode == false)
	{
		time_mode = millis();
		laststate_mode = true;
	}
	if (digitalRead(PIN_MODE) == HIGH && laststate_mode == true)
	{
		time_mode = millis();
		laststate_mode = false;
	}
	if (laststate_mode == true && millis() - time_mode > INTERVAL_SETUP)
	{
    Serial.printf("Mode button pressed for %u ms \n", INTERVAL_SETUP);

		time_mode = millis();
		if (mode == 1)
		{
			deinitializeSetupMode();
      initializeSmartConfigMode();
      mode = 2;
		}
    else if (mode == 2) {
      deinitializeSmartConfigMode();
      initializeRegularMode();
      mode = 0;
    }
    else
    {
			deinitializeRegularMode();
      initializeSetupMode();
      mode = 1;
		}
	}

	if (mode > 0 || laststate_led)
	{
    uint16_t interval;
    if (mode == 1) { interval = INTERVAL_LED_SETUP; }
    else { interval = INTERVAL_LED_SMARTCONFIG; }

    if (millis() - time_led > interval) {
			time_led = millis();
			laststate_led = !laststate_led;
			digitalWrite(PIN_LED, !laststate_led); // LED circuit inverted
		}
	}
}


