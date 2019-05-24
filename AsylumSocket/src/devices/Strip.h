// Strip.h

#ifdef ARDUINO_ESP8266_GENERIC

#ifndef _STRIP_h
#define _STRIP_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "../Device.h"

#define INTERVAL_STRIP_FRAME	  10
#define STRIP_LEDCOUNT          121
#define STARS_PROBABILITY 100
#define STARS_INCREMENT 1

class Strip : public Device
{
public:
  Strip(byte event, byte action);
  void initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix = "Strip");

  void update();
  void updateState(ulong state_new);

  uint32_t strip_wheel(byte angle);

protected:
  void update_strip();
  void frame_solid();
  void frame_rainbow();
  void frame_stars();
  void frame_sunrise();

  ulong time_strip_frame = 0;

  ulong solid_laststate;
  uint8_t rainbow_offset;
  uint8_t stars[STRIP_LEDCOUNT];
  uint16_t sunrise;

  Adafruit_NeoPixel strip;
};

#endif

#endif