#include "LedControlPlugin.h"
#include <display/core/Controller.h>
#include <display/core/Event.h>

void LedControlPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    this->controller = controller;
    pluginManager->on("controller:ready", [this](Event const) { initialized = true; });
}

void LedControlPlugin::loop() {
    if (!initialized) {
        return;
    }
    if (lastUpdate + UPDATE_INTERVAL < millis()) {
        lastUpdate = millis();
        updateControl();
    }
}

void LedControlPlugin::updateControl() {
    Settings settings = this->controller->getSettings();
    int mode = this->controller->getMode();
    if (mode == MODE_STANDBY) {
        sendControl(0, 0, 0, 0, 0);
        return;
    }
    if (this->controller->isActive() && mode == MODE_BREW) {
        sendControl(0, 0, 255, 20, settings.getSunriseExtBrightness());
        return;
    }
    if (this->controller->getLastProcess() != nullptr && this->controller->getLastProcess()->getType() == MODE_BREW &&
        mode == MODE_BREW) {
        sendControl(0, 255, 0, 20, settings.getSunriseExtBrightness());
        return;
    }
    if (this->controller->isLowWaterLevel()) {
        sendControl(255, 0, 0, 20, settings.getSunriseExtBrightness());
        return;
    }
    sendControl(settings.getSunriseR(), settings.getSunriseG(), settings.getSunriseB(), settings.getSunriseW(),
                settings.getSunriseExtBrightness());
}

void LedControlPlugin::sendControl(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t ext) {
    ICommClient *commClient = this->controller->getCommClient();
    if (commClient == nullptr) {
        return;
    }

    if (r != last_r)
        commClient->sendLedControl(0, r);
    if (g != last_g)
        commClient->sendLedControl(1, g);
    if (b != last_b)
        commClient->sendLedControl(2, b);
    if (w != last_w)
        commClient->sendLedControl(3, w);
    if (ext != last_ext) {
        commClient->sendLedControl(4, 255 - ext);
        commClient->sendLedControl(5, 255 - ext);
        commClient->sendLedControl(6, 255 - ext);
        commClient->sendLedControl(7, 255 - ext);
    }
    last_r = r;
    last_g = g;
    last_b = b;
    last_w = w;
    last_ext = ext;
}
