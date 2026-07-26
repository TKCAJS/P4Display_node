// Screen 6 - Test

#include "ui.h"

lv_obj_t * ui_Screen6;
lv_obj_t * ui_Screen6_dashboardbutton = NULL;
lv_obj_t * ui_Screen6_menubutton = NULL;
static lv_obj_t * ui_Screen6_placeholder = NULL;

void ui_Screen6_screen_init(void)
{
    ui_Screen6 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen6, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen6);
    ui_Screen6_dashboardbutton = nav.dashboard_btn;
    ui_Screen6_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen6, "TEST");

    // Empty scratch screen — content goes here.
    ui_Screen6_placeholder = lv_label_create(ui_Screen6);
    lv_label_set_text(ui_Screen6_placeholder, "TEST SCREEN");
    lv_obj_set_style_text_color(ui_Screen6_placeholder, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(ui_Screen6_placeholder);
}

void ui_Screen6_screen_destroy(void)
{
    lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
    ui_Screen6_dashboardbutton = NULL;
    ui_Screen6_menubutton = NULL;
    ui_Screen6_placeholder = NULL;
}
