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

#define PIN_SETUP			      0	  // inverted
#define PIN_EVENT					  0	  // inverted
#define PIN_ACTION					12	// normal
#define PIN_LED				      13	// inverted

#define INTERVAL_SETUP		        10000
#define INTERVAL_EVENT_DEBOUNCE	  100
#define INTERVAL_LED				      500
#define INTERVAL_MQTT_PUBLISH		  200

#define INTERVAL_CONNECTION_WIFI	5000
#define INTERVAL_CONNECTION_MQTT	5000

//DECLARATIONS
bool setup_mode = false;

String uid;

String mqtt_topic_pub;
String mqtt_topic_sub;
String mqtt_topic_setup;
String mqtt_topic_reboot;

volatile bool	  mqtt_value_published = false;
volatile ulong	mqtt_value_publishedtime = 0;

bool laststate_setup = false;
bool laststate_event = false;
bool laststate_led   = false;

ulong time_setup = 0;
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
		
	uid = getId();

	mqtt_topic_sub = uid + "/pub";
	mqtt_topic_pub = uid + "/sub";
	mqtt_topic_setup = uid + "/setup";
	mqtt_topic_reboot = uid + "/reboot";

  Serial.printf("\n\n\n");

	Serial.println(); Serial.println();
	Serial.print("Chip started ( "); Serial.print(uid); Serial.println(" )");
	Serial.println();
	Serial.print("Sketch size: "); Serial.println(ESP.getSketchSize());
	Serial.print("Flash sector size: "); Serial.println(FLASH_SECTOR_SIZE);
	Serial.println();

	delay(1000);

#pragma region setup_pins
	Serial.print("Configuring pins ... ");
	pinMode(PIN_SETUP, INPUT);
	pinMode(PIN_EVENT, INPUT);
	pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
	pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
	Serial.println("done");
#pragma endregion

#pragma region setup_i2c
#if DEVICE_TYPE == 3
  Serial.print("Joining I2C bus ... ");
	Wire.begin(0, 2);        // join i2c bus (address optional for master)
	Serial.println("done");
#endif
#pragma endregion

#pragma region setup_eeprom_and_config
	Serial.print("Initializing EEPROM ("); Serial.print(sizeof(Config)); Serial.println(") bytes ... ");
	EEPROM.begin(sizeof(Config));
	Serial.println("done");

	Serial.print("Loading config ...");
	if (loadConfig()) {
		Serial.println(" success");

		Serial.print(" - state:                       "); Serial.println(config.state);
		Serial.print(" - value:                       "); Serial.println(config.value);
		Serial.print(" - description:                 "); Serial.println(config.description);
		Serial.print(" - mode:                        "); Serial.println(config.mode);
		Serial.print(" - type:                        "); Serial.println(config.type);
		Serial.print(" - apssid:                      "); Serial.println(config.apssid);
		Serial.print(" - apkey:                       "); Serial.println(config.apkey);
		Serial.print(" - locallogin:                  "); Serial.println(config.locallogin);
		Serial.print(" - localpassword:               "); Serial.println(config.localpassword);
		Serial.print(" - mqttserver:                  "); Serial.println(config.mqttserver);
		Serial.print(" - mqttlogin:                   "); Serial.println(config.mqttlogin);
		Serial.print(" - mqttpassword:                "); Serial.println(config.mqttpassword);
		Serial.print(" - extension 1 / 2 / 3:         "); Serial.print(config.extension1); Serial.print(" / "); Serial.print(config.extension2); Serial.print(" / "); Serial.println(config.extension3);

		initializeRegularMode();
	}
	else {
		Serial.println(" error");

		dumpConfig();

		initializeSetupMode();
	}
#pragma endregion

}

void loop() {
	
	if (setup_mode) {
		// LOOP IN SETUP MODE

		dnsServer.processNextRequest();
		httpServer.handleClient();
		
	}
	else
	{
		// LOOP IN NORMAL MODE

		if (WiFi.status() != WL_CONNECTED) {

			if (millis() - time_connection_wifi > INTERVAL_CONNECTION_WIFI) {
				time_connection_wifi = millis();

				Serial.print("Connecting to access point: "); Serial.print(config.apssid);
				Serial.print(", password: "); Serial.print(config.apkey); Serial.print(" ... ");

				WiFi.begin(config.apssid, config.apkey);

				switch (WiFi.waitForConnectResult())
				{
				case WL_CONNECTED:
					Serial.print("connected, IP address: ");
					Serial.println(WiFi.localIP());
					break;
				case WL_NO_SSID_AVAIL:
					Serial.print("AP cannot be reached, SSID: ");
					Serial.println(WiFi.SSID());
					break;
				case WL_CONNECT_FAILED:
					Serial.println("incorrect password");
					break;
				case WL_IDLE_STATUS:
					Serial.println("Wi-Fi is in process of changing between statuses");
					break;
				default:
					Serial.println("module is not configured in station mode");
					break;
				}
			}
		}
		else
		{

			if (!mqttClient.loop()) {

				if (!mqttClient.connected() && millis() - time_connection_mqtt > INTERVAL_CONNECTION_MQTT) {
					time_connection_mqtt = millis();

					Serial.print("Connecting to MQTT server: "); Serial.print(config.mqttserver);
					Serial.print(" as "); Serial.print(uid);
					Serial.print(", login: "); Serial.print(config.mqttlogin);
					Serial.print(", password: "); Serial.print(config.mqttpassword); Serial.print(" ... ");

					if (mqttClient.connect(uid.c_str(), config.mqttlogin, config.mqttpassword)) {
						Serial.print("connected, state = ");
						Serial.println(mqttClient.state());
						mqttClient.subscribe(mqtt_topic_sub.c_str());
						mqttClient.subscribe(mqtt_topic_setup.c_str());
						mqttClient.subscribe(mqtt_topic_reboot.c_str());

						mqtt_sendstatus();
					}
					else 
					{
						Serial.print("failed, state = ");
						Serial.print(mqttClient.state());
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


	if (digitalRead(PIN_SETUP) == LOW && laststate_setup == false)
	{
		time_setup = millis();
		laststate_setup = true;
	}
	if (digitalRead(PIN_SETUP) == HIGH && laststate_setup == true)
	{
		time_setup = millis();
		laststate_setup = false;
	}
	if (laststate_setup == true && millis() - time_setup > INTERVAL_SETUP)
	{
		time_setup = millis();
		if (!setup_mode)
		{
			deinitializeRegularMode();
			initializeSetupMode();
		}
		else
		{
			deinitializeSetupMode();
			initializeRegularMode();
		}
	}

	if (setup_mode || laststate_led)
	{
		if (millis() - time_led > INTERVAL_LED) {
			time_led = millis();
			laststate_led = !laststate_led;
			digitalWrite(PIN_LED, !laststate_led); // LED circuit inverted
		}
	}
}


