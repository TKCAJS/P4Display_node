// Screen 2 - Hello World Screen

#include "ui.h"

lv_obj_t * ui_Screen2;
lv_obj_t * ui_dashboardbutton = NULL;
lv_obj_t * ui_Screen2_clutch_btn = NULL;
lv_obj_t * ui_Screen2_cooling_btn = NULL;
lv_obj_t * ui_Screen2_settings_btn = NULL;
lv_obj_t * ui_Screen2_test_btn = NULL;
lv_obj_t * ui_Screen2_rpm2_btn = NULL;
lv_obj_t * ui_Screen2_rpm3_btn = NULL;
//lv_obj_t * ui_Screen2_BackBtn;
// lv_obj_t * ui_Screen2_Label;

// Every menu entry is the same ui_settingsbutton template with a centred caption
// and a screen-change callback — only the position, caption and target differ.
static lv_obj_t * ui_menu_entry_create(int32_t x, int32_t y, const char * text, lv_event_cb_t on_click)
{
    lv_obj_t * btn = ui_settingsbutton_create(ui_Screen2);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

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

    // Menu buttons. ui_settingsbutton_create() aligns to LV_ALIGN_CENTER, so the
    // positions here are offsets from screen centre. More than four entries no
    // longer fit on one row (five 160-wide buttons already need more than the
    // 800 px), so they sit 3 + 3 on two rows: the 180 px x pitch spans 520 of
    // the 800 px, and the 120 px y pitch spans 220 of the 480 px with a 20 px
    // gap between the rows.
    ui_Screen2_clutch_btn = ui_menu_entry_create(-180, -60, "CLUTCH", ui_event_settingsbutton_to_screen3);
    ui_Screen2_cooling_btn = ui_menu_entry_create(0, -60, "COOLING", ui_event_settingsbutton_to_screen4);
    ui_Screen2_settings_btn = ui_menu_entry_create(180, -60, "SETTINGS", ui_event_settingsbutton_to_screen5);
    ui_Screen2_test_btn = ui_menu_entry_create(-180, 60, "TEST", ui_event_settingsbutton_to_screen6);
    ui_Screen2_rpm2_btn = ui_menu_entry_create(0, 60, "RPM 2", ui_event_settingsbutton_to_screen7);
    ui_Screen2_rpm3_btn = ui_menu_entry_create(180, 60, "RPM 3", ui_event_settingsbutton_to_screen8);
}

void ui_Screen2_screen_destroy(void)
{
    lv_obj_del(ui_Screen2);
    ui_Screen2 = NULL;
    ui_dashboardbutton = NULL;
    ui_Screen2_clutch_btn = NULL;
    ui_Screen2_cooling_btn = NULL;
    ui_Screen2_settings_btn = NULL;
    ui_Screen2_test_btn = NULL;
    ui_Screen2_rpm2_btn = NULL;
    ui_Screen2_rpm3_btn = NULL;
}
