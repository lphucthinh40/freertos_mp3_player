//
// XPT2046 Touch Controller
//
#include "touch.h"
#include "ssp1.h"
#include "utilities.h"
#define REQ_Z1  0xB1
#define REQ_Z2  0xC1
#define REQ_X   0x91
#define REQ_Y   0xD1
#define REQ_Y_POWERDN 0xD0
#define MAX_READS 0xFF

inline static void swap(long &a, long &b) {
    long tmp = a;
    a = b;
    b = tmp;
}

XPT2046A::XPT2046A(GPIO &pin_CS, GPIO &pin_PenIRQ)
{
    cs = &pin_CS;
    irq = &pin_PenIRQ;
    cs->setAsOutput();
    irq->setAsInput();
    cs->setHigh();
};

void XPT2046A::setupPins(GPIO &pin_CS, GPIO &pin_PenIRQ)
{
    cs = &pin_CS;
    irq = &pin_PenIRQ;
    cs->setAsOutput();
    irq->setAsInput();
    cs->setHigh();
};

void XPT2046A::getADCData(Point &adc_data)
{
    cs->setLow();                      // cs enabled
    int16_t x, y;
    /* Pressure reading not implemented. */
    /* REMEMBER that the SPI reads the data from the PREVIOUS SPI Request */
    ssp1_exchange_byte(REQ_X);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(REQ_X);
    ssp1_exchange_byte(0x00);

    x = readADC(REQ_X);

    ssp1_exchange_byte(REQ_Y);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(REQ_Y);
    ssp1_exchange_byte(0x00);

    y = readADC(REQ_Y);

    ssp1_exchange_byte(REQ_Y_POWERDN);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x00);
    ssp1_exchange_byte(0x00);
//    ssp1_exchange_byte(0x00);
//    ssp1_exchange_byte(0x00);
    cs->setHigh();                      // cs disabled
    /* Normalization for different orientation and direction of touchpad. This may be
       different for your module. The purpose of the normalization is to achieve the same
       origin points (0,0) for the TFT and Touchpad. */
    adc_data.y = 16 - x;
    adc_data.x = y;
}

bool XPT2046A::isTouching()
{
    return !(irq->read());
}

int16_t XPT2046A::readADC(uint8_t ctrl) const
{
    uint16_t prev = 0xffff, cur = 0xffff;
    uint8_t i = 0;
    do
    {
        prev = cur;
        cur = ssp1_exchange_byte16(ctrl) >> 3;
    } while ((prev != cur) && (++i < MAX_READS));

    return cur;
}
