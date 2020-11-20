// XPT2046A.h
//
/* XPT2046 / ADS7843 TouchPad Controller Driver
   Adapted from https://github.com/spapadim/XPT2046 and https://github.com/PaulStoffregen/XPT2046_Touchscreen
   See http://www.ti.com/lit/an/sbaa036/sbaa036.pdf "Touch Screen Controller Tips"
   for information on algorithms. See data sheets for information on SPI timing:
   - https://ldm-systems.ru/f/doc/catalog/HY-TFT-2,8/XPT2046.pdf
   - http://www.ti.com/lit/ds/symlink/ads7843.pdf
*/
#ifndef _XPT2046A_h
#define _XPT2046A_h

#include "gpio.hpp"

struct Point {
    long    x, y;
};


class XPT2046A {
public:

    XPT2046A(GPIO &pin_CS, GPIO &pin_PenIRQ);
    XPT2046A() {}
    void setupPins(GPIO& pin_CS, GPIO& pin_PenIRQ);
    bool isTouching();
    void getADCData(Point &adc_data);

private:
    const uint16_t ADC_MAX = 0x0fff;

    GPIO* cs;
    GPIO* irq;

    int16_t readADC(uint8_t ctrl) const;
};
#endif
