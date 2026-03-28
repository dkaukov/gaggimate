#ifndef STM32DIMMEDPUMP_H
#define STM32DIMMEDPUMP_H

#ifndef ESP32

#include "PressureController/PressureController.h"
#include "PressureSensor.h"
#include "Pump.h"
#include <HardwareTimer.h>
#include <Arduino.h>
#include <platform/Threading.h>

/**
 * @brief STM32-specific dimmed pump driver
 *
 * Implements phase-angle control for AC pump dimming using:
 * - Zero-cross detection via external interrupt
 * - Hardware timer for precise firing delay
 * - TRIAC/SSR control via GPIO
 *
 * This implementation uses STM32 hardware timers for more precise timing
 * compared to software-based approaches.
 */
class STM32DimmedPump : public Pump {
  public:
    enum class ControlMode { POWER, PRESSURE, FLOW };

    /**
     * @brief Construct STM32 Dimmed Pump
     * @param ssrPin TRIAC/SSR gate pin
     * @param zeroCrossPin Zero-cross detector input pin
     * @param pressureSensor Pointer to pressure sensor (can be nullptr)
     * @param acFrequency AC mains frequency (50 or 60 Hz)
     */
    STM32DimmedPump(uint8_t ssrPin, uint8_t zeroCrossPin, PressureSensor *pressureSensor, uint8_t acFrequency = 50);
    ~STM32DimmedPump() = default;

    void setup() override;
    void loop() override;
    void setPower(float setpoint) override;

    float getCoffeeVolume();
    float getPumpFlow();
    float getPuckFlow();
    float getPuckResistance();
    float *getPumpFlowPtr() { return &_currentFlow; }
    int *getValveStatusPtr() { return &_valveStatus; }
    void tare();

    void setFlowTarget(float targetFlow, float pressureLimit);
    void setPressureTarget(float targetPressure, float flowLimit);
    void setPumpFlowCoeff(float oneBarFlow, float nineBarFlow);
    void setPumpFlowPolyCoeffs(float a, float b, float c, float d);
    void stop();
    void fullPower();
    void setValveState(bool open);

    // Interrupt handlers (called from ISR)
    void onZeroCross();
    void onTimerFire();

  private:
    uint8_t _ssrPin;
    uint8_t _zeroCrossPin;
    uint8_t _acFrequency;
    PressureSensor *_pressureSensor;
    PressureController _pressureController;
    xTaskHandle taskHandle;

    ControlMode _mode = ControlMode::POWER;
    float _power = 0.0f;
    float _controllerPower = 0.0f;
    float _ctrlPressure = 0.0f;
    float _ctrlFlow = 0.0f;
    float _currentPressure = 0.0f;
    float _currentFlow = 0.0f;
    float _lastPressure = 0.0f;
    int _valveStatus = 0;

    // Timing parameters
    uint32_t _halfCycleMicros;   // Half-cycle duration in microseconds
    uint32_t _firingDelayMicros; // Delay from zero-cross to fire TRIAC
    volatile bool _pendingFire = false;
    volatile unsigned long _lastZeroCross = 0;

    static constexpr float BASE_FLOW_RATE = 0.25f;
    static constexpr float MAX_PRESSURE = 15.0f;
    static constexpr float MIN_FIRING_DELAY_US = 500;   // Minimum delay after zero-cross
    static constexpr float MAX_FIRING_DELAY_US = 9000;  // Maximum delay (for 50Hz)
    static constexpr float TRIAC_PULSE_WIDTH_US = 100;  // Gate pulse duration

    void updatePower();
    void setFiringDelay(float powerPercent);
    void setupZeroCrossInterrupt();
    void setupTimer();

    const char *LOG_TAG = "STM32DimmedPump";
    static void loopTask(void *arg);
    static void timerISR();

    // Static instance pointer for ISR access
    static STM32DimmedPump *_instance;
    static HardwareTimer *_timer;
    static void zeroCrossISR();
};

#endif // !ESP32

#endif // STM32DIMMEDPUMP_H
