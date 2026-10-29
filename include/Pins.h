#ifndef PINS_H
#define PINS_H

#include <driver/gpio.h>
#include <driver/i2c.h>

/**
 * @file Pins.h
 * @brief Zentrale Definition aller verwendeten IO-Pins.
 */

// ===================================================================
// IO-Expander, Display und Touch
// ===================================================================

namespace expanderPins
{
    constexpr uint8_t TP_RST = 1;  ///< Touch-Controller Reset
    constexpr uint8_t LCD_BL = 2;  ///< Display Hintergrundbeleuchtung
    constexpr uint8_t LCD_RST = 3; ///< Display Reset
    constexpr uint8_t SD_CS = 4;   ///< SD-Karten Chip-Select
    constexpr uint8_t USB_SEL = 5; ///< USB-Umschalt-Pin
}

// ===================================================================
// I2C-Bus
// ===================================================================

namespace i2cPins
{
    constexpr int8_t I2C_MASTER_NUM = I2C_NUM_0;         /// I2C-Port-Nummer)
    constexpr gpio_num_t I2C_MASTER_SDA_IO = GPIO_NUM_8; ///< I2C-Datenleitung
    constexpr gpio_num_t I2C_MASTER_SCL_IO = GPIO_NUM_9; ///< I2C-Taktleitung
}

// ===================================================================
// I2S-Audio
// ===================================================================

namespace i2sPins
{
    constexpr gpio_num_t I2S_DOUT = GPIO_NUM_43; ///< (TXD) I2S Datenausgang
    constexpr gpio_num_t I2S_BCLK = GPIO_NUM_44; ///< (RXD) I2S Bit-Clock
    constexpr gpio_num_t I2S_LRC = GPIO_NUM_6;   ///< (Sensor AD Pin) I2S Word-Select
}

// ===================================================================
// Lautsprecher-Abschaltung über den RS485-Treiber
// ===================================================================
namespace speakerPins
{
    constexpr gpio_num_t RS485_CONTROL = GPIO_NUM_16; ///< HIGH: B/TX- niedrig; LOW: B/TX- hoch; Abschlusswiderstand muss ausgeschaltet sein
}

// ===================================================================
// SPI (SD-Karte)
// ===================================================================
namespace spiPins
{
    constexpr gpio_num_t SD_MOSI = GPIO_NUM_11; ///< SPI-Datenausgang
    constexpr gpio_num_t SD_CLK = GPIO_NUM_12;  ///< SPI-Taktleitung
    constexpr gpio_num_t SD_MISO = GPIO_NUM_13; ///< SPI-Dateneingang
    constexpr gpio_num_t SD_SS = GPIO_NUM_4;    ///< SPI Chip-Select der SD-Karte
}

// ===================================================================
// RGB-Display-Bus
// ===================================================================

namespace rgbPins
{
    constexpr gpio_num_t LCD_RGB_D0 = GPIO_NUM_14;    ///< RGB-Datenleitung D0
    constexpr gpio_num_t LCD_RGB_D1 = GPIO_NUM_38;    ///< RGB-Datenleitung D1
    constexpr gpio_num_t LCD_RGB_D2 = GPIO_NUM_18;    ///< RGB-Datenleitung D2
    constexpr gpio_num_t LCD_RGB_D3 = GPIO_NUM_17;    ///< RGB-Datenleitung D3
    constexpr gpio_num_t LCD_RGB_D4 = GPIO_NUM_10;    ///< RGB-Datenleitung D4
    constexpr gpio_num_t LCD_RGB_D5 = GPIO_NUM_39;    ///< RGB-Datenleitung D5
    constexpr gpio_num_t LCD_RGB_D6 = GPIO_NUM_0;     ///< RGB-Datenleitung D6
    constexpr gpio_num_t LCD_RGB_D7 = GPIO_NUM_45;    ///< RGB-Datenleitung D7
    constexpr gpio_num_t LCD_RGB_D8 = GPIO_NUM_48;    ///< RGB-Datenleitung D8
    constexpr gpio_num_t LCD_RGB_D9 = GPIO_NUM_47;    ///< RGB-Datenleitung D9
    constexpr gpio_num_t LCD_RGB_D10 = GPIO_NUM_21;   ///< RGB-Datenleitung D10
    constexpr gpio_num_t LCD_RGB_D11 = GPIO_NUM_1;    ///< RGB-Datenleitung D11
    constexpr gpio_num_t LCD_RGB_D12 = GPIO_NUM_2;    ///< RGB-Datenleitung D12
    constexpr gpio_num_t LCD_RGB_D13 = GPIO_NUM_42;   ///< RGB-Datenleitung D13
    constexpr gpio_num_t LCD_RGB_D14 = GPIO_NUM_41;   ///< RGB-Datenleitung D14
    constexpr gpio_num_t LCD_RGB_D15 = GPIO_NUM_40;   ///< RGB-Datenleitung D15
    constexpr gpio_num_t LCD_RGB_DE = GPIO_NUM_5;     ///< Data-Enable-Signal des RGB-Displays
    constexpr gpio_num_t LCD_RGB_VSYNC = GPIO_NUM_3;  ///< Vertikales Sync-Signal des RGB-Displays
    constexpr gpio_num_t LCD_RGB_HSYNC = GPIO_NUM_46; ///< Horizontales Sync-Signal des RGB-Displays
    constexpr gpio_num_t LCD_RGB_PCLK = GPIO_NUM_7;   ///< Pixeltakt des RGB-Displays
}

#endif // PINS_H
