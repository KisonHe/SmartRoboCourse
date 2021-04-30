#define FS_NO_GLOBALS
#include <FS.h>
#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
extern TFT_eSPI tft;         // Invoke custom library

#include <lvgl.h>
#include <TFT_eSPI.h>

static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

// static lv_style_t style1;

/*Read the touchpad*/
static bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY, 600);
    if(!touched) {
      data->state = LV_INDEV_STATE_REL;
    } else {
      data->state = LV_INDEV_STATE_PR;
      /*Set the coordinates*/
      data->point.x = touchX; data->point.y = touchY;
  
    //   Serial.print("Data x"); Serial.println(touchX);
    //   Serial.print("Data y"); Serial.println(touchY);
    }
    return false; /*Return `false` because we are not buffering and no more data to read*/
}


/* Display flushing */
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}


int ScreenInit()
{
    tft.begin();
    tft.setTextColor(0xFFFF, 0x0000);
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);
    tft.setSwapBytes(true); 
    lv_init();
    uint16_t calData[5] = { 376, 3526, 288, 3524, 7 };
    tft.setTouch(calData);
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the input device driver*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);  

    return 0;

}

