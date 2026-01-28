#include "SerialCommClient.h"

SerialCommClient::SerialCommClient(HardwareSerial &serial, uint32_t baudRate, int8_t rxPin, int8_t txPin)
    : _serial(serial), _baudRate(baudRate), _rxPin(rxPin), _txPin(txPin) {}

void SerialCommClient::init() {
#ifdef ESP32
    if (_rxPin >= 0 && _txPin >= 0) {
        _serial.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
    } else {
        _serial.begin(_baudRate);
    }
#else
    _serial.begin(_baudRate);
#endif
    _initialized = true;
    _lastInfoRequest = millis();
}

void SerialCommClient::loop() {
    if (!_initialized) {
        return;
    }

    // Check connection timeout
    if (_connected && (millis() - _lastMessageTime > CONNECTION_TIMEOUT_MS)) {
        _connected = false;
    }

    // Process incoming data
    while (_serial.available()) {
        uint8_t byte = _serial.read();
        SerialProtocol::ParseResult result = _parser.processByte(byte);

        if (result.valid) {
            _lastMessageTime = millis();
            _connected = true;
            processMessage(result.type, result.payload);
        }
    }
}

bool SerialCommClient::connect() {
    // For serial, connection is established when we receive valid data
    // Request system info to initiate communication
    sendFrame(SerialProtocol::MSG_REQUEST_INFO, "");
    _lastInfoRequest = millis();

    // Wait briefly for response
    unsigned long startTime = millis();
    while (millis() - startTime < 500) {
        loop();
        if (_connected && _systemInfo.length() > 0) {
            return true;
        }
        delay(10);
    }
    return _connected;
}

bool SerialCommClient::isReadyForConnection() {
    // Serial is always ready after initialization
    if (!_initialized) {
        return false;
    }

    // Periodically request info if not connected
    if (!_connected && (millis() - _lastInfoRequest > INFO_REQUEST_INTERVAL_MS)) {
        sendFrame(SerialProtocol::MSG_REQUEST_INFO, "");
        _lastInfoRequest = millis();
    }

    return _initialized;
}

bool SerialCommClient::isConnected() { return _connected; }

String SerialCommClient::readInfo() { return _systemInfo; }

void SerialCommClient::sendFrame(char type, const String &payload) {
    size_t len = SerialProtocol::buildFrame(type, payload, _txBuffer, sizeof(_txBuffer));
    if (len > 0) {
        _serial.write(reinterpret_cast<uint8_t *>(_txBuffer), len);
    }
}

void SerialCommClient::sendOutputControl(bool valve, float pumpSetpoint, float boilerSetpoint) {
    char payload[64];
    snprintf(payload, sizeof(payload), "%d,%d,%.1f,%.1f", 0, valve ? 1 : 0, pumpSetpoint, boilerSetpoint);
    sendFrame(SerialProtocol::MSG_OUTPUT_CONTROL, payload);
}

void SerialCommClient::sendAdvancedOutputControl(bool valve, float boilerSetpoint, bool pressureTarget, float pressure,
                                                 float flow) {
    char payload[64];
    snprintf(payload, sizeof(payload), "%d,%d,%.1f,%.1f,%d,%.2f,%.2f", 1, valve ? 1 : 0, 100.0f, boilerSetpoint,
             pressureTarget ? 1 : 0, pressure, flow);
    sendFrame(SerialProtocol::MSG_OUTPUT_CONTROL, payload);
}

void SerialCommClient::sendAltControl(bool pinState) {
    sendFrame(SerialProtocol::MSG_ALT_CONTROL, pinState ? "1" : "0");
}

void SerialCommClient::sendPing() { sendFrame(SerialProtocol::MSG_PING, "1"); }

void SerialCommClient::sendAutotune(int testTime, int samples) {
    char payload[32];
    snprintf(payload, sizeof(payload), "%d,%d", testTime, samples);
    sendFrame(SerialProtocol::MSG_AUTOTUNE_START, payload);
}

void SerialCommClient::sendPidSettings(const String &pid) { sendFrame(SerialProtocol::MSG_PID_SETTINGS, pid); }

void SerialCommClient::sendPumpModelCoeffs(const String &pumpModelCoeffs) {
    sendFrame(SerialProtocol::MSG_PUMP_MODEL, pumpModelCoeffs);
}

void SerialCommClient::setPressureScale(float scale) {
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", scale);
    sendFrame(SerialProtocol::MSG_PRESSURE_SCALE, payload);
}

void SerialCommClient::sendLedControl(uint8_t channel, uint8_t brightness) {
    char payload[16];
    snprintf(payload, sizeof(payload), "%d,%d", channel, brightness);
    sendFrame(SerialProtocol::MSG_LED_CONTROL, payload);
}

void SerialCommClient::tare() { sendFrame(SerialProtocol::MSG_TARE, "1"); }

void SerialCommClient::processMessage(char type, const String &payload) {
    using namespace SerialProtocol;

    switch (type) {
    case MSG_SENSOR_DATA: {
        float temperature = getToken(payload, 0, ',').toFloat();
        float pressure = getToken(payload, 1, ',').toFloat();
        float puckFlow = getToken(payload, 2, ',').toFloat();
        float pumpFlow = getToken(payload, 3, ',').toFloat();
        float puckResistance = getToken(payload, 4, ',').toFloat();
        if (_sensorCallback) {
            _sensorCallback(temperature, pressure, puckFlow, pumpFlow, puckResistance);
        }
        break;
    }

    case MSG_ERROR: {
        int errorCode = payload.toInt();
        if (_remoteErrorCallback) {
            _remoteErrorCallback(errorCode);
        }
        break;
    }

    case MSG_BREW_BTN: {
        bool brewButtonStatus = payload.toInt() == 1;
        if (_brewBtnCallback) {
            _brewBtnCallback(brewButtonStatus);
        }
        break;
    }

    case MSG_STEAM_BTN: {
        bool steamButtonStatus = payload.toInt() == 1;
        if (_steamBtnCallback) {
            _steamBtnCallback(steamButtonStatus);
        }
        break;
    }

    case MSG_INFO: {
        _systemInfo = payload;
        break;
    }

    case MSG_AUTOTUNE_RESULT: {
        float Kp = getToken(payload, 0, ',').toFloat();
        float Ki = getToken(payload, 1, ',').toFloat();
        float Kd = getToken(payload, 2, ',').toFloat();
        float Kf = 0.0f;
        String kfToken = getToken(payload, 3, ',');
        if (kfToken.length() > 0) {
            Kf = kfToken.toFloat();
        }
        if (_autotuneResultCallback) {
            _autotuneResultCallback(Kp, Ki, Kd, Kf);
        }
        break;
    }

    case MSG_VOLUMETRIC: {
        float value = payload.toFloat();
        if (_volumetricMeasurementCallback) {
            _volumetricMeasurementCallback(value);
        }
        break;
    }

    case MSG_TOF: {
        int value = payload.toInt();
        if (_tofMeasurementCallback) {
            _tofMeasurementCallback(value);
        }
        break;
    }

    default:
        // Unknown message type, ignore
        break;
    }
}

void SerialCommClient::registerRemoteErrorCallback(const comm_remote_err_callback_t &callback) {
    _remoteErrorCallback = callback;
}

void SerialCommClient::registerBrewBtnCallback(const comm_brew_callback_t &callback) { _brewBtnCallback = callback; }

void SerialCommClient::registerSteamBtnCallback(const comm_steam_callback_t &callback) { _steamBtnCallback = callback; }

void SerialCommClient::registerSensorCallback(const comm_sensor_read_callback_t &callback) {
    _sensorCallback = callback;
}

void SerialCommClient::registerAutotuneResultCallback(const comm_pid_control_callback_t &callback) {
    _autotuneResultCallback = callback;
}

void SerialCommClient::registerVolumetricMeasurementCallback(const comm_float_callback_t &callback) {
    _volumetricMeasurementCallback = callback;
}

void SerialCommClient::registerTofMeasurementCallback(const comm_int_callback_t &callback) {
    _tofMeasurementCallback = callback;
}
