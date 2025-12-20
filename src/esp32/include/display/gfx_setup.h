/**
 * Arduino_GFX Setup for VIEWESMART UEDX48480021-MD80E
 * 
 * Uses the EXACT init sequence from the working ESP-IDF BSP:
 * UEDX48480021-MD80ESP32_2.1inch-Knob/examples/ESP-IDF/UEDX48480021-MD80E-SDK/components/bsp/sub_board/bsp_lcd.c
 * 
 * Controller: ST7701S
 * Interface: 3-wire SPI for init + 16-bit RGB parallel for data
 * Pixel Clock: 26 MHz (critical!)
 */

#ifndef GFX_SETUP_H
#define GFX_SETUP_H

#include <Arduino_GFX_Library.h>
#include "display_config.h"

// =============================================================================
// ST7701S Init Sequence - EXACT copy from working bsp_lcd.c
// This is the init sequence that works with washer.bin!
// =============================================================================

static const uint8_t st7701s_init_operations[] = {
    BEGIN_WRITE,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x13}, 5}
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    
    // {0xEF, {0x08}, 1}
    WRITE_C8_D8, 0xEF, 0x08,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x10}, 5}
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    
    // {0xC0, {0x3B,0x00}, 2}
    WRITE_C8_D16, 0xC0, 0x3B, 0x00,
    
    // {0xC1, {0x0B,0x02}, 2}
    WRITE_C8_D16, 0xC1, 0x0B, 0x02,
    
    // {0xC2, {0x07,0x02}, 2}
    WRITE_C8_D16, 0xC2, 0x07, 0x02,
    
    // {0xC7, {0x00}, 1}
    WRITE_C8_D8, 0xC7, 0x00,
    
    // {0xCC, {0x10}, 1}
    WRITE_C8_D8, 0xCC, 0x10,
    
    // {0xCD, {0x08}, 1}
    WRITE_C8_D8, 0xCD, 0x08,
    
    // {0xB0, 16 bytes} - Positive Gamma
    WRITE_COMMAND_8, 0xB0,
    WRITE_BYTES, 16,
    0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09,
    0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31, 0x18,
    
    // {0xB1, 16 bytes} - Negative Gamma
    WRITE_COMMAND_8, 0xB1,
    WRITE_BYTES, 16,
    0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09,
    0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31, 0x18,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x11}, 5} - Page 1
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
    
    WRITE_C8_D8, 0xB0, 0x6D,
    WRITE_C8_D8, 0xB1, 0x37,
    WRITE_C8_D8, 0xB2, 0x8B,
    WRITE_C8_D8, 0xB3, 0x80,
    WRITE_C8_D8, 0xB5, 0x43,
    WRITE_C8_D8, 0xB7, 0x85,
    WRITE_C8_D8, 0xB8, 0x20,
    WRITE_C8_D8, 0xC0, 0x09,
    WRITE_C8_D8, 0xC1, 0x78,
    WRITE_C8_D8, 0xC2, 0x78,
    WRITE_C8_D8, 0xD0, 0x88,
    
    // {0xE0, {0x00,0x00,0x02}, 3}
    WRITE_COMMAND_8, 0xE0,
    WRITE_BYTES, 3, 0x00, 0x00, 0x02,
    
    // {0xE1, 11 bytes}
    WRITE_COMMAND_8, 0xE1,
    WRITE_BYTES, 11,
    0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0, 0x00, 0x00,
    0x00, 0x20, 0x20,
    
    // {0xE2, 13 bytes}
    WRITE_COMMAND_8, 0xE2,
    WRITE_BYTES, 13,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    
    // {0xE3, {0x00,0x00,0x11,0x00}, 4}
    WRITE_COMMAND_8, 0xE3,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x00,
    
    // {0xE4, {0x22,0x00}, 2}
    WRITE_C8_D16, 0xE4, 0x22, 0x00,
    
    // {0xE5, 16 bytes}
    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 16,
    0x05, 0xEC, 0xF6, 0xCA, 0x07, 0xEE, 0xF6, 0xCA,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // {0xE6, {0x00,0x00,0x11,0x00}, 4}
    WRITE_COMMAND_8, 0xE6,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x00,
    
    // {0xE7, {0x22,0x00}, 2}
    WRITE_C8_D16, 0xE7, 0x22, 0x00,
    
    // {0xE8, 16 bytes}
    WRITE_COMMAND_8, 0xE8,
    WRITE_BYTES, 16,
    0x06, 0xED, 0xF6, 0xCA, 0x08, 0xEF, 0xF6, 0xCA,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // {0xE9, {0x36,0x00}, 2}
    WRITE_C8_D16, 0xE9, 0x36, 0x00,
    
    // {0xEB, 7 bytes}
    WRITE_COMMAND_8, 0xEB,
    WRITE_BYTES, 7,
    0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00,
    
    // {0xED, 16 bytes}
    WRITE_COMMAND_8, 0xED,
    WRITE_BYTES, 16,
    0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xFF, 0x45, 0xFF,
    0xFF, 0x54, 0xFF, 0xA0, 0xAB, 0xFF, 0xFF, 0xFF,
    
    // {0xEF, {0x08,0x08,0x08,0x45,0x3F,0x54}, 6}
    WRITE_COMMAND_8, 0xEF,
    WRITE_BYTES, 6,
    0x08, 0x08, 0x08, 0x45, 0x3F, 0x54,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x13}, 5}
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    
    // {0xE8, {0x00,0x0E}, 2}
    WRITE_C8_D16, 0xE8, 0x00, 0x0E,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x00}, 5} - Page 0
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    
    // {0x11, {0X00}, 1} - Sleep Out
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    
    DELAY, 120,  // Wait 120ms after sleep out
    
    BEGIN_WRITE,
    // {0xFF, {0x77,0x01,0x00,0x00,0x13}, 5}
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    
    // {0xE8, {0x00,0x0C}, 2}
    WRITE_C8_D16, 0xE8, 0x00, 0x0C,
    
    // {0xE8, {0x00,0x00}, 2}
    WRITE_C8_D16, 0xE8, 0x00, 0x00,
    
    // {0xFF, {0x77,0x01,0x00,0x00,0x00}, 5} - Page 0
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    
    // {0x36, {0x00}, 1} - Memory Access Control
    WRITE_C8_D8, 0x36, 0x00,
    
    // {0x3A, {0x66}, 1} - Pixel Format: RGB666 (0x66)
    // This is critical! Must be 0x66 for the display to work properly
    WRITE_C8_D8, 0x3A, 0x66,
    
    // {0x29} - Display ON (sent separately in BSP after 20ms delay)
    WRITE_COMMAND_8, 0x29,
    END_WRITE,
};

// =============================================================================
// Arduino_GFX Display Objects
// =============================================================================

// 3-wire SPI bus for display initialization
// Note: GPIO12 and GPIO13 are shared with RGB DATA2/DATA3
// They will be reconfigured as RGB pins after init
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC - not used in 3-wire SPI */,
    DISPLAY_SPI_CS_PIN /* CS = GPIO18 */,
    DISPLAY_SPI_SCK_PIN /* SCK = GPIO13 - shared with RGB DATA3 */,
    DISPLAY_SPI_MOSI_PIN /* MOSI = GPIO12 - shared with RGB DATA2 */,
    GFX_NOT_DEFINED /* MISO */);

// RGB panel with VIEWESMART pinout and timing
// Pin mapping from esp-bsp.h:
// - DATA0-4 (Blue): GPIO 10,11,12,13,14
// - DATA5-10 (Green): GPIO 21,47,48,45,38,39
// - DATA11-15 (Red): GPIO 40,41,42,2,1
//
// Arduino_ESP32RGBPanel constructor expects: R(5), G(6), B(5) pins
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    DISPLAY_DE_PIN /* DE = GPIO17 */,
    DISPLAY_VSYNC_PIN /* VSYNC = GPIO3 */,
    DISPLAY_HSYNC_PIN /* HSYNC = GPIO46 */,
    DISPLAY_PCLK_PIN /* PCLK = GPIO9 */,
    // RED channel (5 bits): DATA11-15 = GPIO 40,41,42,2,1
    40, 41, 42, 2, 1,
    // GREEN channel (6 bits): DATA5-10 = GPIO 21,47,48,45,38,39
    21, 47, 48, 45, 38, 39,
    // BLUE channel (5 bits): DATA0-4 = GPIO 10,11,12,13,14
    10, 11, 12, 13, 14,
    // Timing parameters from BSP (esp-bsp.h)
    0 /* hsync_polarity */,
    40 /* hsync_front_porch (BSP_LCD_HSYNC_FRONT_PORCH) */,
    8 /* hsync_pulse_width (BSP_LCD_HSYNC_PULSE_WIDTH) */,
    20 /* hsync_back_porch (BSP_LCD_HSYNC_BACK_PORCH) */,
    0 /* vsync_polarity */,
    50 /* vsync_front_porch (BSP_LCD_VSYNC_FRONT_PORCH) */,
    8 /* vsync_pulse_width (BSP_LCD_VSYNC_PULSE_WIDTH) */,
    20 /* vsync_back_porch (BSP_LCD_VSYNC_BACK_PORCH) */,
    0 /* pclk_active_neg (BSP_LCD_PCLK_ACTIVE_NEG = false) */,
    26000000 /* prefer_speed = 26MHz (BSP_LCD_PIXEL_CLOCK_HZ) - CRITICAL! */);

// Main display object with ST7701S init sequence
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    DISPLAY_WIDTH /* 480 */,
    DISPLAY_HEIGHT /* 480 */,
    rgbpanel,
    DISPLAY_ROTATION /* 0 */,
    true /* auto_flush */,
    bus,
    DISPLAY_RST_PIN /* RST = GPIO8 */,
    st7701s_init_operations,
    sizeof(st7701s_init_operations));

#endif // GFX_SETUP_H
