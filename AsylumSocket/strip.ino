#if DEVICE_TYPE == 4


ulong     time_strip_frame = 0;
bool      strip_updated_flag;

#define   STARS_PROBABILITY 100
#define   STARS_INCREMENT 1

uint8_t   rainbow_offset;
uint8_t   stars[STRIP_LEDCOUNT];
uint16_t  sunrise;

void update_strip() {
  if (millis() - time_strip_frame > INTERVAL_STRIP_FRAME) {
    time_strip_frame = millis();
    // max value is 16777215 = white color
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
      if (strip_updated_flag == false) {
        uint8_t r = state >> 16;
        uint8_t g = state >> 8 & 0xFF;
        uint8_t b = state & 0xFF;

        for (int i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(r, g, b));
        }

        strip.show();
        strip_updated_flag = true;

        sunrise = 0; // prevent sunrise start from the middle
      }
    }
  }
}


void frame_rainbow() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip_wheel((((i + rainbow_offset) * 256 / strip.numPixels())) & 255));
  }

  rainbow_offset++;
  strip.show();
}

void frame_stars() {
  if (random(STARS_PROBABILITY) == 0) { stars[random(STRIP_LEDCOUNT - 1)] = STARS_INCREMENT; }

  for (uint16_t i = 0; i < STRIP_LEDCOUNT; i++) {
    if (stars[i] > 0) { stars[i] = stars[i] + STARS_INCREMENT; }
  }

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(stars[i], stars[i], stars[i]));
  }
  strip.show();
}

void frame_sunrise() {
  sunrise++;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(sunrise >> 8, sunrise >> 8, sunrise >> 8));
  }

  if (sunrise >= 65535) {
    sunrise = 0;
    updateState(16777215);
  }

  strip.show();
}

uint32_t strip_wheel(byte angle) {
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

#endif

