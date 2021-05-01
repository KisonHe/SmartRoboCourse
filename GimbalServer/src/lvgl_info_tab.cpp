#include <Arduino.h>
#include <lvgl.h>
#include "lvgl_info_tab.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

extern SemaphoreHandle_t infoTab_wait_Semaphore;
extern int servo_x;
extern int servo_y;
extern uint8_t NVSHasData;
extern int16_t Positions[6][2];


lv_obj_t *infoTabMainCont = nullptr;
lv_obj_t *infoTabHintLabel = nullptr;
lv_obj_t *infoTabNVSDataLabel[6];
lv_obj_t *infoTabRealTimeDataLabel[2];
lv_obj_t * infotabModeSwitch = nullptr;

void lv_info_hint_label_init(lv_obj_t *parent, lv_obj_t *&label);
void lv_info_nvs_data_label_init(lv_obj_t *parent, lv_obj_t **labels);
void lv_infotab_mode_switch_init(lv_obj_t * parent, lv_obj_t *& sw);
void lv_info_real_time_data_label_init(lv_obj_t *parent, lv_obj_t **labels);

void lv_info_tab_init(lv_obj_t* view){
    infoTabMainCont = lv_cont_create(view, NULL);
    lv_obj_set_auto_realign(infoTabMainCont, true);                    /*Auto realign when the size changes*/
    lv_obj_align_origo(infoTabMainCont, NULL, LV_ALIGN_CENTER, 0, 0);  /*This parametrs will be sued when realigned*/
    lv_cont_set_fit(infoTabMainCont, LV_FIT_PARENT);
    lv_cont_set_layout(infoTabMainCont, LV_LAYOUT_OFF);
    lv_info_hint_label_init(infoTabMainCont, infoTabHintLabel);
    lv_infotab_mode_switch_init(infoTabMainCont, infotabModeSwitch);
    lv_info_real_time_data_label_init(infoTabMainCont,infoTabRealTimeDataLabel);

}

void lv_info_hint_label_init(lv_obj_t *parent, lv_obj_t *&label)
{
  label = lv_label_create(parent, NULL);
  lv_label_set_recolor(label, true);                /*Enable re-coloring by commands in the text*/
  lv_label_set_align(label, LV_LABEL_ALIGN_CENTER); /*Center aligned lines*/
  // lv_label_set_text(label, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label "
  //                           "and  wrap long text automatically.");

  lv_obj_set_width(label, 280);
  if (NVSHasData == 1){
    lv_label_set_text(label, "The NVS has Calibrated data");
    lv_info_nvs_data_label_init(parent,infoTabNVSDataLabel);
  }
  else{
   lv_label_set_text(label, "The NVS has #ff1234 No# data "
   "Please go and calibrate, or OpenCV side Use follow mode"); 
  }
  lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, -60, -50);
}


void lv_info_nvs_data_label_init(lv_obj_t *parent, lv_obj_t **labels){
  for (int i = 0;i < 6;i++){
    labels[i] = lv_label_create(parent, NULL);
    lv_label_set_text_fmt(labels[i], "%d,%d", Positions[i][0],Positions[i][1]);
  }
  lv_obj_align(labels[0], NULL, LV_ALIGN_CENTER, -120, 0);
  lv_obj_align(labels[1], NULL, LV_ALIGN_CENTER, -60, 0);
  lv_obj_align(labels[2], NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(labels[3], NULL, LV_ALIGN_CENTER, -120, 40);
  lv_obj_align(labels[4], NULL, LV_ALIGN_CENTER, -60, 40);
  lv_obj_align(labels[5], NULL, LV_ALIGN_CENTER, 0, 40);
}

void lv_info_real_time_data_label_init(lv_obj_t *parent, lv_obj_t **labels){
  labels[0] = lv_label_create(parent, NULL);
  labels[1] = lv_label_create(parent, NULL);
  lv_label_set_text_fmt(labels[0], "ServoX:%d", servo_x);
  lv_label_set_text_fmt(labels[1], "ServoY:%d", servo_y);

  lv_obj_align(labels[0], NULL, LV_ALIGN_CENTER, 80, 40);
  lv_obj_align(labels[1], NULL, LV_ALIGN_CENTER, 80, 60);
}

void lv_info_real_time_data_label_update(lv_obj_t **labels){
  lv_label_set_text_fmt(labels[0], "ServoX:%d", servo_x);
  lv_label_set_text_fmt(labels[1], "ServoY:%d", servo_y);
}


static void infotab_mode_switch_event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED) {
        printf("State: %s\n", lv_switch_get_state(obj) ? "On" : "Off");
    }
}

void lv_infotab_mode_switch_init(lv_obj_t * parent, lv_obj_t *& sw)
{
    /*Create a switch and apply the styles*/
    sw = lv_switch_create(parent, NULL);
    lv_obj_align(sw, NULL, LV_ALIGN_CENTER, 100, -30);
    lv_obj_set_event_cb(sw, infotab_mode_switch_event_handler);
    lv_obj_t *label = lv_label_create(parent, NULL);
    lv_label_set_text(label, "Wait?");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 100, 0);
}

void lv_infotab_continue_msgbox_init(lv_obj_t *parent)
{
  static const char *btns[] = {"Yes", ""};

  lv_obj_t *mbox1 = lv_msgbox_create(parent, NULL);
  lv_msgbox_set_text(mbox1, "Continue?");
  lv_msgbox_add_btns(mbox1, btns);
  lv_obj_set_width(mbox1, 200);
  lv_obj_set_event_cb(mbox1, [](lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_PRESSED)
    {
      if (lv_msgbox_get_active_btn(obj) == 0)
      {
        xSemaphoreGive(infoTab_wait_Semaphore);
        lv_msgbox_start_auto_close(obj, 0);
      }
    }
  });
  lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); /*Align to the corner*/
}