/**
 * @file      Sunton43Panel.h
 * @brief     Display panel driver for Sunton ESP32-8048S043C 4.3" 800x480 RGB LCD
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

enum Sunton43Panel_TouchType {
    SUNTON43_TOUCH_UNKNOWN,
    SUNTON43_TOUCH_GT911,
};

enum Sunton43Panel_Color_Order {
    SUNTON43_ORDER_RGB,
    SUNTON43_ORDER_BGR,
};

class Sunton43Panel : public Display {
  public:
    Sunton43Panel();

    ~Sunton43Panel() override;

    bool begin(Sunton43Panel_Color_Order order = SUNTON43_ORDER_BGR);

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

  private:
    void initBUS();

    bool initTouch();

    uint8_t _brightness;

    esp_lcd_panel_handle_t _panelDrv;

    TouchDrvInterface *_touchDrv;

    Sunton43Panel_Color_Order _order;

    bool _has_init;

    Sunton43Panel_TouchType _touchType;
};
