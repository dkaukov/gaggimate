/**
 * @file      SuntonPanel.cpp
 * @brief     Display panel implementation for Sunton/Elecrow ESP32-S3 5" 800x480 RGB LCD
 */
#include "SuntonPanel.h"
#include "utilities.h"
#include <driver/gpio.h>

SuntonPanel::SuntonPanel()
    : _brightness(0), _panelDrv(nullptr), _touchDrv(nullptr), _order(SUNTON_ORDER_BGR), _has_init(false),
      _touchType(SUNTON_TOUCH_UNKNOWN) {}

SuntonPanel::~SuntonPanel() {
    if (_panelDrv) {
        esp_lcd_panel_del(_panelDrv);
        _panelDrv = nullptr;
    }
    if (_touchDrv) {
        delete _touchDrv;
        _touchDrv = nullptr;
    }
}

bool SuntonPanel::begin(SuntonPanel_Color_Order order) {
    if (_panelDrv) {
        return true;
    }

    _order = order;

    // Setup backlight PWM
    log_i("Initializing backlight PWM...");
    ledcSetup(SUNTON_PWM_CHANNEL, SUNTON_PWM_FREQ, SUNTON_PWM_RESOLUTION);
    ledcAttachPin(SUNTON_BOARD_TFT_BL, SUNTON_PWM_CHANNEL);

    // Initialize touch before display
    if (!initTouch()) {
        Serial.println(F("Touch chip not found."));
        // Continue without touch - display can still work
    }

    initBUS();

    _has_init = true;
    return true;
}

bool SuntonPanel::installSD() {
    SD_MMC.setPins(SUNTON_BOARD_SDMMC_CLK, SUNTON_BOARD_SDMMC_CMD, SUNTON_BOARD_SDMMC_D0);

    if (SD_MMC.begin("/sdcard", true, false)) {
        uint8_t cardType = SD_MMC.cardType();
        if (cardType != CARD_NONE) {
            Serial.print(F("SD Card Type: "));
            if (cardType == CARD_MMC)
                Serial.println(F("MMC"));
            else if (cardType == CARD_SD)
                Serial.println(F("SDSC"));
            else if (cardType == CARD_SDHC)
                Serial.println(F("SDHC"));
            else
                Serial.println(F("UNKNOWN"));
            uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);
        }
        return true;
    }
    return false;
}

void SuntonPanel::uninstallSD() { SD_MMC.end(); }

void SuntonPanel::setBrightness(uint8_t value) {
    value = constrain(value, 0, 16);
    const uint8_t pwmValue = value == 0 ? 0 : map(value, 1, 16, SUNTON_BACKLIGHT_MIN, SUNTON_BACKLIGHT_MAX);
    _brightness = value;
    ledcWrite(SUNTON_PWM_CHANNEL, pwmValue);
}

uint8_t SuntonPanel::getBrightness() const { return _brightness; }

const char *SuntonPanel::getTouchModelName() const {
    if (_touchDrv) {
        return _touchDrv->getModelName();
    }
    return "UNKNOWN";
}

void SuntonPanel::sleep() {
    // Fade out backlight
    for (int i = _brightness; i >= 0; --i) {
        setBrightness(i);
        delay(10);
    }

    if (_panelDrv) {
        esp_lcd_panel_disp_off(_panelDrv, true);
    }

    Wire.end();

    pinMode(SUNTON_BOARD_I2C_SDA, OPEN_DRAIN);
    pinMode(SUNTON_BOARD_I2C_SCL, OPEN_DRAIN);

    // If SD card is initialized, unmount it
    if (SD_MMC.cardSize()) {
        SD_MMC.end();
    }

    // Set touch IRQ as wakeup source
    esp_sleep_enable_ext1_wakeup(_BV(SUNTON_BOARD_TOUCH_IRQ), ESP_EXT1_WAKEUP_ANY_LOW);

    esp_deep_sleep_start();
}

void SuntonPanel::wakeup() {
    // Reinitialize if needed after wakeup
}

uint16_t SuntonPanel::width() { return SUNTON_BOARD_TFT_WIDTH; }

uint16_t SuntonPanel::height() { return SUNTON_BOARD_TFT_HEIGHT; }

uint8_t SuntonPanel::getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point) {
    if (_touchDrv) {
        uint8_t touched = _touchDrv->getPoint(x_array, y_array, get_point);
        return touched;
    }
    return 0;
}

bool SuntonPanel::isPressed() const {
    if (_touchDrv) {
        return _touchDrv->isPressed();
    }
    return false;
}

void SuntonPanel::initBUS() {
    if (_panelDrv) {
        return;
    }

    // RGB panel configuration for 800x480 display
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings =
            {
                .pclk_hz = SUNTON_RGB_TIMING_FREQ_HZ,
                .h_res = SUNTON_BOARD_TFT_WIDTH,
                .v_res = SUNTON_BOARD_TFT_HEIGHT,
                // Timing parameters for 800x480 panel
                .hsync_pulse_width = 4,
                .hsync_back_porch = 8,
                .hsync_front_porch = 8,
                .vsync_pulse_width = 4,
                .vsync_back_porch = 8,
                .vsync_front_porch = 8,
                .flags =
                    {
                        .pclk_active_neg = 1,
                    },
            },
        .data_width = 16, // RGB565 in parallel mode
        .psram_trans_align = 64,
        .hsync_gpio_num = SUNTON_BOARD_TFT_HSYNC,
        .vsync_gpio_num = SUNTON_BOARD_TFT_VSYNC,
        .de_gpio_num = SUNTON_BOARD_TFT_DE,
        .pclk_gpio_num = SUNTON_BOARD_TFT_PCLK,
        .data_gpio_nums =
            {
                SUNTON_LCD_PIN_NUM_RGB_DATA0,
                SUNTON_LCD_PIN_NUM_RGB_DATA1,
                SUNTON_LCD_PIN_NUM_RGB_DATA2,
                SUNTON_LCD_PIN_NUM_RGB_DATA3,
                SUNTON_LCD_PIN_NUM_RGB_DATA4,
                SUNTON_LCD_PIN_NUM_RGB_DATA5,
                SUNTON_LCD_PIN_NUM_RGB_DATA6,
                SUNTON_LCD_PIN_NUM_RGB_DATA7,
                SUNTON_LCD_PIN_NUM_RGB_DATA8,
                SUNTON_LCD_PIN_NUM_RGB_DATA9,
                SUNTON_LCD_PIN_NUM_RGB_DATA10,
                SUNTON_LCD_PIN_NUM_RGB_DATA11,
                SUNTON_LCD_PIN_NUM_RGB_DATA12,
                SUNTON_LCD_PIN_NUM_RGB_DATA13,
                SUNTON_LCD_PIN_NUM_RGB_DATA14,
                SUNTON_LCD_PIN_NUM_RGB_DATA15,
            },
        .disp_gpio_num = GPIO_NUM_NC,
        .on_frame_trans_done = NULL,
        .user_ctx = NULL,
        .flags =
            {
                .fb_in_psram = 1, // Allocate frame buffer in PSRAM
            },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &_panelDrv));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(_panelDrv));
    ESP_ERROR_CHECK(esp_lcd_panel_init(_panelDrv));

    // Apply color order swap if needed
    if (_order == SUNTON_ORDER_BGR) {
        esp_lcd_panel_swap_xy(_panelDrv, false);
        esp_lcd_panel_set_gap(_panelDrv, 0, 0);
        esp_lcd_panel_mirror(_panelDrv, false, false);
    }
}

bool SuntonPanel::initTouch() {
    bool result = false;

    log_i("Initializing touch for Sunton 5\" panel...");

    // GT911 touch controller
    _touchDrv = new TouchDrvGT911();
    _touchDrv->setPins(SUNTON_BOARD_TOUCH_RST, SUNTON_BOARD_TOUCH_IRQ);

    // Try primary I2C address (0x5D)
    result = _touchDrv->begin(Wire, GT911_SLAVE_ADDRESS_L, SUNTON_BOARD_I2C_SDA, SUNTON_BOARD_I2C_SCL);
    if (result) {
        TouchDrvGT911 *gt911 = static_cast<TouchDrvGT911 *>(_touchDrv);
        gt911->setInterruptMode(FALLING);
        _touchType = SUNTON_TOUCH_GT911;
        log_i("Successfully initialized GT911 touch at address 0x5D");
        return true;
    }

    // Try alternate I2C address (0x14)
    result = _touchDrv->begin(Wire, GT911_SLAVE_ADDRESS_H, SUNTON_BOARD_I2C_SDA, SUNTON_BOARD_I2C_SCL);
    if (result) {
        TouchDrvGT911 *gt911 = static_cast<TouchDrvGT911 *>(_touchDrv);
        gt911->setInterruptMode(FALLING);
        _touchType = SUNTON_TOUCH_GT911;
        log_i("Successfully initialized GT911 touch at address 0x14");
        return true;
    }

    delete _touchDrv;
    _touchDrv = nullptr;

    log_e("Unable to find GT911 touch device.");
    return false;
}

void SuntonPanel::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t hight, uint16_t *data) {
    assert(_panelDrv);
    esp_lcd_panel_draw_bitmap(_panelDrv, x, y, width, hight, data);
}
