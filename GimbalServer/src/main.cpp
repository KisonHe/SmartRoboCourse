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

nvs_handle my_handle;

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#include <lvgl.h>
#include <TFT_eSPI.h>

#include "lvgl_calib_tab.h"
#include "lvgl_info_tab.h"
SemaphoreHandle_t LVGL_Semaphore;
SemaphoreHandle_t infoTab_wait_Semaphore;


extern int calib_x;
extern int calib_y;

int servo_x = 0;
int servo_y = 0;

int16_t Positions[6][2];
uint8_t NVSHasData = 255;

int ScreenInit();
int aimHandle(char *command, int length = 0);

void lv_main_tabview_init();
lv_obj_t *mainTabView = nullptr;


void lv_ex_btnmatrix_1(lv_obj_t * parent);

void updateServo(){

}


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
    
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    int i = 0;
    for (i = 0;i<6*2;i++){
      char key[3];
      key[0] = '0' + i/2;
      key[1] = i%2 ? 'x' : 'y';
      key[2] = 0;
      err = nvs_get_i16(my_handle, key, &Positions[i/2][i%2]);
      if (err != ESP_OK){
          Serial.println("Something is **** in NVS");
          Serial.print(esp_err_to_name(err));
          NVSHasData = 250;
          break;
      }
    }
    int sum = 0;
    if (i == 12 && NVSHasData != 250){
      for (int i = 0;i < 6;i++)
        for (int j = 0;j < 2;j++)
          sum += Positions[i][j];
      if (sum ==0){
        Serial.println("NVS has no data");
          NVSHasData = 250;
      } else {
        Serial.println("NVS has data found");
        NVSHasData = 1;
      }
    }
    
    LVGL_Semaphore = xSemaphoreCreateBinary();
    infoTab_wait_Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(infoTab_wait_Semaphore);xSemaphoreTake(infoTab_wait_Semaphore, portMAX_DELAY); 
    xSemaphoreGive(LVGL_Semaphore);xSemaphoreTake(LVGL_Semaphore, portMAX_DELAY);    
    ScreenInit();
    lv_main_tabview_init();
    xSemaphoreGive(LVGL_Semaphore);   

}

void loop()
{
    xSemaphoreTake(LVGL_Semaphore,portMAX_DELAY);
    lv_info_real_time_data_label_update(infoTabRealTimeDataLabel);
    lv_task_handler(); /* let the GUI do its work */
    xSemaphoreGive(LVGL_Semaphore);
    updateServo();
    int incoming = Serial.available();
    if (incoming)
    {
      char* buffer = nullptr;
      if (incoming < 100)
        buffer = new char[incoming + 1];
      memset(buffer,0,incoming + 1);
      Serial.readBytes(buffer,incoming);
      aimHandle(buffer,incoming);
      delete buffer;
    }
    else
      aimHandle(nullptr);

    vTaskDelay(1);
}




void lv_main_tabview_init(){
    lv_obj_t *mainTabView = lv_tabview_create(lv_scr_act(), NULL);

    lv_obj_t *infoTab = lv_tabview_add_tab(mainTabView, "Info");
    lv_obj_t *calibTab = lv_tabview_add_tab(mainTabView, "Calibration");
    lv_info_tab_init(infoTab);
    lv_calib_tab_init(calibTab);
}


int aimHandle(char * command, int length) {
  // there are two modes,
  // the switch on the info page,
  // if choose not to wait for confirm
  // we delay time t and aim the next target.
  // else, we wait for user to press button
  // and user might clicked the button twice,
  // so.. for a certain time t, ignore the signal from the button.
  // also, there is a chance user try to changes mode during no-wait mode.
  // to fix that, make the switch not clickble and labels message when no-wait
  // mode is fired
  static bool wait = false;
  static bool busy = false;
  static uint32_t time = 0;
  static int8_t step = 0;
  static const uint32_t interval = 3500; // wait servo for a duration for servo to get to position
  //0 Not Busy, 1 on handling order command
  static uint8_t order[6];
  if (!busy){
    if (command != nullptr){
      if (length >= 7 && command[0] == 'O' && command[7] == 'E'){
        if (NVSHasData != 1){
          //FIXME pop up message to remind that we have no data
          Serial.print("Got Order Command but NVS has no data");
          return -1;
        }
        // Confirm Move Command
          for (int i = 0; i < 6; i++){
            if (command[i+1] - '0' > 6){
              Serial.println("!!!Invalid Command!!!");
              return -2;
            }
            order[i] = command[i+1] - '0';
          }
          lv_obj_set_click(infotabModeSwitch,false);
          wait = lv_switch_get_state(infotabModeSwitch);
          if (wait){
            lv_infotab_continue_msgbox_init(infoTabMainCont);
          }
          busy = true;
          step = 0;
          time = xTaskGetTickCount();
          
          servo_x = Positions[order[step]][0];
          servo_y = Positions[order[step]][1];
      }
      else {
        // check frame 
        // if frame is move command
        // handle the direct move command
        
        // static int32_t sumX = 0, sumY = 0;
        
        // The system is open loop,
        // From the experience from RM, we know we should 
        // add the command received onto the current value instead of 
        // target value. But open loop we dont know the current value
        // so we divide the target value by factor. The PID can still 
        // get position

        // deFrame and set servo_x, servo_y
      }
    }
  }
  else{
    if (wait){
      if (xSemaphoreTake(infoTab_wait_Semaphore, 0) == pdTRUE){
        step++;
        lv_infotab_continue_msgbox_init(infoTabMainCont);
        if (step >= 6){
          busy = false;
          time = 0;
          lv_obj_set_click(infotabModeSwitch,true);
          return 0;
        }
        servo_x = Positions[order[step]][0];
        servo_y = Positions[order[step]][1];
      }
      else
        return 2;
      return 3;
    }
    else{
      if (xTaskGetTickCount() - time > interval){
        Serial.print("TickCount:");
        Serial.println(xTaskGetTickCount());
        Serial.print("time:");
        Serial.println(time);
        time = xTaskGetTickCount();
        step++;
        if (step >= 6){
          busy = false;
          time = 0;
          lv_obj_set_click(infotabModeSwitch,true);
          return 0;
        }
        time = xTaskGetTickCount();
        servo_x = Positions[order[step]][0];
        servo_y = Positions[order[step]][1];
      }
      return 1;
    }
    // running the order command
    
  }
  return 0;
}

