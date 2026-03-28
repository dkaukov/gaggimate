#include "STM32DimmedPump.h"

#ifndef ESP32

#include <platform/Logger.h>

// Static instance pointer for ISR
STM32DimmedPump *STM32DimmedPump::_instance = nullptr;
HardwareTimer *STM32DimmedPump::_timer = nullptr;

STM32DimmedPump::STM32DimmedPump(uint8_t ssrPin, uint8_t zeroCrossPin, PressureSensor *pressureSensor,
                                 uint8_t acFrequency)
    : _ssrPin(ssrPin), _zeroCrossPin(zeroCrossPin), _acFrequency(acFrequency), _pressureSensor(pressureSensor),
      _pressureController(0.03f, &_ctrlPressure, &_ctrlFlow, &_currentPressure, &_controllerPower, &_valveStatus) {

    // Calculate half-cycle duration based on AC frequency
    // 50Hz: 10000us per half-cycle
    // 60Hz: 8333us per half-cycle
    _halfCycleMicros = 1000000UL / (2 * _acFrequency);
    _firingDelayMicros = _halfCycleMicros; // Start at 0% power (max delay)

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

    // Setup timer for firing delay
    setupTimer();

    // Start the control loop task
    xTaskCreate(loopTask, "STM32DimmedPump::loop", configMINIMAL_STACK_SIZE * 4, this, 1, &taskHandle);

    LOG_I(LOG_TAG, "STM32 Dimmed Pump initialized, AC freq: %dHz, half-cycle: %luus", _acFrequency, _halfCycleMicros);
}

void STM32DimmedPump::setupZeroCrossInterrupt() {
    // Attach interrupt on falling edge (zero-cross detection)
    attachInterrupt(digitalPinToInterrupt(_zeroCrossPin), zeroCrossISR, FALLING);
}

void STM32DimmedPump::setupTimer() {
    if (_timer == nullptr) {
        _timer = new HardwareTimer(TIM2);
    }

    _timer->pause();
    _timer->setOverflow(_halfCycleMicros, MICROSEC_FORMAT);
    _timer->attachInterrupt(timerISR);
    _timer->setCount(0, MICROSEC_FORMAT);
    _timer->refresh();
}

void STM32DimmedPump::zeroCrossISR() {
    if (_instance) {
        _instance->onZeroCross();
    }
}

void STM32DimmedPump::onZeroCross() {
    _lastZeroCross = micros();

    if (_power > 0.0f && _firingDelayMicros < (_halfCycleMicros - TRIAC_PULSE_WIDTH_US)) {
        _pendingFire = true;
        if (_timer != nullptr) {
            _timer->pause();
            _timer->setOverflow(_firingDelayMicros, MICROSEC_FORMAT);
            _timer->setCount(0, MICROSEC_FORMAT);
            _timer->refresh();
            _timer->resume();
        }
    }
}

void STM32DimmedPump::onTimerFire() {
    if (_timer != nullptr) {
        _timer->pause();
    }

    // Fire the TRIAC with a short pulse
    digitalWrite(_ssrPin, HIGH);
    delayMicroseconds(static_cast<unsigned int>(TRIAC_PULSE_WIDTH_US));
    digitalWrite(_ssrPin, LOW);
    _pendingFire = false;
}

void STM32DimmedPump::timerISR() {
    if (_instance) {
        _instance->onTimerFire();
    }
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
        _firingDelayMicros = _halfCycleMicros; // Max delay = 0% power
        _pendingFire = false;
        if (_timer != nullptr) {
            _timer->pause();
            _timer->setCount(0, MICROSEC_FORMAT);
            _timer->refresh();
        }
        digitalWrite(_ssrPin, LOW);
    } else {
        setFiringDelay(_power);
    }
}

void STM32DimmedPump::setFiringDelay(float powerPercent) {
    // Convert power percentage to firing delay
    // 0% power = maximum delay (fire at end of half-cycle, almost no conduction)
    // 100% power = minimum delay (fire immediately after zero-cross)
    // Inverse relationship: higher power = lower delay

    float delayRange = MAX_FIRING_DELAY_US - MIN_FIRING_DELAY_US;
    _firingDelayMicros = static_cast<uint32_t>(MAX_FIRING_DELAY_US - (powerPercent / 100.0f) * delayRange);
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
        setFiringDelay(_power);
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
