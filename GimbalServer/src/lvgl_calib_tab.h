#ifndef LVGL_CALIB_TAB_H
#define LVGL_CALIB_TAB_H
void lv_calib_tab_init(lv_obj_t* view);
void lv_calibCtrl_btnmatrix_init(lv_obj_t * parent);

void lv_calib_leds_init(lv_obj_t * parent,lv_obj_t ** leds);
extern lv_obj_t * CalibLEDs[6];

void lv_calib_label_init(lv_obj_t * parent, lv_obj_t *& label, lv_obj_t *& vxlabel, lv_obj_t *& vylabel);
extern lv_obj_t * calib_hint_label;
extern lv_obj_t * calib_xValue_label;
extern lv_obj_t * calib_yValue_label;
extern lv_obj_t * calibTabMainCont;

enum calib_status_e {notStarted = 0, P1 = 1, P2 = 2, P3 = 3, P4 = 4, P5 = 5, P6 = 6, calibFinish = 7};
extern calib_status_e calib_status; 
#endif