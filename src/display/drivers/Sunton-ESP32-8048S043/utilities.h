/**
 * @file      utilities.h
 * @brief     Pin definitions for Sunton ESP32-8048S043C 4.3" 800x480 RGB LCD
 * @note      ESP32-8048S043C (4.3 inch, 800x480, GT911 capacitive touch)
 */
#pragma once

// Display resolution
#define SUNTON43_BOARD_TFT_WIDTH (800)
#define SUNTON43_BOARD_TFT_HEIGHT (480)

// RGB timing (16MHz for 800x480 @ 60Hz)
#define SUNTON43_RGB_TIMING_FREQ_HZ (16000000UL)

// Backlight PWM
#define SUNTON43_BOARD_TFT_BL (2)
#define SUNTON43_PWM_CHANNEL 0
#define SUNTON43_PWM_FREQ 20000
#define SUNTON43_PWM_RESOLUTION 8
#define SUNTON43_BACKLIGHT_MAX 255

// RGB control signals
#define SUNTON43_BOARD_TFT_HSYNC (39)
#define SUNTON43_BOARD_TFT_VSYNC (40)
#define SUNTON43_BOARD_TFT_DE (41)
#define SUNTON43_BOARD_TFT_PCLK (42)

// RGB565 data pins (directly mapped to GPIO)
// Blue channel (5 bits)
#define SUNTON43_LCD_PIN_NUM_RGB_DATA0 (15)  // B3
#define SUNTON43_LCD_PIN_NUM_RGB_DATA1 (7)   // B4
#define SUNTON43_LCD_PIN_NUM_RGB_DATA2 (6)   // B5
#define SUNTON43_LCD_PIN_NUM_RGB_DATA3 (5)   // B6
#define SUNTON43_LCD_PIN_NUM_RGB_DATA4 (4)   // B7

// Green channel (6 bits)
#define SUNTON43_LCD_PIN_NUM_RGB_DATA5 (9)   // G2
#define SUNTON43_LCD_PIN_NUM_RGB_DATA6 (46)  // G3
#define SUNTON43_LCD_PIN_NUM_RGB_DATA7 (3)   // G4
#define SUNTON43_LCD_PIN_NUM_RGB_DATA8 (8)   // G5
#define SUNTON43_LCD_PIN_NUM_RGB_DATA9 (16)  // G6
#define SUNTON43_LCD_PIN_NUM_RGB_DATA10 (1)  // G7

// Red channel (5 bits)
#define SUNTON43_LCD_PIN_NUM_RGB_DATA11 (14) // R3
#define SUNTON43_LCD_PIN_NUM_RGB_DATA12 (21) // R4
#define SUNTON43_LCD_PIN_NUM_RGB_DATA13 (47) // R5
#define SUNTON43_LCD_PIN_NUM_RGB_DATA14 (48) // R6
#define SUNTON43_LCD_PIN_NUM_RGB_DATA15 (45) // R7

// I2C for touch
#define SUNTON43_BOARD_I2C_SDA (19)
#define SUNTON43_BOARD_I2C_SCL (20)

// Touch interrupt and reset
#define SUNTON43_BOARD_TOUCH_IRQ (18)
#define SUNTON43_BOARD_TOUCH_RST (38)

// SD Card (directly connected SDMMC)
#define SUNTON43_BOARD_SDMMC_CLK (12)
#define SUNTON43_BOARD_SDMMC_CMD (11)
#define SUNTON43_BOARD_SDMMC_D0 (13)
