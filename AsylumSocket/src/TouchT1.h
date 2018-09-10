// TouchT1.h

#ifndef _TOUCHT1_h
#define _TOUCHT1_h

#include "ESP8266WiFi.h"

#define INTERVAL_EVENT_DEBOUNCE	      100
#define INTERVAL_LED_SETUP	          500
#define INTERVAL_LED_SMARTCONFIG      250

class TouchT1
{
  public:
    TouchT1(byte pin_event, byte pin_action, byte pin_led);
    void initialize();
    void check_buttons();
    void update_state(ulong state_new);
    void invert_state();
    void blynk(uint8_t mode, byte onboardled);
    ulong state;

  private:
    byte _pin_event;
    byte _pin_action;
    byte _pin_led;
    ulong _state;
    bool laststate_event = false;
    bool laststate_led = false;
    ulong time_event = 0;
    ulong time_led = 0;
};

#endif

