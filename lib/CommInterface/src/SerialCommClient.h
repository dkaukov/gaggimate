#ifndef SERIALCOMMCLIENT_H
#define SERIALCOMMCLIENT_H

#include "CommInterface.h"
#include "SerialProtocol.h"

/**
 * @brief Serial UART implementation of ICommClient
 *
 * Implements the client side of serial communication for the ESP32 display.
 * Uses the serial protocol defined in SerialProtocol.h.
 */
class SerialCommClient : public ICommClient {
  public:
    /**
     * @brief Construct a new Serial Comm Client
     * @param serial Reference to HardwareSerial instance
     * @param baudRate Baud rate for serial communication
     * @param rxPin RX GPIO pin (-1 to use default)
     * @param txPin TX GPIO pin (-1 to use default)
     */
    SerialCommClient(HardwareSerial &serial, uint32_t baudRate = SerialProtocol::DEFAULT_BAUD_RATE, int8_t rxPin = -1,
                     int8_t txPin = -1);
    ~SerialCommClient() override = default;

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

  private:
    void sendFrame(char type, const String &payload);
    void processMessage(char type, const String &payload);

    HardwareSerial &_serial;
    uint32_t _baudRate;
    int8_t _rxPin;
    int8_t _txPin;
    SerialProtocol::FrameParser _parser;

    // Connection state
    bool _initialized = false;
    bool _connected = false;
    unsigned long _lastMessageTime = 0;
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 3000;
    static constexpr unsigned long INFO_REQUEST_INTERVAL_MS = 1000;
    unsigned long _lastInfoRequest = 0;
    String _systemInfo = "";

    // Callbacks
    comm_remote_err_callback_t _remoteErrorCallback = nullptr;
    comm_brew_callback_t _brewBtnCallback = nullptr;
    comm_steam_callback_t _steamBtnCallback = nullptr;
    comm_sensor_read_callback_t _sensorCallback = nullptr;
    comm_pid_control_callback_t _autotuneResultCallback = nullptr;
    comm_float_callback_t _volumetricMeasurementCallback = nullptr;
    comm_int_callback_t _tofMeasurementCallback = nullptr;

    char _txBuffer[SerialProtocol::MAX_FRAME_SIZE];

    const char *LOG_TAG = "SerialCommClient";
};

#endif // SERIALCOMMCLIENT_H
