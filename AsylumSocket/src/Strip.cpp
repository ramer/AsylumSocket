// Strip.cpp

#include "Strip.h"

Strip::Strip(byte event, byte action) : Device(event, action) {};

void Strip::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  Device::initialize(ptr_mqttClient, ptr_config, prefix);

  strip = Adafruit_NeoPixel(STRIP_LEDCOUNT, pin_action, NEO_GRB + NEO_KHZ800); //rgb
}

void Strip::update() {
  // process buttons
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(&state, &state_old, &state_published, pin_action); saveState(); }

  // update state of strip
  update_strip();

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
}

void Strip::update_strip() {
  if (millis() - time_strip_frame > INTERVAL_STRIP_FRAME) {
    time_strip_frame = millis();

    if (state == 1) {
      frame_rainbow();
    }
    else if (state == 2) {
      frame_stars();
    }
    else if (state == 3) {
      frame_sunrise();
    }
    else {
      frame_solid();
    }
  }
}

void Strip::frame_solid() {
  if (solid_laststate != state) {
    solid_laststate = state;

    uint8_t r = state >> 16;
    uint8_t g = state >> 8 & 0xFF;
    uint8_t b = state & 0xFF;

    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    }

    strip.show();
    sunrise = 0; // prevent sunrise start from the middle
  }
}

void Strip::frame_rainbow() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip_wheel((((i + rainbow_offset) * 256 / strip.numPixels())) & 255));
  }

  rainbow_offset++;
  strip.show();
}

void Strip::frame_stars() {
  if (random(STARS_PROBABILITY) == 0) { stars[random(strip.numPixels() - 1)] = STARS_INCREMENT; }

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (stars[i] > 0) { stars[i] = stars[i] + STARS_INCREMENT; }
  }

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(stars[i], stars[i], stars[i]));
  }
  strip.show();
}

void Strip::frame_sunrise() {
  sunrise++;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(sunrise >> 8, sunrise >> 8, sunrise >> 8));
  }

  if (sunrise >= 65535) {
    sunrise = 0;
    updateState(16777215, &state, &state_old, &state_published, pin_action); saveState();
  }

  strip.show();
}

uint32_t Strip::strip_wheel(byte angle) {
  angle = 255 - angle;
  if (angle < 85) {
    return strip.Color(255 - angle * 3, 0, angle * 3);
  }
  if (angle < 170) {
    angle -= 85;
    return strip.Color(0, angle * 3, 255 - angle * 3);
  }
  angle -= 170;
  return strip.Color(angle * 3, 255 - angle * 3, 0);
}