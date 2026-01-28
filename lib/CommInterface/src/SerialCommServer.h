#ifndef SERIALCOMMSERVER_H
#define SERIALCOMMSERVER_H

#include "CommInterface.h"
#include "SerialProtocol.h"

/**
 * @brief Serial UART implementation of ICommServer
 *
 * Implements the server side of serial communication for the STM32 controller.
 * Uses the serial protocol defined in SerialProtocol.h.
 */
class SerialCommServer : public ICommServer {
  public:
    /**
     * @brief Construct a new Serial Comm Server
     * @param serial Reference to HardwareSerial instance
     * @param baudRate Baud rate for serial communication
     */
    SerialCommServer(HardwareSerial &serial, uint32_t baudRate = SerialProtocol::DEFAULT_BAUD_RATE);
    ~SerialCommServer() override = default;

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

  private:
    void sendFrame(char type, const String &payload);
    void processMessage(char type, const String &payload);

    HardwareSerial &_serial;
    uint32_t _baudRate;
    SerialProtocol::FrameParser _parser;
    String _infoString;

    // Connection tracking
    bool _connected = false;
    unsigned long _lastMessageTime = 0;
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 5000;

    // Callbacks
    comm_simple_output_callback_t _outputControlCallback = nullptr;
    comm_advanced_output_callback_t _advancedControlCallback = nullptr;
    comm_pin_control_callback_t _altControlCallback = nullptr;
    comm_pid_control_callback_t _pidControlCallback = nullptr;
    comm_pump_model_coeffs_callback_t _pumpModelCoeffsCallback = nullptr;
    comm_ping_callback_t _pingCallback = nullptr;
    comm_autotune_callback_t _autotuneCallback = nullptr;
    comm_float_callback_t _pressureScaleCallback = nullptr;
    comm_void_callback_t _tareCallback = nullptr;
    comm_led_control_callback_t _ledControlCallback = nullptr;

    char _txBuffer[SerialProtocol::MAX_FRAME_SIZE];

    const char *LOG_TAG = "SerialCommServer";
};

#endif // SERIALCOMMSERVER_H
