#include "STM32DimmedPump.h"

#ifndef ESP32

#include <platform/Logger.h>

// Static instance pointer for ISR
STM32DimmedPump *STM32DimmedPump::_instance = nullptr;

STM32DimmedPump::STM32DimmedPump(uint8_t ssrPin, uint8_t zeroCrossPin, PressureSensor *pressureSensor,
                                 uint8_t acFrequency)
    : _ssrPin(ssrPin), _zeroCrossPin(zeroCrossPin), _acFrequency(acFrequency), _pressureSensor(pressureSensor),
      _pressureController(0.03f, &_ctrlPressure, &_ctrlFlow, &_currentPressure, &_controllerPower, &_valveStatus) {
    _instance = this;
}

void STM32DimmedPump::setup() {
    // Configure SSR pin as output
    pinMode(_ssrPin, OUTPUT);
    digitalWrite(_ssrPin, LOW);

    // Configure zero-cross pin as input
    pinMode(_zeroCrossPin, INPUT);

    // Setup zero-cross interrupt
    setupZeroCrossInterrupt();

    // Start the control loop task
    xTaskCreate(loopTask, "STM32DimmedPump::loop", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);

    LOG_I(LOG_TAG, "STM32 Dimmed Pump initialized in burst-fire mode, AC freq: %dHz", _acFrequency);
}

void STM32DimmedPump::setupZeroCrossInterrupt() {
    // Attach interrupt on falling edge (zero-cross detection)
    attachInterrupt(digitalPinToInterrupt(_zeroCrossPin), zeroCrossISR, FALLING);
}

void STM32DimmedPump::zeroCrossISR() {
    if (_instance) {
        _instance->onZeroCross();
    }
}

void STM32DimmedPump::onZeroCross() {
    unsigned long now = millis();
    if (_lastInterruptMillis > 0 && (now - _lastInterruptMillis) < MIN_INTERRUPT_DIFF_MS) {
        return;
    }
    _lastInterruptMillis = now;

    if (_dividerCounter >= _divider - 1) {
        _dividerCounter -= _divider - 1;
        calculateSkip();
    } else {
        _dividerCounter++;
    }
}

void STM32DimmedPump::calculateSkip() {
    _accumulator += _burstValue;

    if (_accumulator >= _range) {
        _accumulator -= _range;
        _skip = false;
    } else {
        _skip = true;
    }

    if (_accumulator > _range) {
        _accumulator = 0;
        _skip = false;
    }

    if (!_skip) {
        _cycleCounter++;
    }

    updateControl();
}

void STM32DimmedPump::updateControl() { digitalWrite(_ssrPin, _skip ? LOW : HIGH); }

void STM32DimmedPump::applyBurstValue(float powerPercent) {
    const float clampedPower = constrain(powerPercent, 0.0f, 100.0f);
    _burstValue = static_cast<unsigned int>(clampedPower);
}

void STM32DimmedPump::loop() {
    // Update pressure reading
    if (_pressureSensor) {
        _currentPressure = _pressureSensor->getRawPressure();
    }

    // Update power based on control mode
    updatePower();

    // Update flow estimation
    _currentFlow = _pressureController.getPumpFlowRate();
}

void STM32DimmedPump::setPower(float setpoint) {
    LOG_V(LOG_TAG, "Setting power to %.2f", setpoint);
    _ctrlPressure = setpoint > 0 ? 20.0f : 0.0f;
    _mode = ControlMode::POWER;
    _power = constrain(setpoint, 0.0f, 100.0f);
    _controllerPower = _power;

    if (_power == 0.0f) {
        _currentFlow = 0.0f;
        _burstValue = 0;
        _skip = true;
        digitalWrite(_ssrPin, LOW);
    } else {
        applyBurstValue(_power);
    }
}

float STM32DimmedPump::getCoffeeVolume() { return _pressureController.getCoffeeOutputEstimate(); }

float STM32DimmedPump::getPumpFlow() { return _currentFlow; }

float STM32DimmedPump::getPuckFlow() { return _pressureController.getCoffeeFlowRate(); }

float STM32DimmedPump::getPuckResistance() { return _pressureController.getPuckResistance(); }

void STM32DimmedPump::tare() {
    _pressureController.tare();
    _pressureController.reset();
}

void STM32DimmedPump::loopTask(void *arg) {
    auto *pump = static_cast<STM32DimmedPump *>(arg);
    TickType_t lastWake = xTaskGetTickCount();
    while (true) {
        pump->loop();
        xTaskDelayUntil(&lastWake, pdMS_TO_TICKS(30));
    }
}

void STM32DimmedPump::updatePower() {
    _pressureController.update(static_cast<PressureController::ControlMode>(_mode));
    if (_mode != ControlMode::POWER) {
        _power = _controllerPower;
        applyBurstValue(_power);
    }
}

void STM32DimmedPump::setFlowTarget(float targetFlow, float pressureLimit) {
    _mode = ControlMode::FLOW;
    _ctrlFlow = targetFlow;
    _ctrlPressure = pressureLimit;
    _pressureController.setPressureLimit(pressureLimit);
}

void STM32DimmedPump::setPressureTarget(float targetPressure, float flowLimit) {
    _mode = ControlMode::PRESSURE;
    _ctrlFlow = flowLimit;
    _ctrlPressure = targetPressure;
    _pressureController.setFlowLimit(flowLimit);
}

void STM32DimmedPump::setValveState(bool open) { _valveStatus = open ? 1 : 0; }

void STM32DimmedPump::setPumpFlowCoeff(float oneBarFlow, float nineBarFlow) {
    _pressureController.setPumpFlowCoeff(oneBarFlow, nineBarFlow);
}

void STM32DimmedPump::setPumpFlowPolyCoeffs(float a, float b, float c, float d) {
    _pressureController.setPumpFlowPolyCoeffs(a, b, c, d);
}

void STM32DimmedPump::stop() { setPower(0.0f); }

void STM32DimmedPump::fullPower() { setPower(100.0f); }

#endif // !ESP32
