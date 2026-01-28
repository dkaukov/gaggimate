/**
 * @file      Sunton43Driver.h
 * @brief     Driver wrapper for Sunton ESP32-8048S043C 4.3" 800x480 RGB LCD
 */
#ifndef SUNTON43DRIVER_H
#define SUNTON43DRIVER_H

#include "Driver.h"
#include <display/drivers/Sunton-ESP32-8048S043/Sunton43Panel.h>

class Sunton43Driver : public Driver {
  public:
    bool isCompatible() override;
    void init() override;
    void setBrightness(int brightness) override { panel.setBrightness(brightness); };
    bool supportsSDCard() override;
    bool installSDCard() override;

    static Sunton43Driver *getInstance() {
        if (instance == nullptr) {
            instance = new Sunton43Driver();
        }
        return instance;
    };

  private:
    static Sunton43Driver *instance;
    Sunton43Panel panel;

    Sunton43Driver() {};
};

#endif // SUNTON43DRIVER_H
