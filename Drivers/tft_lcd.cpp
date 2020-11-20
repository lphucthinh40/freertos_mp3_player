#include "tft_lcd.h"

// CONSTRUCTOR
TFT_LCD::TFT_LCD(uint16_t width, uint16_t height, GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight) : Adafruit_GFX(width, height)
{
    max_width = width;
    max_height = height;
    cs = &cs_pin;
    rst = &reset_pin;
    dc = &dc_pin;
    bl = &backlight;

    cs->setAsOutput();
    rst->setAsOutput();
    dc->setAsOutput();
    bl->setAsOutput();
    bl->setHigh();
}

TFT_LCD::TFT_LCD(uint16_t width, uint16_t height) : Adafruit_GFX(width, height)
{
    max_width = width;
    max_height = height;
}

void TFT_LCD::setupPins(GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight)
{
    cs = &cs_pin;
    rst = &reset_pin;
    dc = &dc_pin;
    bl = &backlight;
    cs->setAsOutput();
    rst->setAsOutput();
    dc->setAsOutput();
    bl->setAsOutput();
    bl->setHigh();
}

void TFT_LCD::startWrite(void)
{
    cs->setLow();
}
void TFT_LCD::endWrite(void)
{
    cs->setHigh();
}

void TFT_LCD::writeCommand(uint8_t cmd)
{
    dc->setLow();
    cs->setLow();
    ssp1_exchange_byte(cmd);
    dc->setHigh();
}

void TFT_LCD::selectAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    writeCommand(0x2A);
    ssp1_exchange_byte(x >> 8);
    ssp1_exchange_byte(x);
    ssp1_exchange_byte((x+w-1) >> 8);
    ssp1_exchange_byte(x+w-1);
    cs->setHigh();

    writeCommand(0x2B);
    ssp1_exchange_byte(y >> 8);
    ssp1_exchange_byte(y);
    ssp1_exchange_byte((y+h-1) >> 8);
    ssp1_exchange_byte(y+h-1);
    cs->setHigh();

    writeCommand(0x2C);
    cs->setHigh();

    current_width = w;
    current_height = h;
}

#define MADCTL_MY  0x80  ///< Bottom to top
#define MADCTL_MX  0x40  ///< Right to left
#define MADCTL_MV  0x20  ///< Reverse Mode
#define MADCTL_ML  0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00  ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08  ///< Blue-Green-Red pixel order
#define MADCTL_MH  0x04  ///< LCD refresh right to left
#define ILI9341_MADCTL     0x36     ///< Memory Access Control


void TFT_LCD::setRotation(uint8_t m) {
    rotation = m % 4; // can't be higher than 3
    switch (rotation) {
        case 0:
            m = (MADCTL_MX | MADCTL_BGR);
            _width  = max_width;
            _height = max_height;
            break;
        case 1:
            m = (MADCTL_MV | MADCTL_BGR);
            _width  = max_height;
            _height = max_width;
            break;
        case 2:
            m = (MADCTL_MY | MADCTL_BGR);
            _width  = max_width;
            _height = max_height;
            break;
        case 3:
            m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            _width  = max_height;
            _height = max_width;
            break;
    }

    writeCommand(ILI9341_MADCTL);
    ssp1_exchange_byte(m);
    cs->setHigh();
}


#define ILI9341_INVOFF     0x20     ///< Display Inversion OFF
#define ILI9341_INVON      0x21     ///< Display Inversion ON

void TFT_LCD::invertDisplay(bool invert) {
    writeCommand(invert ? ILI9341_INVON : ILI9341_INVOFF);
    cs->setHigh();
}

void TFT_LCD::selectFullWindow(void)
{
    selectAddressWindow(0, 0, max_width, max_height);
    current_width = max_width;
    current_height = max_height;
}

void TFT_LCD::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Clip first...
    if((x >= 0) && (x < max_width) && (y >= 0) && (y < max_height)) {
        // THEN set up transaction (if needed) and draw...
        startWrite();
        selectAddressWindow(x, y, 1, 1);
        cs->setLow();
        ssp1_exchange_byte(color >> 8);
        ssp1_exchange_byte(color);
        endWrite();
    }
}

void TFT_LCD::writePixel(int16_t x, int16_t y, uint16_t color) {
    if((x >= 0) && (x < max_width) && (y >= 0) && (y < max_height))
    {
        selectAddressWindow(x, y, 1, 1);
        cs->setLow();
        ssp1_exchange_byte(color >> 8);
        ssp1_exchange_byte(color);
    }
}

void TFT_LCD::writePixels(uint16_t *colors_ptr, uint32_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        ssp1_exchange_byte(*colors_ptr >> 8);
        ssp1_exchange_byte(*colors_ptr);
        colors_ptr++;
    }
}

void TFT_LCD::writeColor(uint16_t color, uint32_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        ssp1_exchange_byte(color >> 8);
        ssp1_exchange_byte(color);
    }
}

void TFT_LCD::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w == 0 || h == 0)   return;     // if w or h is zero, do nothing
    // otherwise, check x, y, w, and h to make sure the rectangle is within address window
    if (x < 0)
    {
        x = x + w + 1;  // move x to left edge
        w = -w;          // make w positive
    }
    if (x < max_width)
    {
        if (h < 0)
        {
            y = y + h + 1;
            h = -h;
        }
        if (y < max_height)
        {
            int16_t x2 = x + w - 1;
            if (x2 >= 0)
            {
                int16_t y2 = y + h -1;
                if (y2 >= 0)
                {
                    // Rectangle partly or fully overlaps screen
                    if(x  <  0)       { x = 0; w = x2 + 1; }        // Clip left
                    if(y  <  0)       { y = 0; h = y2 + 1; }        // Clip top
                    if(x2 >= max_width)  { w = max_width  - x;   }  // Clip right
                    if(y2 >= max_height) { h = max_height - y;   }  // Clip bottom
                    writeFillRectPreclipped(x, y, w, h, color);
                }
            }
        }
    }
}

inline void TFT_LCD::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    selectAddressWindow(x, y, w, h);
    cs->setLow();
    writeColor(color, (uint32_t)w * h);
}

void TFT_LCD::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if((y >= 0) && (y < max_height) && w)   // Y on screen, nonzero width
    {
        if(w < 0)                           // If negative width...
        {
            x +=  w + 1;                    //   Move X to left edge
            w  = -w;                        //   Use positive width
        }
        if(x < max_width)                   // Not off right
        {
            int16_t x2 = x + w - 1;
            if(x2 >= 0)                     // Not off left
            {
                // Line partly or fully overlaps screen
                if(x  <  0)       { x = 0; w = x2 + 1; }        // Clip left
                if(x2 >= max_width)  { w = max_width  - x;   }  // Clip right
                writeFillRectPreclipped(x, y, w, 1, color);
            }
        }
    }
}

void TFT_LCD::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if((x >= 0) && (x < max_width) && h)    // X on screen, nonzero height
    {
        if(h < 0)                           // If negative height...
        {
            y +=  h + 1;                    //   Move Y to top edge
            h  = -h;                        //   Use positive height
        }
        if(y < max_height)                  // Not off bottom
        {
            int16_t y2 = y + h - 1;
            if(y2 >= 0)                     // Not off top
            {
                // Line partly or fully overlaps screen
                if(y  <  0)       { y = 0; h = y2 + 1; }        // Clip top
                if(y2 >= max_height) { h = max_height - y;   }  // Clip bottom
                writeFillRectPreclipped(x, y, 1, h, color);
            }
        }
    }
}
//
//void TFT_LCD::drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h)
//{
//    // Both x and y must be in valid positions
//    int16_t x2, y2; // Lower-right coordinate
//    if(( x             >= max_width ) ||      // Off-edge right
//       ( y             >= max_height) ||      // " top
//       ((x2 = (x+w-1)) <  0      )    ||      // " left
//       ((y2 = (y+h-1)) <  0)     ) return;    // " bottom
//
//    int16_t bx1=0, by1=0,   // Clipped top-left within bitmap
//    saveW=w;                // Save original bitmap width value
//    if(x < 0)               // Clip left
//    {
//        w  +=  x;
//        bx1 = -x;
//        x   =  0;
//    }
//    if(y < 0)               // Clip top
//    {
//        h  +=  y;
//        by1 = -y;
//        y   =  0;
//    }
//    if(x2 >= max_width ) w = max_width  - x; // Clip right
//    if(y2 >= max_height) h = max_height - y; // Clip bottom
//
//    pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
//    selectAddressWindow(x, y, w, h); // Clipped area
//    while(h--)
//    {     // For each (clipped) scanline...
//          writePixels(pcolors, w); // Push one (clipped) row
//          pcolors += saveW; // Advance pointer by one full (unclipped) line
//    }
//}

uint16_t TFT_LCD::color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


void TFT_LCD::LCD_reset()
{
    // NOTE: - cs, dc, reset are active LOW
    //       - SSP1 should be initialized outside already
    //       - SPI format: 8-bit, mode 0
    ssp1_set_max_clock(10);             // 10 Mhz SPI clock
    cs->setHigh();                      // cs high
    dc->setHigh();                      // dc high

    // Hardware reset
    rst->setLow();                         // START RESET LCD
    delay_us(50);

    rst->setHigh();                         // END RESET LCD
    delay_ms(5);

    // Software reset
    writeCommand(0x01);                    // SW reset
    delay_ms(5);

    writeCommand(0x28);                     // display off

    // Start Initial Sequence
    // NOTE: cs is set high (deactivate) outside to reduce overhead due to consecutive 'writeByte' function calls
    writeCommand(0xCF);  // Power Control B
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0xC1);
    ssp1_exchange_byte(0x30);

    cs->setHigh();

    writeCommand(0xED); // Power Sequence Control
    ssp1_exchange_byte(0x64);
    ssp1_exchange_byte(0x03);
    ssp1_exchange_byte(0x12);
    ssp1_exchange_byte(0x81);

    cs->setHigh();

    writeCommand(0xE8); // Driver Timing Control 1
    ssp1_exchange_byte(0x85);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x78);

    cs->setHigh();

    writeCommand(0xCB); // Power Control A
    ssp1_exchange_byte(0x39);
    ssp1_exchange_byte(0x2C);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x34);
    ssp1_exchange_byte(0x02);

    cs->setHigh();

    writeCommand(0xF7); // Pump Ratio Control
    ssp1_exchange_byte(0x20);

    cs->setHigh();

    writeCommand(0xEA); // Driver Timing Control 2
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x00);

    cs->setHigh();

    writeCommand(0xC0);                     // POWER_CONTROL_1
    ssp1_exchange_byte(0x23);

    cs->setHigh();

    writeCommand(0xC1);                     // POWER_CONTROL_2
    ssp1_exchange_byte(0x10);

    cs->setHigh();

    writeCommand(0xC5);                     // VCOM_CONTROL_1
    ssp1_exchange_byte(0x3E);
    ssp1_exchange_byte(0x28);

    cs->setHigh();

    writeCommand(0xC7);                     // VCOM_CONTROL_2
    ssp1_exchange_byte(0x86);

    cs->setHigh();

    writeCommand(0x36);                     // MEMORY_ACCESS_CONTROL
    ssp1_exchange_byte(0x48);

    cs->setHigh();

    writeCommand(0x3A);                     // COLMOD_PIXEL_FORMAT_SET
    ssp1_exchange_byte(0x55);                        // 16 bit pixel

    cs->setHigh();

    writeCommand(0xB1);                     // Frame Rate
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x18);

    cs->setHigh();

    writeCommand(0xF2);                     // Gamma Function Disable
    ssp1_exchange_byte(0x00);

    cs->setHigh();

    writeCommand(0x26);
    ssp1_exchange_byte(0x01);                 // gamma set for curve 01/2/04/08

    cs->setHigh();

    writeCommand(0xE0);                     // positive gamma correction
    ssp1_exchange_byte(0x1F);
    ssp1_exchange_byte(0x1A);
    ssp1_exchange_byte(0x18);
    ssp1_exchange_byte(0x0A);
    ssp1_exchange_byte(0x0F);
    ssp1_exchange_byte(0x06);
    ssp1_exchange_byte(0x45);
    ssp1_exchange_byte(0x87);
    ssp1_exchange_byte(0x32);
    ssp1_exchange_byte(0x0A);
    ssp1_exchange_byte(0x07);
    ssp1_exchange_byte(0x02);
    ssp1_exchange_byte(0x07);
    ssp1_exchange_byte(0x05);
    ssp1_exchange_byte(0x00);

    cs->setHigh();

    writeCommand(0xE1);                     // negativ gamma correction
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x25);
    ssp1_exchange_byte(0x27);
    ssp1_exchange_byte(0x05);
    ssp1_exchange_byte(0x10);
    ssp1_exchange_byte(0x09);
    ssp1_exchange_byte(0x3A);
    ssp1_exchange_byte(0x78);
    ssp1_exchange_byte(0x4D);
    ssp1_exchange_byte(0x05);
    ssp1_exchange_byte(0x18);
    ssp1_exchange_byte(0x0D);
    ssp1_exchange_byte(0x38);
    ssp1_exchange_byte(0x3A);
    ssp1_exchange_byte(0x1F);

    cs->setHigh();

//    selectFullWindow();
//    cs->setHigh();

//    writeCommand(0xB7);                       // entry mode
//    ssp1_exchange_byte(0x07);

    cs->setHigh();

    writeCommand(0xB6);                       // display function control
    ssp1_exchange_byte(0x08);
    ssp1_exchange_byte(0x82);
    ssp1_exchange_byte(0x27);
    ssp1_exchange_byte(0x00);

    cs->setHigh();

    writeCommand(0x11);                     // sleep out

    cs->setHigh();
    delay_ms(1000);

    writeCommand(0x29);                     // display on

    cs->setHigh();
    delay_ms(1000);

    writeCommand(0x13);                     // display on
}
