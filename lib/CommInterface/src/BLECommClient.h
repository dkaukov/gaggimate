#ifndef BLECOMMCLIENT_H
#define BLECOMMCLIENT_H

#include "CommInterface.h"

#ifdef ESP32

#include "NimBLEClientController.h"

/**
 * @brief BLE implementation of ICommClient
 *
 * Wraps the existing NimBLEClientController to provide the ICommClient interface.
 * This maintains backward compatibility with the existing BLE communication.
 */
class BLECommClient : public ICommClient {
  public:
    BLECommClient();
    ~BLECommClient() override = default;

    void init() override;
    void loop() override;
    bool connect() override;
    bool isReadyForConnection() override;
    bool isConnected() override;
    String readInfo() override;

    // Send methods
    void sendOutputControl(bool valve, float pumpSetpoint, float boilerSetpoint) override;
    void sendAdvancedOutputControl(bool valve, float boilerSetpoint, bool pressureTarget, float pressure,
                                   float flow) override;
    void sendAltControl(bool pinState) override;
    void sendPing() override;
    void sendAutotune(int testTime, int samples) override;
    void sendPidSettings(const String &pid) override;
    void sendPumpModelCoeffs(const String &pumpModelCoeffs) override;
    void setPressureScale(float scale) override;
    void sendLedControl(uint8_t channel, uint8_t brightness) override;
    void tare() override;

    // Register callback methods
    void registerRemoteErrorCallback(const comm_remote_err_callback_t &callback) override;
    void registerBrewBtnCallback(const comm_brew_callback_t &callback) override;
    void registerSteamBtnCallback(const comm_steam_callback_t &callback) override;
    void registerSensorCallback(const comm_sensor_read_callback_t &callback) override;
    void registerAutotuneResultCallback(const comm_pid_control_callback_t &callback) override;
    void registerVolumetricMeasurementCallback(const comm_float_callback_t &callback) override;
    void registerTofMeasurementCallback(const comm_int_callback_t &callback) override;

    /**
     * @brief Get direct access to underlying BLE controller
     * Useful for BLE-specific operations
     */
    NimBLEClientController *getBLEController() { return &_ble; }

  private:
    NimBLEClientController _ble;
};

#endif // ESP32

#endif // BLECOMMCLIENT_H
