#ifndef STM32DIMMEDPUMP_H
#define STM32DIMMEDPUMP_H

#ifndef ESP32

#include "PressureController/PressureController.h"
#include "PressureSensor.h"
#include "Pump.h"
#include <Arduino.h>
#include <platform/Threading.h>

/**
 * @brief STM32-specific dimmed pump driver
 *
 * Implements the same burst-fire / cycle-skipping behavior as the original
 * ESP32 PSM-based driver:
 * - Zero-cross detection via external interrupt
 * - Full-cycle enable/skip decisions using an error accumulator
 * - SSR control held high or low across whole AC cycles
 */
class STM32DimmedPump : public Pump {
  public:
    enum class ControlMode { POWER, PRESSURE, FLOW };

    /**
     * @brief Construct STM32 Dimmed Pump
     * @param ssrPin SSR control pin
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

    volatile uint8_t _divider = 2;
    volatile uint8_t _dividerCounter = 1;
    volatile unsigned int _range = 100;
    volatile unsigned int _burstValue = 0;
    volatile unsigned int _accumulator = 0;
    volatile bool _skip = true;
    volatile long _cycleCounter = 0;
    volatile unsigned long _lastInterruptMillis = 0;

    static constexpr float BASE_FLOW_RATE = 0.25f;
    static constexpr float MAX_PRESSURE = 15.0f;
    static constexpr unsigned long MIN_INTERRUPT_DIFF_MS = 4;

    void updatePower();
    void applyBurstValue(float powerPercent);
    void calculateSkip();
    void updateControl();
    void setupZeroCrossInterrupt();

    const char *LOG_TAG = "STM32DimmedPump";
    static void loopTask(void *arg);

    // Static instance pointer for ISR access
    static STM32DimmedPump *_instance;
    static void zeroCrossISR();
};

#endif // !ESP32

#endif // STM32DIMMEDPUMP_H
