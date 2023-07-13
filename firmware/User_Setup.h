/**
 * @file User_Setup.h
 * @author Ananta Srikar
 * @brief Configuration file for ST7735 SPI based display to be used with TFT_eSPI Library
 * @version 0.2
 * @date 2023-07-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
// Setup for the ESP32 with ST7735 128x160 display

#define USER_SETUP_ID 70

#define ST7735_DRIVER

// #define TFT_SDA_READ // Display has a bidirectional SDA pin (no MISO)

#define TFT_WIDTH  128
#define TFT_HEIGHT 160

#define TFT_MOSI 5
#define TFT_SCLK 4
#define TFT_CS    2  // Chip select control pin
#define TFT_DC    18 // Data Command control pin
#define TFT_RST   19  // Reset pin (could connect to RST pin)

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  16000000