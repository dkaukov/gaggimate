/**
 * @file main.cpp
 * @brief STM32 GaggiMate Controller Entry Point
 *
 * This is the main entry point for the STM32-based GaggiMate controller.
 * It uses serial UART communication instead of BLE to communicate with
 * the display board.
 */

#include "main.h"
#include "ControllerConfigSTM32.h"
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

    // Register STM32-specific board configurations
    controller.registerBoardConfig(GM_STM32_PRO);
    controller.registerBoardConfig(GM_STM32_STANDARD);

    // Initialize the controller
    controller.setup();

    LOG_I("Main", "Setup complete");
}

void loop() { controller.loop(); }
