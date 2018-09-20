// Encoder.cpp

#include "Encoder.h"

Encoder::Encoder(byte event, byte action, byte eventA, byte eventB) : Device(event, action) {
  pin_eventA = eventA;
  pin_eventB = eventB;
};

void Encoder::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  Device::initialize(ptr_mqttClient, ptr_config, prefix);

  pinMode(pin_eventA, INPUT);
  pinMode(pin_eventB, INPUT);

  encoderinstance = this;
  attachInterrupt(digitalPinToInterrupt(pin_eventA), EncoderInterruptFunc, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_eventB), EncoderInterruptFunc, CHANGE);
}

void Encoder::doEncoder() {
  bool a = digitalRead(pin_eventA);
  bool b = digitalRead(pin_eventB);

  if (pin_event_laststateA == a && pin_event_laststateB == b) return;
  pin_event_laststateA = a; pin_event_laststateB = b;

  seqA <<= 1;
  seqA |= a;

  seqB <<= 1;
  seqB |= b;

  // mask upper 4 bits
  seqA &= 0b00001111;
  seqB &= 0b00001111;
  
  // Serial.printf("%s %s \n", a ? "| " : " |", b ? "| " : " |");
  int new_state = state;
  if (seqA == 0b00000011 && seqB == 0b00001001) new_state += ENCODER_STEP; // CW
  //if (seqA == 0b00001011 && seqB == 0b00001001) encoderstate += ENCODER_STEP;
  if (seqA == 0b00001001 && seqB == 0b00000011) new_state -= ENCODER_STEP; // CCW
  //if (seqA == 0b00001001 && seqB == 0b00000011) encoderstate -= ENCODER_STEP;
  updateState(new_state < 0 ? 0 : (new_state > 255 ? 255 : new_state));
}

void Encoder::updateState(ulong state_new) {
  state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
  state = state_new;
  state_published = false;

  if (state_new >= 250) { digitalWrite(pin_action, HIGH); }
  else if (state_new > 15 && state_new < 250) { analogWrite(pin_action, state_new << 2); }  // esp8266 uses 10 bit PWM
  else { digitalWrite(pin_action, LOW); }
  
  //Serial.printf(" - state changed to %u \n", state_new);
}

void Encoder::invertState() {
  if (state == 0) {
    if (state_old == 0) { state_old = 255; }
    updateState(state_old);
  }
  else {
    updateState(0);
  }
}
