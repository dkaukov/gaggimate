#include "DistanceSensor.h"
#include <platform/Logger.h>
#include <platform/Threading.h>

DistanceSensor::DistanceSensor(TwoWire *wire, distance_callback_t callback) : i2c(wire), _callback(callback) {
    this->tof = new VL53L0X();
}

void DistanceSensor::setup() {
    this->tof->setAddress(0x7E);
    this->tof->setBus(i2c);
    this->tof->setTimeout(1000);
    if (!this->tof->init()) {
        LOG_E("DistanceSensor", "Failed to initialize VL53L0X");
    } else {
        LOG_I("DistanceSensor", "Initialized VL53L0X");
        this->tof->startContinuous(250);
        xTaskCreate(loopTask, "DistanceSensor::loop", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);
    }
}

void DistanceSensor::loop() {
    int millis = tof->readRangeContinuousMillimeters();
    if (tof->timeoutOccurred()) {
        LOG_E("DistanceSensor", "ToF Timeout");
        return;
    }
    currentMillis = currentMillis == 0 ? millis : static_cast<int>(currentMillis * 0.9 + static_cast<double>(millis) * 0.1);
    measurements = (measurements + 1) % 10;
    if (measurements == 0) {
        _callback(currentMillis);
    }
    LOG_V("DistanceSensor", "Received measurement: %d (%d objects)", currentMillis);
}

void DistanceSensor::loopTask(void *arg) {
    auto *sensor = static_cast<DistanceSensor *>(arg);
    TickType_t lastWake = xTaskGetTickCount();
    while (true) {
        sensor->loop();
        xTaskDelayUntil(&lastWake, pdMS_TO_TICKS(500));
    }
}
