#include "SerialCommServer.h"

SerialCommServer::SerialCommServer(HardwareSerial &serial, uint32_t baudRate) : _serial(serial), _baudRate(baudRate) {}

void SerialCommServer::init(const String &infoString) {
    _infoString = infoString;
    _serial.begin(_baudRate);
}

void SerialCommServer::setInfo(const String &infoString) { _infoString = infoString; }

void SerialCommServer::loop() {
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

bool SerialCommServer::isConnected() { return _connected; }

void SerialCommServer::sendFrame(char type, const String &payload) {
    size_t len = SerialProtocol::buildFrame(type, payload, _txBuffer, sizeof(_txBuffer));
    if (len > 0) {
        _serial.write(reinterpret_cast<uint8_t *>(_txBuffer), len);
    }
}

void SerialCommServer::sendSensorData(float temperature, float pressure, float puckFlow, float pumpFlow,
                                      float puckResistance) {
    char payload[64];
    snprintf(payload, sizeof(payload), "%.3f,%.3f,%.3f,%.3f,%.3f", temperature, pressure, puckFlow, pumpFlow,
             puckResistance);
    sendFrame(SerialProtocol::MSG_SENSOR_DATA, payload);
}

void SerialCommServer::sendError(int errorCode) {
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", errorCode);
    sendFrame(SerialProtocol::MSG_ERROR, payload);
}

void SerialCommServer::sendBrewBtnState(bool brewButtonStatus) {
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", brewButtonStatus ? 1 : 0);
    sendFrame(SerialProtocol::MSG_BREW_BTN, payload);
}

void SerialCommServer::sendSteamBtnState(bool steamButtonStatus) {
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", steamButtonStatus ? 1 : 0);
    sendFrame(SerialProtocol::MSG_STEAM_BTN, payload);
}

void SerialCommServer::sendAutotuneResult(float Kp, float Ki, float Kd) {
    char payload[64];
    snprintf(payload, sizeof(payload), "%.3f,%.3f,%.3f,0.0", Kp, Ki, Kd);
    sendFrame(SerialProtocol::MSG_AUTOTUNE_RESULT, payload);
}

void SerialCommServer::sendVolumetricMeasurement(float value) {
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", value);
    sendFrame(SerialProtocol::MSG_VOLUMETRIC, payload);
}

void SerialCommServer::sendTofMeasurement(int value) {
    char payload[16];
    snprintf(payload, sizeof(payload), "%d", value);
    sendFrame(SerialProtocol::MSG_TOF, payload);
}

void SerialCommServer::processMessage(char type, const String &payload) {
    using namespace SerialProtocol;

    switch (type) {
    case MSG_OUTPUT_CONTROL: {
        size_t tokenCount = getToken(payload, 0, ',').length() == 0 ? 0 : countTokens(payload, ',');
        if (tokenCount < 4) {
            break;
        }

        String controlTypeToken = getToken(payload, 0, ',');
        String valveToken = getToken(payload, 1, ',');
        String boilerSetpointToken = getToken(payload, 3, ',');
        if (!isNumericToken(controlTypeToken, false) || !isBoolToken(valveToken) || !isNumericToken(boilerSetpointToken)) {
            break;
        }

        uint8_t controlType = controlTypeToken.toInt();
        uint8_t valve = valveToken.toInt();
        float boilerSetpoint = boilerSetpointToken.toFloat();

        if (controlType == 0) {
            if (tokenCount != 4) {
                break;
            }
            // Simple output control
            String pumpSetpointToken = getToken(payload, 2, ',');
            if (!isNumericToken(pumpSetpointToken)) {
                break;
            }
            float pumpSetpoint = pumpSetpointToken.toFloat();
            if (_outputControlCallback) {
                _outputControlCallback(valve == 1, pumpSetpoint, boilerSetpoint);
            }
        } else if (controlType == 1) {
            if (tokenCount != 7) {
                break;
            }
            // Advanced output control
            String pressureTargetToken = getToken(payload, 4, ',');
            String pumpPressureToken = getToken(payload, 5, ',');
            String pumpFlowToken = getToken(payload, 6, ',');
            if (!isBoolToken(pressureTargetToken) || !isNumericToken(pumpPressureToken) || !isNumericToken(pumpFlowToken)) {
                break;
            }
            bool pressureTarget = pressureTargetToken == "1";
            float pumpPressure = pumpPressureToken.toFloat();
            float pumpFlow = pumpFlowToken.toFloat();
            if (_advancedControlCallback) {
                _advancedControlCallback(valve == 1, boilerSetpoint, pressureTarget, pumpPressure, pumpFlow);
            }
        }
        break;
    }

    case MSG_ALT_CONTROL: {
        if (!isBoolToken(payload)) {
            break;
        }
        bool pinState = payload == "1";
        if (_altControlCallback) {
            _altControlCallback(pinState);
        }
        break;
    }

    case MSG_PING: {
        if (_pingCallback) {
            _pingCallback();
        }
        break;
    }

    case MSG_PID_SETTINGS: {
        size_t tokenCount = countTokens(payload, ',');
        if (tokenCount != 3 && tokenCount != 4) {
            break;
        }
        String kpToken = getToken(payload, 0, ',');
        String kiToken = getToken(payload, 1, ',');
        String kdToken = getToken(payload, 2, ',');
        if (!isNumericToken(kpToken) || !isNumericToken(kiToken) || !isNumericToken(kdToken)) {
            break;
        }
        float Kp = kpToken.toFloat();
        float Ki = kiToken.toFloat();
        float Kd = kdToken.toFloat();
        float Kf = 0.0f;
        String kfToken = getToken(payload, 3, ',');
        if (kfToken.length() > 0) {
            if (!isNumericToken(kfToken)) {
                break;
            }
            Kf = kfToken.toFloat();
        }
        if (_pidControlCallback) {
            _pidControlCallback(Kp, Ki, Kd, Kf);
        }
        break;
    }

    case MSG_PUMP_MODEL: {
        size_t tokenCount = countTokens(payload, ',');
        if (tokenCount != 2 && tokenCount != 4) {
            break;
        }
        String aToken = getToken(payload, 0, ',');
        String bToken = getToken(payload, 1, ',');
        if (!isNumericToken(aToken) || !isNumericToken(bToken)) {
            break;
        }
        float a = aToken.toFloat();
        float b = bToken.toFloat();
        float c = NAN;
        float d = NAN;
        if (tokenCount == 4) {
            String cToken = getToken(payload, 2, ',');
            String dToken = getToken(payload, 3, ',');
            if (!isNumericToken(cToken) || !isNumericToken(dToken)) {
                break;
            }
            c = cToken.toFloat();
            d = dToken.toFloat();
        }
        if (_pumpModelCoeffsCallback) {
            _pumpModelCoeffsCallback(a, b, c, d);
        }
        break;
    }

    case MSG_AUTOTUNE_START: {
        if (countTokens(payload, ',') != 2) {
            break;
        }
        String testTimeToken = getToken(payload, 0, ',');
        String samplesToken = getToken(payload, 1, ',');
        if (!isNumericToken(testTimeToken, false) || !isNumericToken(samplesToken, false)) {
            break;
        }
        int testTime = testTimeToken.toInt();
        int samples = samplesToken.toInt();
        if (_autotuneCallback) {
            _autotuneCallback(testTime, samples);
        }
        break;
    }

    case MSG_PRESSURE_SCALE: {
        if (!isNumericToken(payload)) {
            break;
        }
        float scale = payload.toFloat();
        if (_pressureScaleCallback) {
            _pressureScaleCallback(scale);
        }
        break;
    }

    case MSG_TARE: {
        if (_tareCallback) {
            _tareCallback();
        }
        break;
    }

    case MSG_LED_CONTROL: {
        if (countTokens(payload, ',') != 2) {
            break;
        }
        String channelToken = getToken(payload, 0, ',');
        String brightnessToken = getToken(payload, 1, ',');
        if (!isNumericToken(channelToken, false) || !isNumericToken(brightnessToken, false)) {
            break;
        }
        uint8_t channel = channelToken.toInt();
        uint8_t brightness = brightnessToken.toInt();
        if (_ledControlCallback) {
            _ledControlCallback(channel, brightness);
        }
        break;
    }

    case MSG_REQUEST_INFO: {
        sendFrame(MSG_INFO, _infoString);
        break;
    }

    default:
        // Unknown message type, ignore
        break;
    }
}

void SerialCommServer::registerOutputControlCallback(const comm_simple_output_callback_t &callback) {
    _outputControlCallback = callback;
}

void SerialCommServer::registerAdvancedOutputControlCallback(const comm_advanced_output_callback_t &callback) {
    _advancedControlCallback = callback;
}

void SerialCommServer::registerAltControlCallback(const comm_pin_control_callback_t &callback) {
    _altControlCallback = callback;
}

void SerialCommServer::registerPidControlCallback(const comm_pid_control_callback_t &callback) {
    _pidControlCallback = callback;
}

void SerialCommServer::registerPumpModelCoeffsCallback(const comm_pump_model_coeffs_callback_t &callback) {
    _pumpModelCoeffsCallback = callback;
}

void SerialCommServer::registerPingCallback(const comm_ping_callback_t &callback) { _pingCallback = callback; }

void SerialCommServer::registerAutotuneCallback(const comm_autotune_callback_t &callback) {
    _autotuneCallback = callback;
}

void SerialCommServer::registerPressureScaleCallback(const comm_float_callback_t &callback) {
    _pressureScaleCallback = callback;
}

void SerialCommServer::registerTareCallback(const comm_void_callback_t &callback) { _tareCallback = callback; }

void SerialCommServer::registerLedControlCallback(const comm_led_control_callback_t &callback) {
    _ledControlCallback = callback;
}
