// Only compile for STM32 (Gaggiuino Lego V3 uses MAX6675)
#ifndef ESP32

#include "Max6675Thermocouple.h"
#include <Arduino.h>
#include <SPI.h>
#include <platform/Logger.h>
#include <platform/Threading.h>

Max6675Thermocouple::Max6675Thermocouple(const int csPin, const int misoPin, const int sckPin,
                                         const temperature_callback_t &callback,
                                         const temperature_error_callback_t &error_callback)
    : taskHandle(nullptr), csPin(csPin), misoPin(misoPin), sckPin(sckPin) {
    // MAX6675 constructor: select, sck, miso (software SPI)
    max6675 = new MAX6675(csPin, sckPin, misoPin);
    this->callback = callback;
    this->error_callback = error_callback;
}

float Max6675Thermocouple::read() { return isErrorState() ? 0.0f : temperature; }

bool Max6675Thermocouple::isErrorState() { return temperature <= 0 || errorCount >= MAX6675_MAX_ERRORS; }

void Max6675Thermocouple::setup() {
    SPI.begin();
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    max6675->begin();
    max6675->setSPIspeed(1000000);

    xTaskCreate(monitorTask, "Max6675Thermocouple::monitor", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);
}

void Max6675Thermocouple::loop() {
    if (errorCount >= MAX6675_MAX_ERRORS || temperature > MAX6675_MAX_SAFE_TEMP) {
        LOG_E(LOG_TAG, "Thermocouple failure! Error Count: %d, Temperature: %.2f\n", errorCount, temperature);
        error_callback();
        return;
    }
    // If buffer has been filled up, remove the previous result from the error count
    if (resultCount == MAX6675_ERROR_WINDOW) {
        errorCount -= resultBuffer[bufferIndex];
    } else {
        ++resultCount;
    }

    float temp;
    int status = max6675->read();
    if (status != STATUS_OK) {
        LOG_E(LOG_TAG, "Failed to read temperature: %d\n", status);
        temp = 0.0f;
    } else {
        temp = max6675->getTemperature();
    }

    if (temp <= 0.0f) {
        LOG_E(LOG_TAG, "Temperature reported below 0°C: %.2f\n", temp);
    }

    resultBuffer[bufferIndex] = temp <= 0.0f ? 1 : 0;
    errorCount += resultBuffer[bufferIndex];
    bufferIndex = (bufferIndex + 1) % MAX6675_ERROR_WINDOW;

    if (temp <= 0.0f)
        return;
    temperature = 0.2f * temp + 0.8f * temperature;
    LOG_V(LOG_TAG, "Updated temperature: %2f\n", temperature);
    callback(temperature);
}

[[noreturn]] void Max6675Thermocouple::monitorTask(void *arg) {
    TickType_t lastWake = xTaskGetTickCount();
    auto *thermocouple = static_cast<Max6675Thermocouple *>(arg);
    while (true) {
        thermocouple->loop();
        xTaskDelayUntil(&lastWake, pdMS_TO_TICKS(MAX6675_UPDATE_INTERVAL));
    }
}

#endif // !ESP32
