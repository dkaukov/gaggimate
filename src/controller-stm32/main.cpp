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

#if defined(STM32_USB_BENCH_MONITOR) && defined(USBCON) && defined(USBD_USE_CDC)
namespace {
String usbCommandBuffer;

void printBenchHelp() {
    SerialUSB.println("STM32 USB bench monitor commands:");
    SerialUSB.println("  help          Show this help");
    SerialUSB.println("  status        Print pump/temp/pressure status");
    SerialUSB.println("  pump <0-100>  Override pump output percentage for 5s");
    SerialUSB.println("  off           Disable bench override and stop pump");
}

void handleBenchCommand(String command) {
    command.trim();
    if (command.length() == 0) {
        return;
    }

    if (command.equalsIgnoreCase("help")) {
        printBenchHelp();
        return;
    }

    if (command.equalsIgnoreCase("status")) {
        SerialUSB.println(controller.getBenchDebugStatus());
        return;
    }

    if (command.equalsIgnoreCase("off")) {
        controller.benchDisableOverride();
        SerialUSB.println("bench override disabled");
        return;
    }

    if (command.startsWith("pump ")) {
        float pumpPercent = command.substring(5).toFloat();
        if (pumpPercent < 0.0f || pumpPercent > 100.0f) {
            SerialUSB.println("pump value must be between 0 and 100");
            return;
        }
        controller.benchSetPumpPower(pumpPercent);
        SerialUSB.print("bench pump=");
        SerialUSB.print(pumpPercent, 1);
        SerialUSB.println(" for 5s");
        return;
    }

    SerialUSB.println("unknown command, use 'help'");
}

void handleBenchSerial() {
    while (SerialUSB.available()) {
        char c = static_cast<char>(SerialUSB.read());
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            handleBenchCommand(usbCommandBuffer);
            usbCommandBuffer = "";
            continue;
        }
        if (usbCommandBuffer.length() < 64) {
            usbCommandBuffer += c;
        }
    }
}
} // namespace
#endif

void setup() {
#if defined(LED_BUILTIN)
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

#if defined(USBCON) && defined(USBD_USE_CDC)
    SerialUSB.begin(115200);
    while (!SerialUSB && millis() < 30000) {
        // Give the USB stack time to finish CDC enumeration instead of busy-spinning.
        static bool ledState = false;
#if defined(LED_BUILTIN)
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
#endif
        delay(10);
    }
#if defined(LED_BUILTIN)
    digitalWrite(LED_BUILTIN, LOW);
#endif
#endif

    LOG_I("Main", "GaggiMate STM32 Controller starting...");
    LOG_I("Main", "Version: %s", BUILD_GIT_VERSION);

    // Register Gaggiuino Lego V3 board configuration (MAX6675 thermocouple)
    controller.registerBoardConfig(GM_GAGGIUINO_LEGO_V3);
    LOG_I("Main", "Using Gaggiuino Lego V3 configuration (MAX6675)");

    // Initialize the controller
    controller.setup();

    LOG_I("Main", "Setup complete");
#if defined(STM32_USB_BENCH_MONITOR) && defined(USBCON) && defined(USBD_USE_CDC)
    SerialUSB.println("STM32 USB bench monitor enabled. Type 'help' for commands.");
#endif
}

void loop() {
    controller.loop();
#if defined(STM32_USB_BENCH_MONITOR) && defined(USBCON) && defined(USBD_USE_CDC)
    handleBenchSerial();
#endif
}
