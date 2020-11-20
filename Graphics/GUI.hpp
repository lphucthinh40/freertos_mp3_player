#ifndef GUI_HPP_
#define GUI_HPP_

#include "tft_lcd.h"
#include "touch.h"

#define REC_W 240
#define REC_H 240
#define REC_x 0
#define REC_y 0
// Animation Events
enum GUI_Event
{
    do_nothing,
    do_next,
    do_prev,
    do_pause,
    do_play,
    do_load
};

// Touch Area Definitions
#define TOUCH_TOLERANCE     2
#define PLAY_X          7
#define PLAY_Y          14
#define NEXT_X          12
#define NEXT_Y          14
#define PREV_X          4
#define PREV_Y          14

// GUI_Object Struct
enum colorType
{
    mono,       // 1 bit per pixel (only one color)
    mono_mod,   // 1 bit per pixel (color transition)
    rgb16       // 16 bits per pixel
};

#define BACKGROUND_COLOR    Black
#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       320

struct GUI_object
{
    // display attributes
    uint8_t x0;                 // (x0, y0) is always the top-leftest pixel
    uint8_t y0;
    uint8_t width;
    uint8_t height;
    uint8_t layer_level;
    colorType color_type;
    uint16_t starting_color;   // fixed color for mono, starting color for mono_mod
    uint16_t ending_color;     // only useful for mono_mod
    // data attributes
    uint32_t base_addr;
    uint32_t len;
};

// This class handle touchscreen and graphic processing for mp3 player

//TFT_LCD lcd(SCREEN_WIDTH, SCREEN_HEIGHT);
//XPT2046A touch;

class mp3GUI
{
    public:
    mp3GUI(char* base_address, uint32_t size, GPIO &cs_pin, GPIO &reset_pin, GPIO &dc_pin, GPIO &backlight, GPIO &touch_cs, GPIO &touch_irq);
    ~mp3GUI();
    void loadMainScreen();
    void react(GUI_Event newEvent);

    void drawGradientRec(uint16_t starting_color, uint16_t ending_color, float start_at, float end_at, uint8_t step = 11);

    private:
    TFT_LCD* lcd = NULL;
    XPT2046A* touch = NULL;
    char* base_addr = NULL;
    uint32_t  buffer_size = 0;

};



#endif /* GUI_HPP_ */
