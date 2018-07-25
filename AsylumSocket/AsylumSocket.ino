/*
 Name:		AsylumSocket.ino
 Created:	18.07.2018 14:11:41
 Author:	Sorokovikov Vitaliy
*/

//ESP8266
//Flash size: 512К (128K SPIFFS)

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
#define debug						true
#define debug_core					false
#define uid_prefix					"Socket"
#define dns_port					53
#define http_port					80
#define pin_buttonsetup				0	// inverted
#define pin_button					0	// inverted
#define pin_relay					12	// normal
#define pin_ledsetup				13	// inverted

#define buttonsetup_delaytime		10000
#define button_debouncetime			100
#define ledsetup_delay				500
#define mqtt_publishdelay			200

#define wifi_connetion_retrydelay	5000
#define mqtt_connetion_retrydelay	5000

//#define i2cslaveaddress				(0x1)

//DECLARATIONS
String uid;

String mqtt_topic_pub;
String mqtt_topic_sub;
String mqtt_topic_setup;
String mqtt_topic_reboot;

volatile bool	mqtt_value_published = false;
volatile ulong	mqtt_value_publishedtime = 0;

bool buttonsetup_laststate = false;
bool button_laststate = false;
bool ledsetup_laststate = false;

bool setup_mode = false;

ulong buttonsetup_time = 0;
ulong button_time = 0;
ulong ledsetup_time = 0;

ulong	wifi_connetion_time = 0;
ulong	mqtt_connetion_time = 0;

volatile uint16_t state;

DNSServer				dnsServer;
WiFiClient				wifiClient;
PubSubClient			mqttClient(wifiClient);
ESP8266WebServer		httpServer(http_port);
ESP8266HTTPUpdateServer httpUpdater(debug);
IPAddress				setupMode_AP_IP(192, 168, 4, 1);
IPAddress				setupMode_AP_NetMask(255, 255, 255, 0);

//EEPROM stored configuration
struct Config {
	char		reserved[8] = "";
	uint16_t	state;
	uint16_t	value;
	char		description[128];
	uint16_t	mode;
	uint16_t	type;
	char		apssid[32];
	char		apkey[32];
	char		locallogin[32];
	char		localpassword[32];
	char		mqttserver[256];
	char		mqttlogin[32];
	char		mqttpassword[32];
	uint16_t	extension1;
	uint16_t	extension2;
	uint16_t	extension3;
	byte		validator;
};
Config config;

void setup() {
	if (debug) { Serial.begin(74880); }
	Serial.setDebugOutput(debug_core);

	uid = getId();
	mqtt_topic_sub = uid + "/pub";
	mqtt_topic_pub = uid + "/sub";
	mqtt_topic_setup = uid + "/setup";
	mqtt_topic_reboot = uid + "/reboot";
	Serial.println(); Serial.println();
	Serial.print("Chip started ( "); Serial.print(uid); Serial.println(" )");
	Serial.println();

	delay(1000);

#pragma region setup_pins
	Serial.print("Configuring pins ... ");
	pinMode(pin_buttonsetup, INPUT);
	pinMode(pin_button, INPUT);
	pinMode(pin_relay, OUTPUT);		digitalWrite(pin_relay, LOW);		// default initial value
	pinMode(pin_ledsetup, OUTPUT);	digitalWrite(pin_ledsetup, HIGH);	// default initial value
	Serial.println("done");
#pragma endregion

#pragma region setup_i2c
	//Serial.print("Joining I2C bus ... ");
	//Wire.begin(0, 2);        // join i2c bus (address optional for master)
	//Serial.println("done");
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

			if (millis() - wifi_connetion_time > wifi_connetion_retrydelay) {
				wifi_connetion_time = millis();

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

				if (!mqttClient.connected() && millis() - mqtt_connetion_time > mqtt_connetion_retrydelay) {
					mqtt_connetion_time = millis();

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

	if (digitalRead(pin_button) == LOW && button_laststate == false && millis() - button_time > button_debouncetime)
	{
		button_time = millis();
		button_laststate = true;

		if (state == 0) {
			update_state(1);
		}
		else {
			update_state(0);
		}

	}
	if (digitalRead(pin_button) == HIGH && button_laststate == true && millis() - button_time > button_debouncetime)
	{
		button_time = millis();
		button_laststate = false;
	}


	if (digitalRead(pin_buttonsetup) == LOW && buttonsetup_laststate == false)
	{
		buttonsetup_time = millis();
		buttonsetup_laststate = true;
	}
	if (digitalRead(pin_buttonsetup) == HIGH && buttonsetup_laststate == true)
	{
		buttonsetup_time = millis();
		buttonsetup_laststate = false;
	}
	if (buttonsetup_laststate == true && millis() - buttonsetup_time > buttonsetup_delaytime)
	{
		buttonsetup_time = millis();
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

	if (setup_mode || ledsetup_laststate)
	{
		if (millis() - ledsetup_time > ledsetup_delay) {
			ledsetup_time = millis();
			ledsetup_laststate = !ledsetup_laststate;
			digitalWrite(pin_ledsetup, !ledsetup_laststate); // LED circuit inverted
		}
	}
}


