/*
 Name:		AsylumSocket.ino
 Created:	18.07.2018 14:11:41
 Author:	Sorokovikov Vitaliy
*/

//ESP8266
//Flash size: 1M (64K SPIFFS)

#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include <FS.h>
#include <WiFiUdp.h>

// DEFINES

#define DEBUG						    true
#define DEBUG_CORE					false

#define PORT_DNS					  53
#define PORT_HTTP					  80

#define PIN_MODE			      0	  // inverted

#define INTERVAL_SETUP		        10000
#define INTERVAL_EVENT_DEBOUNCE	  100
#define INTERVAL_LED_SETUP	      500
#define INTERVAL_LED_SMARTCONFIG  250
#define INTERVAL_MQTT_PUBLISH		  200

#define INTERVAL_CONNECTION_WIFI	5000
#define INTERVAL_CONNECTION_MQTT	5000
//#define MQTT_MAX_PACKET_SIZE      524288

#define DEVICE_TYPE           6

#if DEVICE_TYPE == 0
  #define DEVICE_PREFIX				"Socket"
  #define PIN_EVENT					  0	  // inverted
  #define PIN_ACTION					12	// normal
  #define PIN_LED				      13	// inverted
#elif DEVICE_TYPE == 2
  #define DEVICE_PREFIX				"Motor"
  #define PIN_EVENT					  0
  #define PIN_ACTION					12
  #define PIN_ACTION_DIR  		13
  #define PIN_LED				      14
  #define INTERVAL_MOTOR		  5000
#elif DEVICE_TYPE == 3
  #define DEVICE_PREFIX				"Dimmer"
  #include <Wire.h>
  #define ADDRESS_I2C_SLAVE		(0x1)
#elif DEVICE_TYPE == 4
  #define DEVICE_PREFIX				"Strip"
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LEDCOUNT, PIN_ACTION, NEO_GRB + NEO_KHZ800); //rgb
  #define PIN_ACTION					13
  #define PIN_LED				      14
  #define INTERVAL_STRIP_FRAME	  10
  #define STRIP_LEDCOUNT          121
#elif DEVICE_TYPE == 5
  #define DEVICE_PREFIX				"Encoder"
  #define PIN_EVENT					  0
  #define PIN_A					      12
  #define PIN_B					      13
  #define PIN_ACTION					14
  #define PIN_LED				      2
  volatile byte encoderstate = 0;
#elif DEVICE_TYPE == 6
  #define DEVICE_PREFIX				"Remote2"
  #define PIN_EVENT					  0
  #define PIN_EVENT2					2
  #define TOPIC1				      "Touch1-e079/pub"
  #define TOPIC2				      "Encoder-c9b8/pub"
  #define PIN_LED				      14
#elif DEVICE_TYPE == 7        // IMPORTANT: use Generic ESP8285 Module
  #define DEVICE_PREFIX				"Touch1"
  #define PIN_EVENT					  0	  // inverted
  #define PIN_ACTION					12	// normal
  #define PIN_LED				      13	// inverted
#else
  #define DEVICE_PREFIX				"Device"
#endif


//DECLARATIONS
//bool setup_mode = false;
int8_t mode = -1; // 0 - regular , 1 - setup , 2 - smart config

char * uid;

char * mqtt_topic_pub;
char * mqtt_topic_sub;
char * mqtt_topic_status;
char * mqtt_topic_setup;
char * mqtt_topic_reboot;

volatile bool	  mqtt_state_published = false;
volatile ulong	mqtt_state_publishedtime = 0;

bool laststate_mode = false;
bool laststate_event = false;
bool laststate_led   = false;

bool has_new_update = false;
bool has_new_config = false;

ulong time_mode = 0;
ulong time_event = 0;
ulong time_led = 0;

ulong	time_connection_wifi = 0;
ulong	time_connection_mqtt = 0;

volatile ulong state;  // main vars, holds device state or/and value
volatile ulong value;
volatile ulong state_previous;

DNSServer				        dnsServer;
WiFiClient				      wifiClient;
PubSubClient			      mqttClient(wifiClient);
AsyncWebServer          httpServer(PORT_HTTP);
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
  Serial.printf("Chip started. \n", uid);
  resolve_identifiers();
  Serial.printf("UID: (%s) \n", uid);
	Serial.printf("Sketch size: %u \n", ESP.getSketchSize());

	delay(1000);

	Serial.printf("Configuring pins ... ");
	pinMode(PIN_MODE, INPUT);
#if DEVICE_TYPE == 0 // Socket
  pinMode(PIN_EVENT, INPUT);
  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
#elif DEVICE_TYPE == 2 // Motor
  pinMode(PIN_EVENT, INPUT);
  pinMode(PIN_ACTION, OUTPUT);	    digitalWrite(PIN_ACTION, LOW);		// default initial value
  pinMode(PIN_ACTION_DIR, OUTPUT);	digitalWrite(PIN_ACTION_DIR, LOW);	// default initial value
#elif DEVICE_TYPE == 3 // Dimmer
  Serial.printf("Joining I2C bus ... ");
  Wire.begin(0, 2);        // join i2c bus (address optional for master)
  Serial.printf("done \n");
#elif DEVICE_TYPE == 4 // Strip
  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
#elif DEVICE_TYPE == 5 // Encoder
  pinMode(PIN_EVENT, INPUT);
  pinMode(PIN_A, INPUT);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
	pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, LOW);	// default initial value
  attachInterrupt(PIN_A, doEncoder, CHANGE);
#elif DEVICE_TYPE == 6 // Remote2
  pinMode(PIN_EVENT, INPUT);
  pinMode(PIN_EVENT2, INPUT);
  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, LOW);	// default initial value
#elif DEVICE_TYPE == 7 // Touch1
  pinMode(PIN_EVENT, INPUT);
  pinMode(PIN_ACTION, OUTPUT);	digitalWrite(PIN_ACTION, LOW);		// default initial value
  pinMode(PIN_LED, OUTPUT);	    digitalWrite(PIN_LED, HIGH);	// default initial value
#endif
	Serial.printf("done \n");

  Serial.printf("Initializing EEPROM (%u bytes) ... ", sizeof(Config));
	EEPROM.begin(sizeof(Config));
	Serial.printf("done \n");

  Serial.printf("Mounting SPIFFS ... ");
  if (SPIFFS.begin()) {
    Serial.printf("success \n");
  }
  else {
    Serial.printf("error \n");
  }
    
  Serial.printf("Starting HTTP-server ... ");
  httpserver_setuphandlers();
  httpServer.begin();
  Serial.printf("started \n");

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

    set_mode(0);
	}
	else {
		Serial.printf("error \n");

		dumpConfig();

    set_mode(1);
  }

}

void loop() {
	
  if (mode == 1) {
    // LOOP IN SMART CONFIG MODE

    if (WiFi.smartConfigDone()) {

      Serial.printf("Connecting to access point: %s , password: %s ... ", WiFi.SSID().c_str(), WiFi.psk().c_str());

      switch (WiFi.waitForConnectResult())
      {
      case WL_CONNECTED:
        Serial.printf("connected, IP address: %s \n", WiFi.localIP().toString().c_str());

        memset(config.apssid, 0, sizeof(config.apssid));
        memset(config.apkey, 0, sizeof(config.apkey));

        WiFi.SSID().toCharArray(config.apssid, sizeof(config.apssid) - 1);
        WiFi.psk().toCharArray(config.apkey, sizeof(config.apkey) - 1);
          
        initializeSetupMode(); // cant use 'set_mode()' - we can't deinitialize smart config mode yet
        mode = 2; // crutch
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

				Serial.printf("Connecting to access point: %s , password: %s \n", config.apssid, config.apkey);

				WiFi.begin(config.apssid, config.apkey);
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

						mqtt_sendstate();
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
#if DEVICE_TYPE == 0 // Socket
  if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
#elif DEVICE_TYPE == 2 // Motor
  if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
#elif DEVICE_TYPE == 3 // Dimmer
#elif DEVICE_TYPE == 4 // Strip
  update_strip();
#elif DEVICE_TYPE == 5 // Encoder
  if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }

  if (state != encoderstate) { update_state(encoderstate); }
#elif DEVICE_TYPE == 6 // Remote2
  if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    mqtt_sendcommand(TOPIC1);
  }
  if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
  if (digitalRead(PIN_EVENT2) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    mqtt_sendcommand(TOPIC2);
  }
  if (digitalRead(PIN_EVENT2) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
#elif DEVICE_TYPE == 7 // Touch1
  if (digitalRead(PIN_EVENT) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(PIN_EVENT) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
#endif
  
  check_newupdate();
  check_newconfig();
  check_mode();
  blynk();

} // loop

#if DEVICE_TYPE == 5 // Encoder
//void doEncoderA()
//{
//  if (PastB) {
//    if (encoderstate > 0) { encoderstate--; }
//  }
//  else {
//    if (encoderstate < 255) { encoderstate++; }
//  }
//}
//
//void doEncoderB()
//{
//  PastB = !PastB;
//}
void doEncoder() {
  if (digitalRead(PIN_A) == digitalRead(PIN_B)) { // CCW
    if (encoderstate > 0) { encoderstate--; }
  }
  else { // CW
    if (encoderstate < 255) { encoderstate++; }
  }
}
#endif
