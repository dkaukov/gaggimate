#ifndef BLECOMMSERVER_H
#define BLECOMMSERVER_H

#include "CommInterface.h"

#ifdef ESP32

#include "NimBLEServerController.h"

/**
 * @brief BLE implementation of ICommServer
 *
 * Wraps the existing NimBLEServerController to provide the ICommServer interface.
 * This maintains backward compatibility with the existing BLE communication.
 */
class BLECommServer : public ICommServer {
  public:
    BLECommServer();
    ~BLECommServer() override = default;

    void init(const String &infoString) override;
    void setInfo(const String &infoString) override;
    void loop() override;
    bool isConnected() override;

    // Send methods
    void sendSensorData(float temperature, float pressure, float puckFlow, float pumpFlow, float puckResistance) override;
    void sendError(int errorCode) override;
    void sendBrewBtnState(bool brewButtonStatus) override;
    void sendSteamBtnState(bool steamButtonStatus) override;
    void sendAutotuneResult(float Kp, float Ki, float Kd) override;
    void sendVolumetricMeasurement(float value) override;
    void sendTofMeasurement(int value) override;

    // Register callback methods
    void registerOutputControlCallback(const comm_simple_output_callback_t &callback) override;
    void registerAdvancedOutputControlCallback(const comm_advanced_output_callback_t &callback) override;
    void registerAltControlCallback(const comm_pin_control_callback_t &callback) override;
    void registerPidControlCallback(const comm_pid_control_callback_t &callback) override;
    void registerPumpModelCoeffsCallback(const comm_pump_model_coeffs_callback_t &callback) override;
    void registerPingCallback(const comm_ping_callback_t &callback) override;
    void registerAutotuneCallback(const comm_autotune_callback_t &callback) override;
    void registerPressureScaleCallback(const comm_float_callback_t &callback) override;
    void registerTareCallback(const comm_void_callback_t &callback) override;
    void registerLedControlCallback(const comm_led_control_callback_t &callback) override;

    /**
     * @brief Get direct access to underlying BLE controller
     * Useful for BLE-specific operations like OTA
     */
    NimBLEServerController *getBLEController() { return &_ble; }

  private:
    NimBLEServerController _ble;
    bool _connected = false;
};

#endif // ESP32

#endif // BLECOMMSERVER_H
