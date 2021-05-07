#include <Arduino.h>
#include <lvgl.h>
#include "lvgl_calib_tab.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

extern nvs_handle my_handle;

lv_obj_t *calib_hint_label = nullptr;
lv_obj_t *calib_xValue_label = nullptr;
lv_obj_t *calib_yValue_label = nullptr;
lv_obj_t *calibTabMainCont = nullptr;
lv_obj_t *CalibLEDs[6];

int calib_x = 0;
int calib_y = 0;
extern int servo_x;
extern int servo_y;
calib_status_e calib_status = notStarted;

void lv_calib_tab_init(lv_obj_t *view)
{
  calibTabMainCont = lv_cont_create(view, NULL);
  lv_obj_set_auto_realign(calibTabMainCont, true);                   /*Auto realign when the size changes*/
  lv_obj_align_origo(calibTabMainCont, NULL, LV_ALIGN_CENTER, 0, 0); /*This parametrs will be sued when realigned*/
  lv_cont_set_fit(calibTabMainCont, LV_FIT_PARENT);
  lv_cont_set_layout(calibTabMainCont, LV_LAYOUT_OFF);

  lv_calibCtrl_btnmatrix_init(calibTabMainCont);
  lv_calib_leds_init(calibTabMainCont, CalibLEDs);
  lv_calib_label_init(calibTabMainCont, calib_hint_label, calib_xValue_label, calib_yValue_label);
}
static void lv_clear_nvs_msgbox_init(lv_obj_t *parent);
static void lv_save_nvs_msgbox_init(lv_obj_t *parent);

static int16_t savePositions[6][2];

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_PRESSED)
  {
    // const char * txt = lv_btnmatrix_get_active_btn_text(obj);
    // printf("%s was pressed\n", txt);
    int id = lv_btnmatrix_get_active_btn(obj);
    printf("%d is pressed\n", id);
    switch (id)
    {
    case 0:
      calib_status = notStarted;
      lv_label_set_text(calib_hint_label, "Press Save Button to start Calibrating");
      for (int i = 0; i < 6; i++)
        lv_led_off(CalibLEDs[i]);

      break;
    case 1:
      calib_y += 1;
      break;
    case 2:
      lv_clear_nvs_msgbox_init(calibTabMainCont);
      break;
    case 3:
      calib_x -= 1;
      break;
    case 4:
      // calib_x -= 1;
      if (calib_status == notStarted)
      {
        calib_status = P1;
        lv_led_on(CalibLEDs[0]);
        lv_label_set_text(calib_hint_label, "Calibrating Started");
      }
      else if (calib_status != calibFinish)
      {
        savePositions[int(calib_status) - 1][0] = calib_x;
        savePositions[int(calib_status) - 1][1] = calib_y;
        lv_led_off(CalibLEDs[(int)(calib_status - 1)]);
        if (calib_status != P6)
        {
          lv_led_on(CalibLEDs[(int)(calib_status)]);
        }
        else
        {
          vTaskDelay(500);
          lv_save_nvs_msgbox_init(calibTabMainCont);
          calib_status = calibFinish;
          lv_label_set_text(calib_hint_label, "#00ffe5 Calibration Finished Press Restart to Restart#");
          break;
        }
        calib_status = (calib_status_e)(calib_status + 1);
      }
      break;
    case 5:
      calib_x += 1;
      break;
    case 6:
      if (calib_status != notStarted && calib_status != P1 && calib_status != calibFinish)
      {
        lv_led_off(CalibLEDs[(int)(calib_status - 1)]);
        lv_led_on(CalibLEDs[(int)(calib_status - 2)]);
        calib_status = calib_status_e(calib_status - 1);
      }
      break;
    case 7:
      calib_y -= 1;
      break;

    default:
      break;
    }
    lv_label_set_text_fmt(calib_xValue_label, "X: %d", calib_x);
    lv_label_set_text_fmt(calib_yValue_label, "Y: %d", calib_y);
  }
  else if (event == LV_EVENT_LONG_PRESSED_REPEAT)
  {
    // const char * txt = lv_btnmatrix_get_active_btn_text(obj);
    // printf("%s was pressed\n", txt);
    int id = lv_btnmatrix_get_active_btn(obj);
    printf("%d is long pressed\n", id);
    switch (id)
    {
    case 1:
      calib_y += 3;
      break;
    case 3:
      calib_x -= 3;
      break;
    case 5:
      calib_x += 3;
      break;
    case 7:
      calib_y -= 3;
      break;

    default:
      break;
    }
    lv_label_set_text_fmt(calib_xValue_label, "X: %d", calib_x);
    lv_label_set_text_fmt(calib_yValue_label, "Y: %d", calib_y);
  }
  if (calib_status != notStarted && calib_status != calibFinish)
  {
    servo_x = calib_x;
    servo_y = calib_y;
  }
}

static const char *btnm_map[] = {LV_SYMBOL_REFRESH, LV_SYMBOL_UP, LV_SYMBOL_TRASH, "\n",
                                 LV_SYMBOL_LEFT, LV_SYMBOL_SAVE, LV_SYMBOL_RIGHT, "\n",
                                 LV_SYMBOL_PREV, LV_SYMBOL_DOWN, LV_SYMBOL_CLOSE, ""};

void lv_calibCtrl_btnmatrix_init(lv_obj_t *parent)
{
  lv_obj_t *btnm1 = lv_btnmatrix_create(parent, NULL);
  lv_btnmatrix_set_map(btnm1, btnm_map);

  // lv_btnmatrix_set_btn_ctrl(btnm1,2,LV_BTNMATRIX_CTRL_HIDDEN);
  lv_obj_align(btnm1, NULL, LV_ALIGN_IN_LEFT_MID, 5, 0);
  lv_obj_set_event_cb(btnm1, event_handler);
  lv_obj_set_size(btnm1, 160, 130);
}

void lv_calib_label_init(lv_obj_t *parent, lv_obj_t *&label, lv_obj_t *&vxlabel, lv_obj_t *&vylabel)
{
  label = lv_label_create(parent, NULL);
  vxlabel = lv_label_create(parent, NULL);
  vylabel = lv_label_create(parent, NULL);
  // lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);     /*Break the long lines*/
  lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
  lv_label_set_recolor(label, true);                /*Enable re-coloring by commands in the text*/
  lv_label_set_align(label, LV_LABEL_ALIGN_CENTER); /*Center aligned lines*/

  lv_obj_set_width(label, 120);
  lv_label_set_text(label, "Press Save Button to start Calibrating");
  lv_label_set_text_fmt(vxlabel, "X: %d", calib_x);
  lv_label_set_text_fmt(vylabel, "Y: %d", calib_x);
  // lv_obj_set_width(label, 150);
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 80, 20);
  lv_obj_align(vxlabel, NULL, LV_ALIGN_CENTER, 40, 50);
  lv_obj_align(vylabel, NULL, LV_ALIGN_CENTER, 100, 50);
}

void lv_calib_leds_init(lv_obj_t *parent, lv_obj_t **leds)
{
  static lv_style_t style_led;

  /*Create a simple button style*/
  lv_style_init(&style_led);
  lv_style_set_bg_color(&style_led, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_style_set_bg_grad_color(&style_led, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_style_set_shadow_color(&style_led, LV_STATE_DEFAULT, LV_COLOR_LIME);
  // lv_style_set_bg_grad_dir(&style_led, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  for (int i = 0; i < 6; i++)
  {
    leds[i] = lv_led_create(parent, NULL);
    lv_obj_set_size(leds[i], 20, 20);
    lv_obj_add_style(leds[i], LV_LED_PART_MAIN, &style_led);
    lv_led_off(leds[i]);
  }
  lv_obj_align(leds[0], NULL, LV_ALIGN_CENTER, 40, -60);
  lv_obj_align(leds[1], NULL, LV_ALIGN_CENTER, 80, -60);
  lv_obj_align(leds[2], NULL, LV_ALIGN_CENTER, 120, -60);
  lv_obj_align(leds[3], NULL, LV_ALIGN_CENTER, 40, -30);
  lv_obj_align(leds[4], NULL, LV_ALIGN_CENTER, 80, -30);
  lv_obj_align(leds[5], NULL, LV_ALIGN_CENTER, 120, -30);
}

static void lv_clear_nvs_msgbox_init(lv_obj_t *parent)
{
  static const char *btns[] = {"Yes", "No", ""};

  lv_obj_t *mbox1 = lv_msgbox_create(parent, NULL);
  lv_msgbox_set_text(mbox1, "Confirm to Clear Data in NVS?");
  lv_msgbox_add_btns(mbox1, btns);
  lv_obj_set_width(mbox1, 200);
  lv_obj_set_event_cb(mbox1, [](lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_PRESSED)
    {
      if (lv_msgbox_get_active_btn(obj) == 0)
      {
        //FIXME Clear NVS data here
        Serial.print("Clearing NVS Data");
        nvs_flash_erase();
        ESP.restart();
        // nvs_flash_init();
        calib_status = notStarted;
        lv_label_set_text(calib_hint_label, "Press Save Button to start Calibrating");
        for (int i = 0; i < 6; i++)
          lv_led_off(CalibLEDs[i]);
        lv_msgbox_start_auto_close(obj, 0);
      }
      else if (lv_msgbox_get_active_btn(obj) == 1)
      {
        Serial.print("Cancel Clear NVS Data");
        lv_msgbox_start_auto_close(obj, 0);
      }
    }
  });
  lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); /*Align to the corner*/
}

static void lv_save_nvs_msgbox_init(lv_obj_t *parent)
{
  static const char *btns[] = {"Yes", "No", ""};

  lv_obj_t *mbox1 = lv_msgbox_create(parent, NULL);
  lv_msgbox_set_text(mbox1, "Confirm to Save Data to NVS?");
  lv_msgbox_add_btns(mbox1, btns);
  lv_obj_set_width(mbox1, 200);
  lv_obj_set_event_cb(mbox1, [](lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_PRESSED)
    {
      if (lv_msgbox_get_active_btn(obj) == 0)
      {
        //FIXME Clear NVS data here
        Serial.print("Save Data to NVS");
        for (int i = 0; i < 6 * 2; i++)
        {
          esp_err_t err;
          char key[3];
          key[0] = '0' + i / 2;
          key[1] = i % 2 ? 'x' : 'y';
          key[2] = 0;
          err = nvs_set_i16(my_handle, key, savePositions[i / 2][i % 2]);
          // Serial.print("Writing ");
          // Serial.print(i / 2);Serial.print(" / ");Serial.print(i % 2);Serial.print("data of ");Serial.println(savePositions[i / 2][i % 2]);
        }
        //To avoid reloading info tab Etc,
        //Just reboot here
        ESP.restart();
        lv_msgbox_start_auto_close(obj, 0);
      }
      else if (lv_msgbox_get_active_btn(obj) == 1)
      {
        Serial.print("Cancel Save Data to NVS");
        lv_msgbox_start_auto_close(obj, 0);
      }
    }
  });
  lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); /*Align to the corner*/
}