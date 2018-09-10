// TouchT1.cpp

#include "TouchT1.h"

TouchT1::TouchT1(byte pin_event, byte pin_action, byte pin_led) {
  _pin_event = pin_event;
  _pin_action = pin_action;
  _pin_led = pin_led;
}

void TouchT1::initialize() {
  pinMode(_pin_event, INPUT);
  pinMode(_pin_action, OUTPUT);	digitalWrite(_pin_action, LOW);		// default initial value
  pinMode(_pin_led, OUTPUT);	  digitalWrite(_pin_led, HIGH);	    // default initial value
}

void TouchT1::check_buttons() {
  if (digitalRead(_pin_event) == LOW && laststate_event == false && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = true;

    invert_state();
  }
  if (digitalRead(_pin_event) == HIGH && laststate_event == true && millis() - time_event > INTERVAL_EVENT_DEBOUNCE)
  {
    time_event = millis();
    laststate_event = false;
  }
}

void TouchT1::update_state(ulong state_new) {
  _state = (state_new > 0 && state > 0) ? 0 : state;
  state = state_new;

  digitalWrite(_pin_action, (state == 0 ? LOW : HIGH));
}

void TouchT1::invert_state() {
  if (state == 0) {
    if (_state == 0) { _state = 1; }
    update_state(_state);
  }
  else {
    update_state(0);
  }
}

void TouchT1::blynk(uint8_t mode, byte onboardled) {
  if (mode > 0)
  {
    uint16_t interval = (mode == 1) ? INTERVAL_LED_SETUP : INTERVAL_LED_SMARTCONFIG;

    if (millis() - time_led > interval) {
      time_led = millis();
      laststate_led = !laststate_led;
      digitalWrite(_pin_led, !laststate_led); // LED circuit inverted
    }
  }
  else {
    if (millis() - time_led > INTERVAL_LED_SETUP) {
      time_led = millis();
      digitalWrite(_pin_led, (onboardled == 0 ? HIGH : LOW)); // LED circuit inverted
    }
  }
}
