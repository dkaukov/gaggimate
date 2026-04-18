#include "PressureSensor.h"
#include "Wire.h"
#include <algorithm>
#include <platform/Logger.h>
#include <platform/Threading.h>

// Compatibility: std::clamp requires C++17
#if __cplusplus < 201703L
namespace std {
template <typename T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}
} // namespace std
#endif

PressureSensor::PressureSensor(uint8_t sda_pin, uint8_t scl_pin, const pressure_callback_t &callback, float pressure_scale,
                               float voltage_floor, float voltage_ceil)
    : _sda_pin(sda_pin), _scl_pin(scl_pin), _pressure_scale(pressure_scale), _callback(callback), taskHandle(nullptr) {
    _adc_floor = static_cast<int16_t>(voltage_floor / ADC_STEP);
    _pressure_adc_range = (voltage_ceil - voltage_floor) / ADC_STEP;
    _pressure_step = pressure_scale / _pressure_adc_range;
}

bool PressureSensor::setup() {
#ifdef ESP32
    Wire1.begin(_sda_pin, _scl_pin);
    LOG_V(LOG_TAG, "Initializing pressure sensor on SDA: %d, SCL: %d", _sda_pin, _scl_pin);
    delay(100);
    ads = new ADS1115(0x48, &Wire1);
#else
    // STM32 uses Wire with setPins
    Wire.setSDA(_sda_pin);
    Wire.setSCL(_scl_pin);
    Wire.begin();
    LOG_V(LOG_TAG, "Initializing pressure sensor on SDA: %d, SCL: %d", _sda_pin, _scl_pin);
    delay(100);
    ads = new ADS1115(0x48, &Wire);
#endif
    if (!ads->begin()) {
        LOG_E(LOG_TAG, "Failed to initialize ADS1115");
        _available = false;
        return false;
    }
    _available = true;
    ads->setGain(0);
    ads->setDataRate(4);
    ads->setMode(0);
    ads->readADC(0);
    xTaskCreate(loopTask, "PressureSensor::loop", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);
    return true;
}

void PressureSensor::loop() {
    if (_available && ads->isConnected()) {
        int16_t reading = ads->readADC();
        reading = reading - _adc_floor;
        float pressure = reading * _pressure_step;
        _raw_pressure = pressure;
        _pressure = 0.05f * pressure + 0.95f * _pressure;
        _raw_pressure = std::clamp(_raw_pressure, 0.0f, _pressure_scale);
        _pressure = std::clamp(_pressure, 0.0f, _pressure_scale);
        LOG_V(LOG_TAG, "ADC Reading: %d, Pressure Reading: %f, Pressure Step: %f, Floor: %d", reading, _pressure,
                 _pressure_step, _adc_floor);
        _callback(_pressure);
    }
}

void PressureSensor::setScale(float pressure_scale) {
    _pressure_scale = pressure_scale;
    _pressure_step = pressure_scale / _pressure_adc_range;
}

[[noreturn]] void PressureSensor::loopTask(void *arg) {
    TickType_t lastWake = xTaskGetTickCount();
    auto *sensor = static_cast<PressureSensor *>(arg);
    while (true) {
        sensor->loop();
        xTaskDelayUntil(&lastWake, pdMS_TO_TICKS(PRESSURE_READ_INTERVAL_MS));
    }
}
