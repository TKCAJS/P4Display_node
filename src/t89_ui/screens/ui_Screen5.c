// Screen 5 - Settings

#include "ui.h"

lv_obj_t * ui_Screen5;
lv_obj_t * ui_Screen5_dashboardbutton = NULL;
lv_obj_t * ui_Screen5_menubutton = NULL;
lv_obj_t * ui_Screen5_versionMain    = NULL;
lv_obj_t * ui_Screen5_versionRear    = NULL;
lv_obj_t * ui_Screen5_versionDisplay = NULL;
ui_vertical_slider_t ui_Screen5_slider_a = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_b = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_c = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_d = {NULL, NULL};

void ui_Screen5_screen_init(void)
{
    ui_Screen5 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen5, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen5, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen5);
    ui_Screen5_dashboardbutton = nav.dashboard_btn;
    ui_Screen5_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen5, "SETTINGS");

    // Slider A
    ui_Screen5_slider_a = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_a.slider, -225, 40);
    lv_obj_set_align(ui_Screen5_slider_a.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_a.value_label, -225);

    // Slider B
    ui_Screen5_slider_b = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_b.slider, -75, 40);
    lv_obj_set_align(ui_Screen5_slider_b.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_b.value_label, -75);

    // Slider C
    ui_Screen5_slider_c = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_c.slider, 75, 40);
    lv_obj_set_align(ui_Screen5_slider_c.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_c.value_label, 75);

    // Slider D
    ui_Screen5_slider_d = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_d.slider, 225, 40);
    lv_obj_set_align(ui_Screen5_slider_d.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_d.value_label, 225);

    // Node versions — bottom-right corner
    ui_Screen5_versionMain = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionMain, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionMain, "Main: --");
    lv_obj_align(ui_Screen5_versionMain, LV_ALIGN_BOTTOM_RIGHT, -10, -60);

    ui_Screen5_versionRear = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionRear, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionRear, "Rear: --");
    lv_obj_align(ui_Screen5_versionRear, LV_ALIGN_BOTTOM_RIGHT, -10, -40);

    ui_Screen5_versionDisplay = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionDisplay, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionDisplay, "Display: " DISPLAY_NODE_VERSION);
    lv_obj_align(ui_Screen5_versionDisplay, LV_ALIGN_BOTTOM_RIGHT, -10, -20);
}

void ui_Screen5_screen_destroy(void)
{
    lv_obj_del(ui_Screen5);
    ui_Screen5 = NULL;
    ui_Screen5_dashboardbutton = NULL;
    ui_Screen5_menubutton = NULL;
    ui_Screen5_slider_a.slider = NULL;
    ui_Screen5_slider_a.value_label = NULL;
    ui_Screen5_slider_b.slider = NULL;
    ui_Screen5_slider_b.value_label = NULL;
    ui_Screen5_slider_c.slider = NULL;
    ui_Screen5_slider_c.value_label = NULL;
    ui_Screen5_slider_d.slider = NULL;
    ui_Screen5_slider_d.value_label = NULL;
    ui_Screen5_versionMain    = NULL;
    ui_Screen5_versionRear    = NULL;
    ui_Screen5_versionDisplay = NULL;
}
