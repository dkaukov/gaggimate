#ifndef DIGITALINPUT_H
#define DIGITALINPUT_H

#include <Arduino.h>
#include <platform/Threading.h>

constexpr int INPUT_CHECK_INTERVAL_MS = 20;
constexpr int INPUT_DEBOUNCE_MS = 30;

using input_callback_t = std::function<void(const bool state)>;

class DigitalInput {
  public:
    DigitalInput(uint8_t pin, const input_callback_t &callback);
    void setup();
    void loop();

  private:
    uint8_t _pin;
    uint8_t _last_state = HIGH;
    uint8_t _raw_state = HIGH;
    unsigned long _last_change_ms = 0;
    xTaskHandle taskHandle;
    input_callback_t _callback;

    const char *LOG_TAG = "Heater";
    static void loopTask(void *arg);
};

#endif // DIGITALINPUT_H
