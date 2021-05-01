#ifndef LVGL_INFO_TAB_H
#define LVGL_INFO_TAB_H

extern lv_obj_t *infoTabNVSDataLabel[6];
extern lv_obj_t *infoTabRealTimeDataLabel[2];
extern lv_obj_t * infotabModeSwitch;
extern lv_obj_t *infoTabMainCont;
extern lv_obj_t *infoTabHintLabel;

void lv_info_tab_init(lv_obj_t* view);
void lv_info_real_time_data_label_update(lv_obj_t **labels);
void lv_infotab_continue_msgbox_init(lv_obj_t *parent);
#endif