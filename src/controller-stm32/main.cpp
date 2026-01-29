/**
 * @file main.cpp
 * @brief STM32 Gaggiuino Lego V3 Controller Entry Point
 *
 * This is the main entry point for the STM32-based GaggiMate controller
 * using Gaggiuino Lego V3 hardware with MAX6675 thermocouple.
 * It uses serial UART communication instead of BLE to communicate with
 * the display board.
 */

#include "main.h"
#include "ControllerConfig.h"
#include "GaggiMateController.h"
#include <platform/Logger.h>

// Create controller instance
GaggiMateController controller(BUILD_GIT_VERSION);

void setup() {
    // Initialize debug serial (USB CDC or different UART)
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // Wait for serial connection (with timeout)
    }

    LOG_I("Main", "GaggiMate STM32 Controller starting...");
    LOG_I("Main", "Version: %s", BUILD_GIT_VERSION);

    // Register Gaggiuino Lego V3 board configuration (MAX6675 thermocouple)
    controller.registerBoardConfig(GM_GAGGIUINO_LEGO_V3);
    LOG_I("Main", "Using Gaggiuino Lego V3 configuration (MAX6675)");

    // Initialize the controller
    controller.setup();

    LOG_I("Main", "Setup complete");
}

void loop() { controller.loop(); }
