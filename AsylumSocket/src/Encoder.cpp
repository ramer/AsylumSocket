// Encoder.cpp

#include "Encoder.h"

Encoder::Encoder(byte event, byte action, byte eventA, byte eventB) : Device(event, action) {
  pin_eventA = eventA;
  pin_eventB = eventB;
};

void Encoder::initialize(PubSubClient *ptr_mqttClient, Config *ptr_config, String prefix) {
  // Device::initialize(ptr_mqttClient, ptr_config, prefix);     // fully overrided
  _mqttClient = ptr_mqttClient;
  _config = ptr_config;

  pinMode(pin_event, INPUT);
  pinMode(pin_action, OUTPUT);	digitalWrite(pin_action, LOW);		// default initial value
  pinMode(pin_eventA, INPUT);
  pinMode(pin_eventB, INPUT);

  generateUid(prefix);
  generateGlobalTopics();
  generateTopics();

  loadState();

  encoderstate = state;
  encoderinstance = this;
  attachInterrupt(digitalPinToInterrupt(pin_eventA), EncoderInterruptFunc, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_eventB), EncoderInterruptFunc, CHANGE);
}

void Encoder::update() {
  // process buttons
  if (buttonPressed(pin_event, &pin_event_laststate)) { invertState(&state, &state_old, &state_published, pin_action); saveState(); }

  // update state on encoder
  if (encoderstate != state) { updateState(encoderstate, &state, &state_old, &state_published, pin_action); }

  // check state published
  if (!_mqttClient) return;
  if (_mqttClient->connected() && !state_published && millis() - state_publishedtime > INTERVAL_STATE_PUBLISH) { publishState(mqtt_topic_pub, state, &state_published); }
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

/*

  CCW     CW 
  A B     A B

  0 1     0 0
  0 0     1 0
  0 1     1 1
  1 1

*/

  if (seqA == 0b00000011 && seqB == 0b00001001) encoderstate += ENCODER_STEP; // CW
  //if (seqA == 0b00001011 && seqB == 0b00001001) encoderstate += ENCODER_STEP;
  if (seqA == 0b00001001 && seqB == 0b00000011) encoderstate -= ENCODER_STEP; // CCW
  //if (seqA == 0b00001001 && seqB == 0b00000011) encoderstate -= ENCODER_STEP;
  encoderstate = constrain(encoderstate, 0, 255);
}

void Encoder::updateState(ulong state_new, ulong *ptr_state, ulong *ptr_state_old, bool *ptr_state_published, byte pin) {
  if (ptr_state != 0 && ptr_state_old != 0 && ptr_state_published != 0) {
    // pointers passed
    *ptr_state_old = (state_new > 0 && *ptr_state > 0) ? 0 : *ptr_state;
    *ptr_state = state_new;
    analogWrite(pin, *ptr_state);
    *ptr_state_published = false;
  }
  else {
    // no pointers passed; take default states
    state_old = (state_new > 0 && state_old > 0) ? 0 : state_old;
    state = state_new;
    analogWrite(pin, state);
    state_published = false;
  }

  Serial.printf(" - state changed to %u \n", state_new);
  //updateStateCallback(state_new);
}
