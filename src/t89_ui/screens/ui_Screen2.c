// Screen 2 - Hello World Screen

#include "ui.h"

lv_obj_t * ui_Screen2;
lv_obj_t * ui_dashboardbutton = NULL;
lv_obj_t * ui_Screen2_clutch_btn = NULL;
lv_obj_t * ui_Screen2_cooling_btn = NULL;
lv_obj_t * ui_Screen2_settings_btn = NULL;
//lv_obj_t * ui_Screen2_BackBtn;
// lv_obj_t * ui_Screen2_Label;

void ui_Screen2_screen_init(void)
{
    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui_Screen2, &ui_img_1878767743, LV_PART_MAIN | LV_STATE_DEFAULT);

    // // Hello World Label
    // ui_Screen2_Label = lv_label_create(ui_Screen2);
    // lv_obj_set_width(ui_Screen2_Label, LV_SIZE_CONTENT);
    // lv_obj_set_height(ui_Screen2_Label, LV_SIZE_CONTENT);
    // lv_obj_set_x(ui_Screen2_Label, 0);
    // lv_obj_set_y(ui_Screen2_Label, 0);
    // lv_obj_set_align(ui_Screen2_Label, LV_ALIGN_CENTER);
    // lv_label_set_text(ui_Screen2_Label, "Hello World");
    // lv_obj_set_style_text_color(ui_Screen2_Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_Screen2_Label, &ui_font_inkfree40, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_dashboardbutton = ui_dashboardbutton_create(ui_Screen2);
    lv_obj_set_pos(ui_dashboardbutton, 0, 0);

    // Title
    ui_screen_title_create(ui_Screen2, "MENU");

    // Clutch settings button
    ui_Screen2_clutch_btn = ui_settingsbutton_create(ui_Screen2);
    lv_obj_set_pos(ui_Screen2_clutch_btn, -200,0);
    lv_obj_add_event_cb(ui_Screen2_clutch_btn, ui_event_settingsbutton_to_screen3, LV_EVENT_CLICKED, NULL);
    lv_obj_t * label_clutch = lv_label_create(ui_Screen2_clutch_btn);
    lv_label_set_text(label_clutch, "CLUTCH");
    lv_obj_center(label_clutch);

    // Cooling settings button
    ui_Screen2_cooling_btn = ui_settingsbutton_create(ui_Screen2);
    lv_obj_set_pos(ui_Screen2_cooling_btn, 0, 0);
    lv_obj_add_event_cb(ui_Screen2_cooling_btn, ui_event_settingsbutton_to_screen4, LV_EVENT_CLICKED, NULL);
    lv_obj_t * label_cooling = lv_label_create(ui_Screen2_cooling_btn);
    lv_label_set_text(label_cooling, "COOLING");
    lv_obj_center(label_cooling);

    // Settings button
    ui_Screen2_settings_btn = ui_settingsbutton_create(ui_Screen2);
    lv_obj_set_pos(ui_Screen2_settings_btn, 200, 0);
    lv_obj_add_event_cb(ui_Screen2_settings_btn, ui_event_settingsbutton_to_screen5, LV_EVENT_CLICKED, NULL);
    lv_obj_t * label_settings = lv_label_create(ui_Screen2_settings_btn);
    lv_label_set_text(label_settings, "SETTINGS");
    lv_obj_center(label_settings);
}

void ui_Screen2_screen_destroy(void)
{
    lv_obj_del(ui_Screen2);
    ui_Screen2 = NULL;
    ui_dashboardbutton = NULL;
    ui_Screen2_clutch_btn = NULL;
    ui_Screen2_cooling_btn = NULL;
    ui_Screen2_settings_btn = NULL;
}
