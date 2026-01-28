#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

#include <Arduino.h>
#include <functional>

// Error codes shared between all communication implementations
constexpr size_t COMM_ERROR_CODE_NONE = 0;
constexpr size_t COMM_ERROR_CODE_COMM_SEND = 1;
constexpr size_t COMM_ERROR_CODE_COMM_RCV = 2;
constexpr size_t COMM_ERROR_CODE_PROTO_ERR = 3;
constexpr size_t COMM_ERROR_CODE_RUNAWAY = 4;
constexpr size_t COMM_ERROR_CODE_TIMEOUT = 5;

// Callback type definitions for server-side (controller)
using comm_pin_control_callback_t = std::function<void(bool isActive)>;
using comm_pid_control_callback_t = std::function<void(float Kp, float Ki, float Kd, float Kf)>;
using comm_pump_model_coeffs_callback_t = std::function<void(float a, float b, float c, float d)>;
using comm_ping_callback_t = std::function<void()>;
using comm_autotune_callback_t = std::function<void(int testTime, int samples)>;
using comm_void_callback_t = std::function<void()>;
using comm_float_callback_t = std::function<void(float val)>;
using comm_simple_output_callback_t = std::function<void(bool valve, float pumpSetpoint, float boilerSetpoint)>;
using comm_advanced_output_callback_t =
    std::function<void(bool valve, float boilerSetpoint, bool pressureTarget, float pumpPressure, float pumpFlow)>;
using comm_led_control_callback_t = std::function<void(uint8_t channel, uint8_t brightness)>;

// Callback type definitions for client-side (display)
using comm_remote_err_callback_t = std::function<void(int errorCode)>;
using comm_brew_callback_t = std::function<void(bool brewButtonStatus)>;
using comm_steam_callback_t = std::function<void(bool steamButtonStatus)>;
using comm_sensor_read_callback_t =
    std::function<void(float temperature, float pressure, float puckFlow, float pumpFlow, float puckResistance)>;
using comm_int_callback_t = std::function<void(int val)>;

/**
 * @brief Abstract interface for communication server (runs on controller board)
 *
 * This interface abstracts the communication layer used by the GaggiMate controller
 * to receive commands from and send data to the display board.
 */
class ICommServer {
  public:
    virtual ~ICommServer() = default;

    /**
     * @brief Initialize the communication server
     * @param infoString JSON string containing system information
     */
    virtual void init(const String &infoString) = 0;

    /**
     * @brief Update system info string
     * @param infoString JSON string containing system information
     */
    virtual void setInfo(const String &infoString) = 0;

    /**
     * @brief Process incoming messages and handle communication tasks
     * Should be called regularly from the main loop
     */
    virtual void loop() = 0;

    /**
     * @brief Check if a client is connected
     * @return true if connected
     */
    virtual bool isConnected() = 0;

    // Send methods (controller -> display)
    virtual void sendSensorData(float temperature, float pressure, float puckFlow, float pumpFlow,
                                float puckResistance) = 0;
    virtual void sendError(int errorCode) = 0;
    virtual void sendBrewBtnState(bool brewButtonStatus) = 0;
    virtual void sendSteamBtnState(bool steamButtonStatus) = 0;
    virtual void sendAutotuneResult(float Kp, float Ki, float Kd) = 0;
    virtual void sendVolumetricMeasurement(float value) = 0;
    virtual void sendTofMeasurement(int value) = 0;

    // Register callback methods (display -> controller)
    virtual void registerOutputControlCallback(const comm_simple_output_callback_t &callback) = 0;
    virtual void registerAdvancedOutputControlCallback(const comm_advanced_output_callback_t &callback) = 0;
    virtual void registerAltControlCallback(const comm_pin_control_callback_t &callback) = 0;
    virtual void registerPidControlCallback(const comm_pid_control_callback_t &callback) = 0;
    virtual void registerPumpModelCoeffsCallback(const comm_pump_model_coeffs_callback_t &callback) = 0;
    virtual void registerPingCallback(const comm_ping_callback_t &callback) = 0;
    virtual void registerAutotuneCallback(const comm_autotune_callback_t &callback) = 0;
    virtual void registerPressureScaleCallback(const comm_float_callback_t &callback) = 0;
    virtual void registerTareCallback(const comm_void_callback_t &callback) = 0;
    virtual void registerLedControlCallback(const comm_led_control_callback_t &callback) = 0;
};

/**
 * @brief Abstract interface for communication client (runs on display board)
 *
 * This interface abstracts the communication layer used by the GaggiMate display
 * to send commands to and receive data from the controller board.
 */
class ICommClient {
  public:
    virtual ~ICommClient() = default;

    /**
     * @brief Initialize the communication client
     */
    virtual void init() = 0;

    /**
     * @brief Process incoming messages and handle communication tasks
     * Should be called regularly from the main loop
     */
    virtual void loop() = 0;

    /**
     * @brief Attempt to connect to the server
     * @return true if connection was successful
     */
    virtual bool connect() = 0;

    /**
     * @brief Check if ready to attempt connection
     * @return true if ready for connection attempt
     */
    virtual bool isReadyForConnection() = 0;

    /**
     * @brief Check if currently connected to server
     * @return true if connected
     */
    virtual bool isConnected() = 0;

    /**
     * @brief Read system info from the controller
     * @return JSON string containing system information
     */
    virtual String readInfo() = 0;

    // Send methods (display -> controller)
    virtual void sendOutputControl(bool valve, float pumpSetpoint, float boilerSetpoint) = 0;
    virtual void sendAdvancedOutputControl(bool valve, float boilerSetpoint, bool pressureTarget, float pressure,
                                           float flow) = 0;
    virtual void sendAltControl(bool pinState) = 0;
    virtual void sendPing() = 0;
    virtual void sendAutotune(int testTime, int samples) = 0;
    virtual void sendPidSettings(const String &pid) = 0;
    virtual void sendPumpModelCoeffs(const String &pumpModelCoeffs) = 0;
    virtual void setPressureScale(float scale) = 0;
    virtual void sendLedControl(uint8_t channel, uint8_t brightness) = 0;
    virtual void tare() = 0;

    // Register callback methods (controller -> display)
    virtual void registerRemoteErrorCallback(const comm_remote_err_callback_t &callback) = 0;
    virtual void registerBrewBtnCallback(const comm_brew_callback_t &callback) = 0;
    virtual void registerSteamBtnCallback(const comm_steam_callback_t &callback) = 0;
    virtual void registerSensorCallback(const comm_sensor_read_callback_t &callback) = 0;
    virtual void registerAutotuneResultCallback(const comm_pid_control_callback_t &callback) = 0;
    virtual void registerVolumetricMeasurementCallback(const comm_float_callback_t &callback) = 0;
    virtual void registerTofMeasurementCallback(const comm_int_callback_t &callback) = 0;
};

#endif // COMMINTERFACE_H
