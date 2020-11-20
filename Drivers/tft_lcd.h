#ifndef TFT_LCD_H_
#define TFT_LCD_H_

#include "Adafruit_GFX_Mod.h"
#include "ssp1.h"
#include "gpio.hpp"
#include "LPC17xx.h"
#include "utilities.h"

/* some RGB color definitions (ramanSpectrometer)                                                */
#define Black           0x0000      /*   0,   0,   0 */
#define Navy            0x000F      /*   0,   0, 128 */
#define DarkGreen       0x03E0      /*   0, 128,   0 */
#define DarkCyan        0x03EF      /*   0, 128, 128 */
#define Maroon          0x7800      /* 128,   0,   0 */
#define Purple          0x780F      /* 128,   0, 128 */
#define Olive           0x7BE0      /* 128, 128,   0 */
#define LightGrey       0xC618      /* 192, 192, 192 */
#define DarkGrey        0x7BEF      /* 128, 128, 128 */
#define Blue            0x001F      /*   0,   0, 255 */
#define Green           0x07E0      /*   0, 255,   0 */
#define Cyan            0x07FF      /*   0, 255, 255 */
#define Red             0xF800      /* 255,   0,   0 */
#define Magenta         0xF81F      /* 255,   0, 255 */
#define Yellow          0xFFE0      /* 255, 255,   0 */
#define White           0xFFFF      /* 255, 255, 255 */
#define Orange          0xFD20      /* 255, 165,   0 */
#define GreenYellow     0xAFE5      /* 173, 255,  47 */


// Rewrote for SJ-One Board based on Adafruit's GFX Library and ramanSpectrometer's GitHub for ILI9341
//: public Adafruit_GFX
class TFT_LCD : public Adafruit_GFX {
    public:
        TFT_LCD(uint16_t width, uint16_t height, GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight);
        TFT_LCD(uint16_t width, uint16_t height);
        void setupPins(GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight);
        // TFT initialization
        void LCD_reset(void);

        // Window selection
        void selectAddressWindow (uint16_t x, uint16_t y, uint16_t w, uint16_t h); // select a window on screen
        void selectFullWindow(void);                                                       // select the whole screen
        void setRotation(uint8_t m);
        void invertDisplay(bool invert);

        // MCU & LCD transactions via SPI
        void startWrite(void);
        void endWrite(void);
        void writeCommand(uint8_t cmd);
        // note: For 'writeData', we can directly use ssp1_exchange_byte to send data to LCD

        // Basic drawing utilities
        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void writePixel(int16_t x, int16_t y, uint16_t color);
        void writePixels(uint16_t *colors, uint32_t len);
        void writeColor(uint16_t color, uint32_t len);
        void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        inline void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
        void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
        //void drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h);
        uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

        // More advance drawing utilities

    private:
        GPIO* cs;
        GPIO* rst;
        GPIO* dc;
        GPIO* bl;

        uint16_t max_width = 0, max_height = 0;       // Dimension of LCD Screen
        uint16_t current_width = 0, current_height = 0; // Dimension of the selected Window
};



#endif /* TFT_LCD_H_ */
