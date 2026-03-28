#ifndef MAX6675THERMOCOUPLE_H
#define MAX6675THERMOCOUPLE_H

// Only compile for STM32 (Gaggiuino Lego V3 uses MAX6675)
#ifndef ESP32

#include "TemperatureSensor.h"
#include <MAX6675.h>
#include <platform/Threading.h>

constexpr int MAX6675_UPDATE_INTERVAL = 250;
constexpr int MAX6675_ERROR_WINDOW = 20;
constexpr float MAX6675_MAX_ERROR_RATE = 0.5f;
constexpr int MAX6675_MAX_ERRORS = static_cast<int>(static_cast<float>(MAX6675_ERROR_WINDOW) * MAX6675_MAX_ERROR_RATE);
constexpr double MAX6675_MAX_SAFE_TEMP = 170.0;

using temperature_callback_t = std::function<void(float)>;
using temperature_error_callback_t = std::function<void()>;

class Max6675Thermocouple : public TemperatureSensor {
  public:
    Max6675Thermocouple(int csPin, int misoPin, int sckPin, const temperature_callback_t &callback,
                        const temperature_error_callback_t &error_callback);
    float read() override;
    bool isErrorState() override;

    void setup() override;
    void loop();

  private:
    MAX6675 *max6675;
    xTaskHandle taskHandle;

    int errorCount = 0;
    std::array<int, MAX6675_ERROR_WINDOW> resultBuffer{};
    size_t resultCount = 0;
    size_t bufferIndex = 0;

    float temperature = .0f;

    int csPin = 0;
    int misoPin = 0;
    int sckPin = 0;

    temperature_callback_t callback;
    temperature_error_callback_t error_callback;

    const char *LOG_TAG = "Max6675Thermocouple";
    [[noreturn]] static void monitorTask(void *arg);
};

#endif // !ESP32

#endif // MAX6675THERMOCOUPLE_H
