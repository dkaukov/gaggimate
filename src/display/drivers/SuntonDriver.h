/**
 * @file      SuntonDriver.h
 * @brief     Driver wrapper for Sunton/Elecrow ESP32-S3 5" 800x480 RGB LCD
 */
#ifndef SUNTONDRIVER_H
#define SUNTONDRIVER_H

#include "Driver.h"
#include <display/drivers/Sunton-ESP32-8048S050/SuntonPanel.h>

class SuntonDriver : public Driver {
  public:
    bool isCompatible() override;
    void init() override;
    void setBrightness(int brightness) override { panel.setBrightness(brightness); };
    bool supportsSDCard() override;
    bool installSDCard() override;

    static SuntonDriver *getInstance() {
        if (instance == nullptr) {
            instance = new SuntonDriver();
        }
        return instance;
    };

  private:
    static SuntonDriver *instance;
    SuntonPanel panel;

    SuntonDriver() {};
};

#endif // SUNTONDRIVER_H
