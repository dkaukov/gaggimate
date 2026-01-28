#include "SimpleRelay.h"
#include <platform/Logger.h>

SimpleRelay::SimpleRelay(int pin, uint8_t onState) : state(false) {
    this->_pin = pin;
    this->_onState = onState;
}

void SimpleRelay::setup() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, !_onState);
}

void SimpleRelay::set(bool state) {
    this->state = state;
    digitalWrite(_pin, state ? _onState : !_onState);
    LOG_V(LOG_TAG, "Switching to: %u", state);
}
