#ifndef GAGGIMATECONTROLLER_H
#define GAGGIMATECONTROLLER_H
#include "ControllerConfig.h"
#include <CommInterface.h>
#include <peripherals/DigitalInput.h>
#include <peripherals/DistanceSensor.h>
#include <peripherals/Heater.h>
#include <peripherals/LedController.h>
#include <peripherals/PressureSensor.h>
#include <peripherals/Pump.h>
#include <peripherals/SimpleRelay.h>
#include <peripherals/TemperatureSensor.h>
#include <vector>

constexpr double PING_TIMEOUT_SECONDS = 20.0;

#ifdef ESP32
// ESP32 board detection pins
constexpr int DETECT_EN_PIN = 40;
constexpr int DETECT_VALUE_PIN = 11;
#else
// STM32 doesn't use auto-detection (config passed at startup)
constexpr int DETECT_EN_PIN = 0;
constexpr int DETECT_VALUE_PIN = 0;
#endif

class GaggiMateController {
  public:
    GaggiMateController(String version);
    ~GaggiMateController();

    void setup(void);
    void loop(void);

    void registerBoardConfig(ControllerConfig config);

    /**
     * @brief Set a custom communication server
     *
     * For STM32, this allows setting up SerialCommServer before setup() is called.
     * For ESP32, BLECommServer is created automatically in setup().
     *
     * @param comm Pointer to ICommServer implementation (ownership transferred)
     */
    void setCommServer(ICommServer *comm);

  private:
    void setupCommunication();
    void detectBoard();
    void detectAddon();
    void handlePing();
    void handlePingTimeout(void);
    void thermalRunawayShutdown(void);
    void startPidAutotune(void);
    void stopPidAutotune(void);
    void sendSensorData(void);
    void registerCommCallbacks();

    ControllerConfig _config = ControllerConfig{};
    ICommServer *_comm = nullptr;
    bool _ownComm = false; // Whether we own the comm server and should delete it

    TemperatureSensor *thermocouple = nullptr;
    Heater *heater = nullptr;
    SimpleRelay *valve = nullptr;
    SimpleRelay *alt = nullptr;
    Pump *pump = nullptr;
    DigitalInput *brewBtn = nullptr;
    DigitalInput *steamBtn = nullptr;
    PressureSensor *pressureSensor = nullptr;
    LedController *ledController = nullptr;
    DistanceSensor *distanceSensor = nullptr;

    std::vector<ControllerConfig> configs;

    String _version;
    unsigned long lastPingTime = 0;
    size_t errorState = COMM_ERROR_CODE_NONE;

    const char *LOG_TAG = "GaggiMateController";
};

#endif // GAGGIMATECONTROLLER_H
