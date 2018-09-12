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

#define DEVICE_TYPE  7

//    0 - Socket
//    1 - reserved
//    2 - Motor
//    3 - Dimmer
//    4 - Strip
//    5 - Encoder
//    6 - Remote2
//    7 - Touch-T1

// DEFINES

#define DEBUG						              true
#define DEBUG_CORE					          false
#define PORT_DNS					            53
#define PORT_HTTP					            80
#define PIN_MODE			                0	  // inverted
#define PIN_LED                       13  // inverted
#define INTERVAL_SETUP		            10000
#define INTERVAL_MODE_TIMEOUT         600000
#define INTERVAL_CONNECTION_WIFI	    5000
#define INTERVAL_CONNECTION_MQTT	    5000

#if DEVICE_TYPE == 0
  #include "src/Socket.h"
  Socket device(0, 12);
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
  #include "src/TouchT1.h"
  TouchT1 device(0, 12, 9, 5, 10, 4);
#else
  #define DEVICE_PREFIX				"Device"
#endif


//DECLARATIONS

int8_t mode = -1; // 0 - regular , 1 - setup , 2 - smart config

bool laststate_mode = false;
bool laststate_led = false;

bool has_new_update = false;
bool has_new_config = false;

ulong time_mode = 0;
ulong time_mode_set = 0;
ulong time_led = 0;

ulong	time_connection_wifi = 0;
ulong	time_connection_mqtt = 0;

DNSServer				dnsServer;
WiFiClient			wifiClient;
PubSubClient		mqttClient(wifiClient);
AsyncWebServer  httpServer(PORT_HTTP);
IPAddress				wifi_AP_IP  (192, 168, 4, 1);
IPAddress				wifi_AP_MASK(255, 255, 255, 0);

//EEPROM stored configuration



void setup() {
#if DEBUG == true
  Serial.begin(74880);
  Serial.setDebugOutput(DEBUG_CORE);
#endif
  
  Serial.printf("\n\n\n");
  Serial.printf("Chip started. \n");
	Serial.printf("Sketch size: %u \n", ESP.getSketchSize());

	Serial.printf("Configuring pins ... ");
	pinMode(PIN_MODE, INPUT);
  pinMode(PIN_LED, OUTPUT);	  digitalWrite(PIN_LED, HIGH);	    // default initial value
  Serial.printf("done \n");
  
  Serial.printf("Mounting SPIFFS ... ");
  if (SPIFFS.begin()) {
    Serial.printf("success \n");
  }
  else {
    Serial.printf("error \n");
  }

  Serial.printf("Initializing device ... ");
  device.initialize(&mqttClient);
  Serial.printf("%s \n", device.uid.c_str());

  if (Config::load()) {
    //Serial.printf(" - description:         %s \n", config["description"].c_str());
    //Serial.printf(" - mode:                %u \n", config["mode"].toInt());
    //Serial.printf(" - apssid:              %s \n", config["apssid"].c_str());
    //Serial.printf(" - apkey:               %s \n", config["apkey"].c_str());
    //Serial.printf(" - locallogin:          %s \n", config["locallogin"].c_str());
    //Serial.printf(" - localpassword:       %s \n", config["localpassword"].c_str());
    //Serial.printf(" - mqttserver:          %s \n", config["mqttserver"].c_str());
    //Serial.printf(" - mqttlogin:           %s \n", config["mqttlogin"].c_str());
    //Serial.printf(" - mqttpassword:        %s \n", config["mqttpassword"].c_str());
    //Serial.printf(" - onboot:              %u \n", config["onboot"].toInt());
    //Serial.printf(" - onboardled:          %u \n", config["onboardled"].toInt());
    //Serial.printf(" - extension 1 / 2 / 3: %s / %s / %s \n", config["extension1"].c_str(), config["extension2"].c_str(), config["extension3"].c_str());

    //device.setOnBootState();

    set_mode(0);
  }
  else {
    set_mode(1);
  }

  Serial.printf("Starting HTTP-server ... ");
  httpserver_setuphandlers();
  httpServer.begin();
  Serial.printf("started \n");
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

        config["apssid"] = WiFi.SSID();
        config["apkey"] = WiFi.psk();
          
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

        if (config["apssid"].length() != 0) {
          Serial.printf("Connecting to access point: %s , password: %s \n", config["apssid"].c_str(), config["apkey"].c_str());
          WiFi.begin(config["apssid"].c_str(), config["apkey"].c_str());
        } else {
          Serial.printf("Connecting to access point error: no SSID specified \n");
        }
			}
		}
		else
		{
			if (!mqttClient.loop()) {

				if (!mqttClient.connected() && millis() - time_connection_mqtt > INTERVAL_CONNECTION_MQTT) {
					time_connection_mqtt = millis();

					Serial.printf("Connecting to MQTT server: %s as %s , auth %s : %s ... ", config["mqttserver"].c_str(), device.uid.c_str(), config["mqttlogin"].c_str(), config["mqttpassword"].c_str());

					if (mqttClient.connect(device.uid.c_str(), config["mqttlogin"].c_str(), config["mqttpassword"].c_str())) {
						Serial.printf("connected, state = %i \n", mqttClient.state());
						
            device.subscribe();
            mqtt_sendstatus();
					}
					else 
					{
						Serial.printf ("failed, state = %i \n", mqttClient.state());
					}
				}
			}
			else {

        device.checkPublished();

			}
		}
	}

	// LOOP ANYWAY

  device.checkButtons();
  check_newupdate();
  check_newconfig();
  check_mode();
  blynk();

#if DEVICE_TYPE == 0 // Socket

#elif DEVICE_TYPE == 2 // Motor
  if (digitalRead(PIN_EVENT) == LOW && pin_event_laststate == false && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = true;

    invertState();
  }
  if (digitalRead(PIN_EVENT) == HIGH && pin_event_laststate == true && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = false;
  }
#elif DEVICE_TYPE == 3 // Dimmer
#elif DEVICE_TYPE == 4 // Strip
  update_strip();
#elif DEVICE_TYPE == 5 // Encoder
  if (digitalRead(PIN_EVENT) == LOW && pin_event_laststate == false && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = true;

    invertState();
  }
  if (digitalRead(PIN_EVENT) == HIGH && pin_event_laststate == true && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = false;
  }

  if (state != encoderstate) { updateState(encoderstate); }
#elif DEVICE_TYPE == 6 // Remote2
  if (digitalRead(PIN_EVENT) == LOW && pin_event_laststate == false && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = true;

    mqtt_sendcommand(TOPIC1);
  }
  if (digitalRead(PIN_EVENT) == HIGH && pin_event_laststate == true && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = false;
  }
  if (digitalRead(PIN_EVENT2) == LOW && pin_event_laststate == false && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = true;

    mqtt_sendcommand(TOPIC2);
  }
  if (digitalRead(PIN_EVENT2) == HIGH && pin_event_laststate == true && millis() - pin_event_time > INTERVAL_EVENT_DEBOUNCE)
  {
    pin_event_time = millis();
    pin_event_laststate = false;
  }
#elif DEVICE_TYPE == 7 // Touch1

#endif

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



//#if DEVICE_TYPE == 0 // Socket
//#elif DEVICE_TYPE == 2 // Motor
//  pinMode(PIN_EVENT, INPUT);
//  pinMode(PIN_ACTION, OUTPUT);	    digitalWrite(PIN_ACTION, LOW);		// default initial value
//  pinMode(PIN_ACTION_DIR, OUTPUT);	digitalWrite(PIN_ACTION_DIR, LOW);	// default initial value
//#elif DEVICE_TYPE == 3 // Dimmer
//  Serial.printf("Joining I2C bus ... ");
//  Wire.begin(0, 2);        // join i2c bus (address optional for master)
//  Serial.printf("done \n");
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