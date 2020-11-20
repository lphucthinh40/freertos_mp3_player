#include "GUI.hpp"

mp3GUI::mp3GUI(char* base_address, uint32_t size, GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight, GPIO &touch_cs, GPIO &touch_irq)
    {
        lcd = new TFT_LCD(240, 320);
        touch = new XPT2046A();
        base_addr = base_address;
        buffer_size = size;
        lcd->setupPins(cs_pin, reset_pin, dc_pin, backlight);
        touch->setupPins(touch_cs, touch_irq);
    }

mp3GUI::~mp3GUI()
{
    delete lcd;
    delete touch;
}

void mp3GUI::drawGradientRec(uint16_t starting_color, uint16_t ending_color, float start_at, float end_at, uint8_t step)
{
    uint16_t y = 0;
       uint8_t offset = 0;
       if (starting_color == Red) offset = 11;
       for (uint8_t i = 0; i < 32; i++)
       {
           lcd->fillRect(0, y, 240, 8, starting_color - (i << offset));
           y += 8;
       }
}
