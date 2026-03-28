#include "DigitalInput.h"

DigitalInput::DigitalInput(uint8_t pin, const input_callback_t &callback) : _pin(pin), _callback(callback) {}

void DigitalInput::setup() {
    pinMode(_pin, INPUT_PULLUP);
    _last_state = digitalRead(_pin);
    _raw_state = _last_state;
    _last_change_ms = millis();
    xTaskCreate(loopTask, "DigitalInput::loop", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);
}

void DigitalInput::loop() {
    uint8_t currentState = digitalRead(_pin);

    if (currentState != _raw_state) {
        _raw_state = currentState;
        _last_change_ms = millis();
    }

    if (_raw_state != _last_state && millis() - _last_change_ms >= INPUT_DEBOUNCE_MS) {
        _last_state = _raw_state;
        _callback(!_last_state);
    }
}

void DigitalInput::loopTask(void *arg) {
    auto *input = static_cast<DigitalInput *>(arg);
    while (true) {
        input->loop();
        vTaskDelay(INPUT_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}
