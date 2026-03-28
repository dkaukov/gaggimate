#include "GaggiMateController.h"
#include "utilities.h"
#include <Arduino.h>
#include <peripherals/SimplePump.h>
#include <platform/Logger.h>
#include <platform/System.h>
#include <platform/Threading.h>

#include <utility>

// Include platform-specific implementations
#ifdef ESP32
#include <BLECommServer.h>
#include <peripherals/DimmedPump.h>
#include <peripherals/Max31855Thermocouple.h>
#else
// STM32 with Gaggiuino Lego V3 uses MAX6675 thermocouple
#include <SerialCommServer.h>
#include <peripherals/STM32DimmedPump.h>
#include <peripherals/Max6675Thermocouple.h>
#endif

GaggiMateController::GaggiMateController(String version) : _version(std::move(version)) {
#ifdef ESP32
    // Register default ESP32 board configurations
    configs.push_back(GM_STANDARD_REV_1X);
    configs.push_back(GM_STANDARD_REV_2X);
    configs.push_back(GM_PRO_REV_1x);
    configs.push_back(GM_PRO_LEGO);
    configs.push_back(GM_PRO_REV_11);
#endif
    // STM32 configs are registered via registerBoardConfig() from main.cpp
}

GaggiMateController::~GaggiMateController() {
    if (_ownComm && _comm) {
        delete _comm;
        _comm = nullptr;
    }
}

void GaggiMateController::setCommServer(ICommServer *comm) {
    if (_ownComm && _comm) {
        delete _comm;
    }
    _comm = comm;
    _ownComm = false; // Caller retains ownership
}

void GaggiMateController::setupCommunication() {
    if (_comm == nullptr) {
#ifdef ESP32
        // Create BLE communication server for ESP32
        _comm = new BLECommServer();
        _ownComm = true;
#else
        // Create Serial communication server for STM32
        // Using Serial1 (USART1) for communication with display
        _comm = new SerialCommServer(Serial1);
        _ownComm = true;
#endif
    }
}

void GaggiMateController::setup() {
#ifdef ESP32
    delay(5000);
#else
    // Keep STM32 startup responsive; this port does not need the long BLE-era boot pause.
    delay(250);
#endif

    // Setup communication first (may be needed for board detection on some platforms)
    setupCommunication();

    detectBoard();
    detectAddon();

    // Create thermocouple driver based on platform
#ifdef ESP32
    this->thermocouple = new Max31855Thermocouple(
        _config.maxCsPin, _config.maxMisoPin, _config.maxSckPin, [this](float temperature) { /* noop */ },
        [this]() { thermalRunawayShutdown(); });
#else
    // STM32 with Gaggiuino Lego V3 uses MAX6675 thermocouple
    this->thermocouple = new Max6675Thermocouple(
        _config.maxCsPin, _config.maxMisoPin, _config.maxSckPin, [this](float temperature) { /* noop */ },
        [this]() { thermalRunawayShutdown(); });
#endif
    this->heater = new Heater(
        this->thermocouple, _config.heaterPin, [this]() { thermalRunawayShutdown(); },
        [this](float Kp, float Ki, float Kd) { _comm->sendAutotuneResult(Kp, Ki, Kd); });
    this->valve = new SimpleRelay(_config.valvePin, _config.valveOn);
    this->alt = new SimpleRelay(_config.altPin, _config.altOn);

    if (_config.capabilites.pressure) {
        pressureSensor = new PressureSensor(_config.pressureSda, _config.pressureScl, [this](float pressure) { /* noop */ });
        if (!pressureSensor->setup()) {
            LOG_E(LOG_TAG, "Pressure sensor initialization failed; disabling pressure capability for this boot");
            _config.capabilites.pressure = false;
        }
    }

    // Create appropriate pump implementation based on platform and capabilities
    if (_config.capabilites.dimming) {
#ifdef ESP32
        pump = new DimmedPump(_config.pumpPin, _config.pumpSensePin, pressureSensor);
#else
        pump = new STM32DimmedPump(_config.pumpPin, _config.pumpSensePin, pressureSensor);
#endif
    } else {
        pump = new SimplePump(_config.pumpPin, _config.pumpOn, _config.capabilites.ssrPump ? 1000.0f : 5000.0f);
    }

    this->brewBtn = new DigitalInput(_config.brewButtonPin, [this](const bool state) { _comm->sendBrewBtnState(state); });
    this->steamBtn = new DigitalInput(_config.steamButtonPin, [this](const bool state) { _comm->sendSteamBtnState(state); });

    // 4-Pin peripheral port (I2C for LED controller and ToF sensor)
    bool addonBusAvailable = false;
#ifdef ESP32
    if (!Wire.begin(_config.sunriseSdaPin, _config.sunriseSclPin, 400000)) {
        LOG_E(LOG_TAG, "Failed to initialize I2C bus");
    } else {
        addonBusAvailable = true;
    }
#else
    // STM32 addon probing is only valid when the board config defines addon I2C pins.
    if (_config.sunriseSdaPin != 0 && _config.sunriseSclPin != 0) {
        Wire.setSDA(_config.sunriseSdaPin);
        Wire.setSCL(_config.sunriseSclPin);
        Wire.begin();
        addonBusAvailable = true;
    } else {
        LOG_I(LOG_TAG, "Skipping addon I2C initialization: no STM32 addon bus pins configured");
    }
#endif

    if (addonBusAvailable) {
        this->ledController = new LedController(&Wire);
        this->distanceSensor = new DistanceSensor(&Wire, [this](int distance) { _comm->sendTofMeasurement(distance); });

        if (this->ledController->isAvailable()) {
            _config.capabilites.ledControls = true;
            _config.capabilites.tof = true;
        }
    }

    // Initialize communication with system info
    String systemInfo = make_system_info(_config, _version);
    _comm->init(systemInfo);

    // Register all communication callbacks
    registerCommCallbacks();

    // Setup peripherals
    if (_config.capabilites.ledControls && this->ledController != nullptr) {
        this->ledController->setup();
    }
    if (_config.capabilites.tof && this->distanceSensor != nullptr) {
        this->distanceSensor->setup();
    }

    this->thermocouple->setup();
    this->heater->setup();
    this->valve->setup();
    this->alt->setup();
    this->pump->setup();
    this->brewBtn->setup();
    this->steamBtn->setup();

    if (_config.capabilites.pressure && pressureSensor != nullptr && pressureSensor->isAvailable()) {
        _comm->registerPressureScaleCallback([this](float scale) { this->pressureSensor->setScale(scale); });
    }

    // Set up thermal feedforward for main heater if pressure/dimming capability exists
    if (heater && _config.capabilites.dimming && _config.capabilites.pressure) {
#ifdef ESP32
        auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
        auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
        float *pumpFlowPtr = dimmedPump->getPumpFlowPtr();
        int *valveStatusPtr = dimmedPump->getValveStatusPtr();

        heater->setThermalFeedforward(pumpFlowPtr, 23.0f, valveStatusPtr);
        heater->setFeedforwardScale(0.0f);
    }

    // Initialize last ping time
    lastPingTime = millis();

    LOG_I(LOG_TAG, "Initialization done");
}

void GaggiMateController::registerCommCallbacks() {
    _comm->registerOutputControlCallback([this](bool valve, float pumpSetpoint, float heaterSetpoint) {
        handlePing();
        if (errorState != COMM_ERROR_CODE_NONE) {
            return;
        }
        this->pump->setPower(pumpSetpoint);
        this->valve->set(valve);
        this->heater->setSetpoint(heaterSetpoint);
        if (!_config.capabilites.dimming) {
            return;
        }
#ifdef ESP32
        auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
        auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
        dimmedPump->setValveState(valve);
    });

    _comm->registerAdvancedOutputControlCallback(
        [this](bool valve, float heaterSetpoint, bool pressureTarget, float pressure, float flow) {
            handlePing();
            if (errorState != COMM_ERROR_CODE_NONE) {
                return;
            }
            this->valve->set(valve);
            this->heater->setSetpoint(heaterSetpoint);
            if (!_config.capabilites.dimming) {
                return;
            }
#ifdef ESP32
            auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
            auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
            if (pressureTarget) {
                dimmedPump->setPressureTarget(pressure, flow);
            } else {
                dimmedPump->setFlowTarget(flow, pressure);
            }
            dimmedPump->setValveState(valve);
        });

    _comm->registerAltControlCallback([this](bool state) { this->alt->set(state); });

    _comm->registerPidControlCallback([this](float Kp, float Ki, float Kd, float Kf) {
        this->heater->setTunings(Kp, Ki, Kd);
        // Apply thermal feedforward parameters if available
        this->heater->setFeedforwardScale(Kf);
    });

    _comm->registerPumpModelCoeffsCallback([this](float a, float b, float c, float d) {
        if (_config.capabilites.dimming) {
#ifdef ESP32
            auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
            auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
            // Check if this is a flow measurement call (a and b are flow measurements, c and d are nan)
            if (isnan(c) && isnan(d)) {
                dimmedPump->setPumpFlowCoeff(a, b); // a = oneBarFlow, b = nineBarFlow
            } else {
                dimmedPump->setPumpFlowPolyCoeffs(a, b, c, d); // a, b, c, d are polynomial coefficients
            }
        }
    });

    _comm->registerPingCallback([this]() { handlePing(); });

    _comm->registerAutotuneCallback([this](int goal, int windowSize) { this->heater->autotune(goal, windowSize); });

    _comm->registerTareCallback([this]() {
        if (!_config.capabilites.dimming) {
            return;
        }
#ifdef ESP32
        auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
        auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
        dimmedPump->tare();
    });

    if (ledController != nullptr) {
        _comm->registerLedControlCallback(
            [this](uint8_t channel, uint8_t brightness) { ledController->setChannel(channel, brightness); });
    }
}

void GaggiMateController::loop() {
    // Process communication (important for serial mode)
    if (_comm) {
        _comm->loop();
    }

    unsigned long now = millis();
    if (lastPingTime < now && (now - lastPingTime) / 1000 > PING_TIMEOUT_SECONDS) {
        handlePingTimeout();
    }
    sendSensorData();
#ifdef ESP32
    delay(250);
#else
    delay(50);
#endif
}

void GaggiMateController::registerBoardConfig(ControllerConfig config) { configs.push_back(config); }

void GaggiMateController::detectBoard() {
#ifdef ESP32
    // ESP32: Use ADC-based board detection
    pinMode(DETECT_EN_PIN, OUTPUT);
    pinMode(DETECT_VALUE_PIN, INPUT_PULLDOWN);
    digitalWrite(DETECT_EN_PIN, HIGH);
    uint16_t millivolts = Platform::readAnalogMillivolts(DETECT_VALUE_PIN);
    digitalWrite(DETECT_EN_PIN, LOW);
    int boardId = round(((float)millivolts) / 100.0f - 0.5f);
    LOG_I(LOG_TAG, "Detected Board ID: %d", boardId);
    for (ControllerConfig config : configs) {
        if (config.autodetectValue == boardId) {
            _config = config;
            LOG_I(LOG_TAG, "Using Board: %s", _config.name.c_str());
            return;
        }
    }
    LOG_W(LOG_TAG, "No compatible board detected.");
    delay(5000);
    Platform::systemRestart();
#else
    // STM32: Use first registered config (board selection done at compile time)
    if (configs.empty()) {
        LOG_E(LOG_TAG, "No board configuration registered!");
        delay(5000);
        Platform::systemRestart();
    }
    _config = configs[0];
    LOG_I(LOG_TAG, "Using Board: %s", _config.name.c_str());
#endif
}

void GaggiMateController::detectAddon() {
    // TODO: Add I2C scanning for extensions
}

void GaggiMateController::handlePing() {
    if (errorState == COMM_ERROR_CODE_TIMEOUT) {
        errorState = COMM_ERROR_CODE_NONE;
    }
    lastPingTime = millis();
    LOG_V(LOG_TAG, "Ping received, system is alive");
}

void GaggiMateController::handlePingTimeout() {
    if (errorState == COMM_ERROR_CODE_TIMEOUT) {
        return;
    }

    LOG_E(LOG_TAG, "Ping timeout detected. Turning off heater and pump for safety.\n");
    // Turn off the heater and pump as a safety measure
    this->heater->setSetpoint(0);
    this->pump->setPower(0);
    this->valve->set(false);
    this->alt->set(false);
    errorState = COMM_ERROR_CODE_TIMEOUT;
}

void GaggiMateController::thermalRunawayShutdown() {
    LOG_E(LOG_TAG, "Thermal runaway detected! Turning off heater and pump!\n");
    // Turn off the heater and pump immediately
    this->heater->setSetpoint(0);
    this->pump->setPower(0);
    this->valve->set(false);
    this->alt->set(false);
    errorState = COMM_ERROR_CODE_RUNAWAY;
    _comm->sendError(COMM_ERROR_CODE_RUNAWAY);
}

void GaggiMateController::sendSensorData() {
    if (_config.capabilites.pressure) {
#ifdef ESP32
        auto dimmedPump = static_cast<DimmedPump *>(pump);
#else
        auto dimmedPump = static_cast<STM32DimmedPump *>(pump);
#endif
        _comm->sendSensorData(this->thermocouple->read(), this->pressureSensor->getPressure(), dimmedPump->getPuckFlow(),
                              dimmedPump->getPumpFlow(), dimmedPump->getPuckResistance());
        if (this->valve->getState()) {
            _comm->sendVolumetricMeasurement(dimmedPump->getCoffeeVolume());
        }
    } else {
        _comm->sendSensorData(this->thermocouple->read(), 0.0f, 0.0f, 0.0f, 0.0f);
    }
}
