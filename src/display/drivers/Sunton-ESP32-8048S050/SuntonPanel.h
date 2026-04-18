/**
 * @file      SuntonPanel.h
 * @brief     Display panel driver for Sunton/Elecrow ESP32-S3 5" 800x480 RGB LCD
 */
#pragma once

#include <Arduino.h>

#ifndef BOARD_HAS_PSRAM
#error "Please turn on PSRAM to OPI !"
#endif

#include <SD_MMC.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>

#include <display/drivers/common/Display.h>
#include <display/drivers/common/ext.h>
#include "utilities.h"

enum SuntonPanel_TouchType {
    SUNTON_TOUCH_UNKNOWN,
    SUNTON_TOUCH_GT911,
};

enum SuntonPanel_Color_Order {
    SUNTON_ORDER_RGB,
    SUNTON_ORDER_BGR,
};

class SuntonPanel : public Display {
  public:
    SuntonPanel();

    ~SuntonPanel() override;

    bool begin(SuntonPanel_Color_Order order = SUNTON_ORDER_BGR);

    bool installSD();

    void uninstallSD();

    void setBrightness(uint8_t level);

    uint8_t getBrightness() const;

    const char *getTouchModelName() const;

    void sleep();

    void wakeup();

    uint16_t width() override;

    uint16_t height() override;

    uint8_t getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point = 1) override;

    bool isPressed() const;

    void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t hight, uint16_t *data) override;

    bool supportsDirectMode() override { return false; }
    size_t getPreferredDrawBufferSize() override { return static_cast<size_t>(SUNTON_BOARD_TFT_WIDTH) * 20 * sizeof(uint16_t); }
    bool preferInternalDrawBuffer() override { return true; }
    bool preferDoubleDrawBuffer() override { return false; }

  private:
    void initBUS();

    bool initTouch();

    uint8_t _brightness;

    esp_lcd_panel_handle_t _panelDrv;

    TouchDrvInterface *_touchDrv;

    SuntonPanel_Color_Order _order;

    bool _has_init;

    SuntonPanel_TouchType _touchType;
};
