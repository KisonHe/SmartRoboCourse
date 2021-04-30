#define FS_NO_GLOBALS
#include <FS.h>
#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#include <lvgl.h>
#include <TFT_eSPI.h>

#include "lvgl_calib_tab.h"

SemaphoreHandle_t LVGL_Semaphore;


int calib_x = 0;
int calib_y = 0;

int8_t Positions[6][2];
uint8_t NVSHasData = 255;

int ScreenInit();

void lv_info_tab_init(lv_obj_t* view);
void lv_main_tabview_init();
lv_obj_t *mainTabView = nullptr;


void lv_ex_btnmatrix_1(lv_obj_t * parent);


void setup()
{
    Serial.begin(115200);
    memset(Positions,0,sizeof(Positions));
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    int i = 0;
    for (i = 0;i<6*2;i++){
      char key[3];
      key[0] = '0' + i/2;
      key[1] = i%2 ? 'x' : 'y';
      key[2] = 0;
      err = nvs_get_i8(my_handle, key, &Positions[i/2][i%2]);
      if (err != ESP_OK){
          Serial.println("NVS has no data");
          NVSHasData = 250;
          break;
      }
    }
    if (i == 12 && NVSHasData != 250){
      Serial.println("NVS has data found");
      NVSHasData = 1;
    }
    
    LVGL_Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(LVGL_Semaphore);
    
    
    xSemaphoreTake(LVGL_Semaphore, portMAX_DELAY);    
    ScreenInit();
    lv_main_tabview_init();
    xSemaphoreGive(LVGL_Semaphore);   

}

void loop()
{
    xSemaphoreTake(LVGL_Semaphore,portMAX_DELAY);
    lv_task_handler(); /* let the GUI do its work */
    xSemaphoreGive(LVGL_Semaphore);
    vTaskDelay(5);
}




void lv_main_tabview_init(){
    lv_obj_t *mainTabView = lv_tabview_create(lv_scr_act(), NULL);

    lv_obj_t *infoTab = lv_tabview_add_tab(mainTabView, "Info");
    lv_obj_t *calibTab = lv_tabview_add_tab(mainTabView, "Calibration");
    lv_info_tab_init(infoTab);
    lv_calib_tab_init(calibTab);

    calibTabMainCont = lv_cont_create(calibTab, NULL);
    lv_obj_set_auto_realign(calibTabMainCont, true);                    /*Auto realign when the size changes*/
    lv_obj_align_origo(calibTabMainCont, NULL, LV_ALIGN_CENTER, 0, 0);  /*This parametrs will be sued when realigned*/
    lv_cont_set_fit(calibTabMainCont, LV_FIT_PARENT);
    lv_cont_set_layout(calibTabMainCont, LV_LAYOUT_OFF);

    lv_calibCtrl_btnmatrix_init(calibTabMainCont);
    lv_calib_leds_init(calibTabMainCont,CalibLEDs);
    lv_calib_label_init(calibTabMainCont, calib_hint_label, calib_xValue_label, calib_yValue_label);
}

void lv_info_tab_init(lv_obj_t* view){

}
