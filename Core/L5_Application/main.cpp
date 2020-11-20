#include "FreeRTOS.h"
#include "task.h"
#include "uart0_min.h"
#include "tft_lcd.h"
#include "str.hpp"
#include "gpio.hpp"
#include "ssp1.h"
#include "ssp0.h"
#include "LPC17xx.h"
#include "glcdfont.c"
#include "FreeMonoBoldOblique12pt7b.h"
#include "FreeMono9pt7b.h"
#include "FreeMonoBold9pt7b.h"
#include "FreeMonoBold12pt7b.h"
#include "semphr.h"
#include "touch.h"
#include "storage.hpp"
#include "lpc_sys.h"
#include "command_handler.hpp"
#include "tasks.hpp"
#include "ff.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#define VOLSTEP 0x0F
#define VOLINIT 0x00//0x40
uint8_t current_volume = VOLINIT;

TaskHandle_t LCD_handle = NULL;
SemaphoreHandle_t touchIREQ = xSemaphoreCreateBinary();

//char* title[3] = {"Game of Thrones", "On My Way", "Sway"};
//char* listofsong[3] = {"1:Game_of_Thrones.mp3", "1:On_My_Way.mp3", "1:Sway.mp3"};
//char* song_list[3] = {"1:Game_of_Thrones.mp3", "1:On_My_Way.mp3", "1:Sway.mp3"};
char** song_list;
uint8_t playlist_size = 0;
uint8_t title_index = 0;
uint8_t song_index = 0;
uint8_t current_mode = 0;

// |P1.19, P1.20, P1.22| -> |cs, reset, dc|
GPIO cs_pin(P1_19);
GPIO reset_pin(P1_20);
GPIO dc_pin(P1_22);
GPIO backlight_pin(P1_23);
TFT_LCD lcd(240, 320, cs_pin, reset_pin, dc_pin, backlight_pin);

enum Effect
{   // music playing
    do_next,
    do_prev,
    do_pause,
    do_play,
    do_volume_off,
    do_volume_on,
    do_change_mode
    //
};

void updateModeButton()
{
    lcd.fillCircle(66, 178, 22, DarkGrey);
    switch(current_mode)
    {
        case 0:     lcd.drawBitmap(50, 162, replay, 32, 32, White);
                    break;
        case 1:     lcd.drawBitmap(50, 162, replay1, 32, 32, White);
                    break;
        case 2:     lcd.drawBitmap(50, 162, shuffle, 32, 32, White);
                    break;
    }
}

void react(Effect pen_event)
{
    switch (pen_event)
{
    case do_next:
            {
                    lcd.drawBitmap(166, 276, next, 32, 32, Yellow);
                    delay_ms(600);
                    lcd.drawBitmap(166, 276, next, 32, 32, White);
            }
            break;
    case do_prev:
            {
                    lcd.drawBitmap(42, 276, back, 32, 32, Yellow);
                    delay_ms(800);
                    lcd.drawBitmap(42, 276, back, 32, 32, White);
            }
            break;
    case do_pause:
            {
                    lcd.fillRect(105, 271, 40, 40, Black);
                    lcd.drawBitmap(104, 276, play_button, 32, 32, White);
//                    lcd.fillRect(0, 230, 240, 16, Yellow);
//                    lcd.setFont();
//                    lcd.setTextColor(Black);
//                    lcd.setCursor(30, 234);
//                    lcd.writeln(" Paused: ");
//                    lcd.writeln(title[title_index]);
            }
            break;
    case do_play:
            {
                    lcd.fillRect(105, 271, 40, 40, Black);
                    lcd.drawBitmap(104, 276, pause, 32, 32, White);
            }
            break;
    case do_volume_on:
            {
                lcd.fillCircle(166, 178, 22, DarkGrey);
                lcd.drawBitmap(150, 162, volume_on, 32, 32, White);
            }
            break;
    case do_volume_off:
            {
                lcd.fillCircle(166, 178, 22, DarkGrey);
                lcd.drawBitmap(150, 162, volume_off, 32, 32, White);
            }
            break;
    case do_change_mode:
            {
                updateModeButton();
            }
            break;
}

}


#define REC_W 240
#define REC_H 320
#define REC_x 0
#define REC_y 0


void drawGradientRec(uint16_t starting_color = Blue, uint16_t ending_color = Black, float start_at = 0, float end_at = 0)
{
    uint16_t y = 0;
    uint8_t offset = 0;
    uint8_t space = 32;
    uint8_t step_width = 8;
    switch (starting_color)
    {
        case Red:
                {    offset = 11;
                     space = 32;
                     step_width = 8;
                }
                break;
        case Blue:
                {    offset = 0;
                     space = 32;
                     step_width = 8;
                }
                break;
        case Green:
                {    offset = 5;
                     space = 64;
                     step_width = 4;
                }
                break;
    }
    for (uint8_t i = 0; i < space; i++)
    {
        lcd.fillRect(0, y, 240, step_width, starting_color - (i << offset));
        y += step_width;
    }
    lcd.fillRect(0, y, 240, 8, Black);
}

char** get_song_list(uint8_t &size)
{
   DIR Dir;
   FILINFO Finfo;
   FRESULT returnCode = FR_OK;
   static char lfn[20];
   Finfo.lfname = lfn;
   Finfo.lfsize = sizeof(lfn);
   char** songlist = (char**)malloc(20*(sizeof(char*)));

   static unsigned int numFiles = 0;       //Count number of file
   const char *dirPath = "1:";             //
   //Falty path, should not happen in a controlled environment
   if (FR_OK != (returnCode = f_opendir(&Dir, dirPath))) {
       //printf("Invalid directory: |%s| (Error %i)\n", dirPath, returnCode);
       return NULL;
   }
   for (;;)
   {
       returnCode = f_readdir(&Dir, &Finfo);
       if ( (FR_OK != returnCode) || !Finfo.fname[0]) {
           break;
       }
       //getting songname
       char* songname= Finfo.lfname;
       if (songname[strlen(songname)-1] != '3') continue;
       char s[20];
       strcpy(s, dirPath);
       strcat(s,songname);
       songlist[numFiles] = (char*)malloc( sizeof(char) * strlen(s));
       memcpy(songlist[numFiles], s, sizeof(s));
       numFiles++;
   }
   size = numFiles;
   return songlist;
}

uint8_t color_index = 0;
uint16_t color[3] = {Red, Blue, Green};

void loadMainScreen()
{
    drawGradientRec(color[color_index], Black, 0.1, 0.8);
    // Art Designs Area
    lcd.drawBitmap(0, 0, music, 240, 240, Black);
    lcd.setFont(&FreeMonoBoldOblique12pt7b);

    lcd.setCursor(84, 80);
    lcd.setTextColor(Yellow);
    lcd.writeln("Feel");
    lcd.setCursor(87, 100);
    lcd.writeln("The");
    lcd.setCursor(84, 120);
    lcd.writeln("Beat!");
    lcd.fillRect(105, 271, 40, 40, Black);
    lcd.drawBitmap(104, 276, pause, 32, 32, White);
    lcd.drawBitmap(166, 276, next, 32, 32, White);
    lcd.drawBitmap(42, 276, back, 32, 32, White);
//    lcd.drawBitmap(180, 150, volup, 32, 32, DarkGrey);
//    lcd.drawBitmap(140, 150, voldown, 32, 32, DarkGrey);
//    lcd.fillRoundRect(124, 166, 84, 32, 8, 0xB000);
    lcd.setFont(&FreeMonoBold12pt7b);
    lcd.setTextColor(DarkGrey);
    lcd.setCursor(126, 184);
    lcd.write('-');
    lcd.setCursor(194, 184);
    lcd.write('+');

    lcd.fillCircle(166, 178, 22, DarkGrey);
    //lcd.fillRect(90, 176, 32, 32, color[color_index]);
    if (current_volume == 0xFE) lcd.drawBitmap(150, 162, volume_off, 32, 32, White);
    else                        lcd.drawBitmap(150, 162, volume_on, 32, 32, White);

    updateModeButton();

    // Song Info Area
    lcd.fillRect(0, 224, 240, 20, Yellow);
    //lcd.setFont();
    lcd.setFont(&FreeMonoBold9pt7b);
    lcd.setTextColor(Black);
    //lcd.setCursor(30, 234);
    lcd.setCursor(4, 238);
    lcd.writeln("Now : "); // playing
    lcd.writeln(song_list[title_index]); // playing
    //lcd.setCursor(30, 250);
    lcd.setFont(&FreeMono9pt7b);
    lcd.setCursor(4, 258);
    lcd.setTextColor(DarkGrey);
    lcd.writeln("Next: "); // next
    lcd.writeln(song_list[(title_index + 1) % playlist_size]); // next
    //lcd.setCursor(30, 220);
    lcd.setCursor(4, 218);
    lcd.setTextColor(DarkGrey);
    lcd.writeln("Prev: "); // previous
    lcd.writeln(song_list[(title_index + playlist_size - 1) % playlist_size]); // previous
    color_index = (color_index + 1) % 3;

}

void check()
{
    long yield = 0;
    //touch.getADCData(pos);
    //delay_ms(1000);
    xSemaphoreGiveFromISR(touchIREQ, &yield);
    portYIELD_FROM_ISR(yield);
}


void interrupt_handler()
{
    printf("in");
    if (LPC_GPIOINT->IO2IntStatF & (1 << 1))
    {   LPC_GPIOINT->IO2IntEnF &= ~(1 << 1); // disable interrupt
        LPC_GPIOINT->IO2IntClr |= (1 << 1);
        //printf("pen_irq: %d\n", NVIC_GetPendingIRQ(EINT3_IRQn));
        //LPC_GPIOINT->IO2IntEnF |= (1 << 1); // INT for P2.1
        check();
        //xSemaphoreGiveFromISR(touchIREQ, NULL);
    }
}

// MUSIC PLAYING PARTS ****************************************************
GPIO cs_decoder(P0_0);
GPIO rst_decoder(P0_1);
GPIO dreq_pin(P0_30);
static SemaphoreHandle_t spi_bus_lock = xSemaphoreCreateMutex();
TaskHandle_t PlayerTask_handle = NULL;

bool cancel_song = false;
bool change_vol = false;

void adesto_cs(void)
{
    cs_decoder.setLow();
}

void rst(void)
{
    rst_decoder.setAsOutput();
    rst_decoder.setLow();
    vTaskDelay(1000);
    rst_decoder.setHigh();
    vTaskDelay(1000);


}


void adesto_ds(void)
{
    cs_decoder.setHigh();
}

bool ssp1_locked = false; // needed to resolve conflict between read SDcard and write LCD (both using spi1)
void loadSong(str filename,unsigned int& totalBytesRead, bool& eof_flag)
{
    uint8_t buffer[512] = { 0 };
    FRESULT status = FR_OK;
    unsigned int bytesRead = 0;
    unsigned int transmission_size = 32;
    unsigned int bytesTransferred = 0;

    // Read 256 bytes at a time -> less overhead
    ssp1_locked = true;
    status = Storage::read(filename.c_str(),  buffer, sizeof(buffer), totalBytesRead, &bytesRead);
    ssp1_locked = false;
    //printf("size %d\n", bytesRead);
    if (status == FR_OK){
        if (bytesRead < 512)    eof_flag = 1;
    }
   totalBytesRead += bytesRead;

   // Send 32 bytes at a time (if possible) to MP3 DECODER
   while (bytesTransferred < bytesRead)
   {
       {
           if ( 1 == dreq_pin.read())    // FIFO can take at least 32 more bytes
           {
               adesto_ds(); // enable data transfer
               if (bytesRead - bytesTransferred < 32)  transmission_size = bytesRead - bytesTransferred;
                                                       transmission_size = 32;
               for (unsigned int i = 0; i < transmission_size; i++)
                   ssp0_exchange_byte(buffer[bytesTransferred + i]);
               adesto_cs(); // disable after every 32-byte transmission
               bytesTransferred += transmission_size;
           }
       }
   }
}

uint16_t getEndFillByte(){
    uint8_t d[4];
    uint16_t fillByte;
    // Set up base address register
    adesto_cs();
        {
            d[0] = ssp0_exchange_byte(0x02); // write
            d[1] = ssp0_exchange_byte(0x07); // base address register
            // FillByte is at 0x1E06
            d[2] = ssp0_exchange_byte(0x06);
            d[3] = ssp0_exchange_byte(0x1E);
        }
    adesto_ds();
    delay_ms(1);
    // Read from RAM register
    adesto_cs();
        {
            //reset
            d[0] = ssp0_exchange_byte(0x02); // read
            d[1] = ssp0_exchange_byte(0x06); // WRAM register
            d[2] = ssp0_exchange_byte(0x06);
            d[3] = ssp0_exchange_byte(0x1E);
        }
    adesto_ds();
    fillByte = d[2] + (d[3] << 8);
    return fillByte;
}

uint16_t sendSCI(uint8_t command, uint8_t address, uint16_t data)
{
    uint8_t d[4];
    adesto_cs();
            {
                d[0] = ssp0_exchange_byte(command); // write
                d[1] = ssp0_exchange_byte(address); // base address register
                d[2] = ssp0_exchange_byte(data >> 8);
                d[3] = ssp0_exchange_byte(data & 0xFF);
            }
    adesto_ds();
    return (d[2] << 8) + d[3];
}

void vplayMusic(void * pvParameters)
{
    unsigned int totalBytesRead = 0;
    uint16_t endFillByte = 0;
    uint16_t read_dat;
    bool eof_flag = 0;
    rst();
    cs_decoder.setAsOutput();
    dreq_pin.setAsInput();

    vTaskDelay(1000);
    uint8_t d[5];
    adesto_ds();
    adesto_cs();
    vTaskDelay(5);
    //sendSCI(0x02, 0x00, 0x4804); // soft reset
    sendSCI(0x02, 0x03, 0x6000); // reset, set clock
    sendSCI(0x02, 0x00, 0x4C00); // default was 0x4800, now also set SDISHARE
    sendSCI(0x02, 0x0B, 0x1010); // lower volume because I am on quiet floor
    read_dat = sendSCI(0x03, 0x00, 0x0000);
    printf("Returned data: %x\n", read_dat);
//    {
//        //reset
//        d[0] = spi1.transfer(0x02);
//        d[1] = spi1.transfer(0x03);
//        d[2] = spi1.transfer(0x00);
//        d[3] = spi1.transfer(0x60);
//    }
    adesto_ds();
    while(1)
       {
        //printf("inside music load\n");
        //if(xSemaphoreTake(spi_bus_lock, 1000)) {
           // Send full song to the decoder

           while (0 == eof_flag)
           {
               //printf("mp\n");
               delay_ms(11);
               if (cancel_song == false) loadSong(song_list[song_index], totalBytesRead, eof_flag);
               //printf("song playing: %s\n", listofsong[current_song]);
               else
               {
                  adesto_cs();
                  // set CANCEL bit
                  sendSCI(0x02, 0x00, 0x4C08);
                  printf("Song Paused\n");
                  // send more end-fill-bytes
                  bool cancel_cleared = 0;
                  while (0 == cancel_cleared)
                  {
                      adesto_ds();
                      for (int i=0; i < 36; i++)
                      {
                          d[5] = ssp0_exchange_byte(endFillByte);
                      }
                      adesto_cs();
                      read_dat = sendSCI(0x03, 0x00, 0x0000);
                      if ((read_dat & (1 << 3)) == 0)    cancel_cleared = 1;
                  }
                  endFillByte = getEndFillByte();
                  adesto_ds();
                  for (int i=0; i < 2052; i++)
                  {
                      d[5] = ssp0_exchange_byte(endFillByte);
                  }
                  adesto_cs();
                  cancel_song = false;

//                  printf("Playback Done!\n");
                  totalBytesRead = 0;
//                  eof_flag = 0;

                  //xSemaphoreGive(spi_bus_lock);
               }
               if(change_vol == true)
               {
                   uint16_t volume = (current_volume << 8) + current_volume;
                   sendSCI(0x02, 0x0B, volume);
               }
           }
           endFillByte = getEndFillByte();
           printf("end fill byte: %x\n", endFillByte);

           // send end-fill-bytes
           adesto_ds();
           for (int i=0; i < 2100; i++)
           {
               d[5] = ssp0_exchange_byte(endFillByte);
           }
           adesto_cs();
           // set CANCEL bit
           sendSCI(0x02, 0x00, 0x4C08);
           // send more end-fill-bytes
           bool cancel_cleared = 0;
           while (0 == cancel_cleared){
               adesto_ds();
               for (int i=0; i < 36; i++)
               {
                   d[5] = ssp0_exchange_byte(endFillByte);
               }
               adesto_cs();
               read_dat = sendSCI(0x03, 0x00, 0x0000);
               if ((read_dat & (1 << 3)) == 0)    cancel_cleared = 1;
           }

           printf("Playback Done!\n");
           switch(current_mode)
           {
               case 0: {
                           title_index = (title_index+1) % playlist_size;
                           song_index = title_index;
                           loadMainScreen();
                       }
                       break;
               case 2: {
                           srand (sys_get_uptime_ms());
                           title_index = rand() % playlist_size;
                           song_index = title_index;
                           loadMainScreen();
                       }
                       break;
           }
           totalBytesRead = 0;
           eof_flag = 0;
           //xSemaphoreGive(spi_bus_lock);
       }
       //vTaskDelay(1000);

}
//*****************************************************


bool is_playing = true;
uint8_t counter = 0;
void vTouch(void *p)
{
    GPIO touch_cs(P1_29);
    GPIO touch_irq(P2_1);
    Point pos;
    XPT2046A touch(touch_cs, touch_irq);

    touch_cs.setAsOutput();
    touch_irq.setAsInput();
    touch_cs.setHigh();

    while(1)
    {
            if(xSemaphoreTake(touchIREQ, portMAX_DELAY))
            {
            if (counter == 0) {
            //taskENTER_CRITICAL();
            while (ssp1_locked) vTaskDelay(1); // wait for fread to finish
            touch.getADCData(pos);
            if (pos.x > 10 && pos.x < 14 && pos.y == 14)  // next button
            {
                title_index = (title_index + 1) % playlist_size;
                react(do_next);
                loadMainScreen();
                song_index = (song_index + 1) % playlist_size;
                cancel_song = true;
                if (!is_playing)    {   vTaskResume(PlayerTask_handle);
                                        is_playing = true;
                                    }
            }
            else if (pos.x > 6 && pos.x < 9 && pos.y == 14)  // play button
            {
                if (is_playing) {   react(do_pause);
                                    vTaskSuspend(PlayerTask_handle);
                                }
                else            {   react(do_play);
                                    vTaskResume(PlayerTask_handle);
                                }
                is_playing = !is_playing;
            }
            else if (pos.x > 2 && pos.x < 5 && pos.y == 14)  // back button
            {
                title_index = (title_index + playlist_size - 1) % playlist_size;
                react(do_prev);
                loadMainScreen();
                song_index = (song_index + playlist_size - 1) % playlist_size;
                cancel_song = true;
                if (!is_playing)    {   vTaskResume(PlayerTask_handle);
                                        is_playing = true;
                                    }
            }
            else if (pos.x > 7 && pos.x < 10 && pos.y == 9)  // volume down
            {
                if(current_volume < 0xFE)
                            {
                                change_vol = true;
                                if(current_volume < 0xFE - VOLSTEP)     current_volume = current_volume + VOLSTEP;
                                else
                                    {                                   current_volume = 0xFE;
                                                                        react(do_volume_off);
                                    }
                            }
            }
            else if (pos.x > 11 && pos.x < 14 && pos.y == 9)  // volume up
            {
                if(current_volume > 0x00)
                {
                    if(current_volume == 0xFE)      react(do_volume_on);
                    change_vol = true;
                    if(current_volume > VOLSTEP)    current_volume = current_volume - VOLSTEP;
                    else                            current_volume = 0;
                }
            }
            else if (pos.x > 2 && pos.x < 5 && pos.y == 9)  // change mode
            {
                current_mode = (current_mode + 1) % 3;
                react(do_change_mode);
            }
            printf("x: %d, y: %d\n", pos.x, pos.y);
            //vTaskDelay(500); // This sleeps the task for 100ms (because 1 RTOS tick = 1 millisecond)
            NVIC_ClearPendingIRQ(EINT3_IRQn);

            //taskEXIT_CRITICAL();
            }
            vTaskDelay(200);
            counter = (counter + 1) % 2;
            LPC_GPIOINT->IO2IntEnF |= (1 << 1); // INT for P2.1

            }
    }
}

// You can comment out the sample code of lpc1758_freertos project and run this code instead
int main(int argc, char const *argv[])
{
    backlight_pin.setAsOutput();
    backlight_pin.setHigh();
    ssp1_init();
    ssp0_init(1);

    ssp_set_max_clock(LPC_SSP1, 2);
    ssp_set_max_clock(LPC_SSP0, 2);
//
//    LPC_PINCON->PINSEL0 &= ~(3 << 12);      // set pin 0.6 as GPIO
//    LPC_GPIO0->FIODIR |= (1 << 6);          // set pin 0.6 as output (CS of Flash)
//    LPC_PINCON->PINSEL3 &= ~(3 << 18);      // set pin 1.25 as GPIO
//    LPC_GPIO1->FIODIR |= (1 << 25);         // set pin 1.25 as output (CS of SDcard)
//    LPC_PINCON->PINSEL1 &= ~(3 << 0);      // set pin 0.16 as GPIO
//    LPC_GPIO0->FIODIR |= (1 << 16);         // set pin 0.16 as output (CS of wifi)
////    // Deselect both SSP0 & SSP1 peripherals (all CS are Active LOW)
//    LPC_GPIO0->FIOSET |= (1 << 6);  // 0.6
//    LPC_GPIO1->FIOSET |= (1 << 25); // 1.25
//    LPC_GPIO0->FIOSET |= (1 << 16); // 0.16
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    song_list = get_song_list(playlist_size);
    lcd.LCD_reset();

    lcd.fillScreen(Black);
    cs_pin.setHigh();
    loadMainScreen();


    /// This "stack" memory is enough for each task to run properly (512 * 32-bit) = 2Kbytes stack
    /// const uint32_t STACK_SIZE_WORDS = 512;

    /**
     * Observe and explain the following scenarios:
     *
     * 1) Same Priority: A = 1, B = 1
     * 2) Different Priority: A = 2, B = 1
     * 3) Different Priority: A = 1, B = 2
     *
     * Turn in screen shots of what you observed
     * as well as an explanation of what you observed
     */
    LPC_GPIOINT->IO2IntEnF |= (1 << 1); // INT for P2.1
    LPC_GPIOINT->IO2IntEnR &= ~(1 << 1); // INT for P2.1

//    isr_register(EINT3_IRQn, interrupt_handler);
//    NVIC_EnableIRQ(EINT3_IRQn);

//    xTaskCreate(vplayMusic, "devid", 2048, NULL, PRIORITY_MEDIUM, &PlayerTask_handle);
//    xTaskCreate(vTouch, "Touch", 512, ( void * ) 1, PRIORITY_HIGH, &LCD_handle/* Fill in the rest parameters for this task */ );

    /* Start Scheduler - This will not return, and your tasks will start to run their while(1) loop */
    //vTaskStartScheduler();
//    scheduler_start();

    return 0;
}
