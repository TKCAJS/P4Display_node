// Screen 3 - Two Button Screen

#include "ui.h"

lv_obj_t * ui_Screen3;
lv_obj_t * ui_Screen3_dashboardbutton = NULL;
lv_obj_t * ui_Screen3_menubutton = NULL;
lv_obj_t * ui_Screen3_clutch_img = NULL;

void ui_Screen3_screen_init(void)
{
    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen3);
    ui_Screen3_dashboardbutton = nav.dashboard_btn;
    ui_Screen3_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen3, "CLUTCH");

    // Static clutch plate image
    ui_Screen3_clutch_img = lv_image_create(ui_Screen3);
    lv_image_set_src(ui_Screen3_clutch_img, &clutch);
    lv_obj_center(ui_Screen3_clutch_img);
}

void ui_Screen3_screen_destroy(void)
{
    // Stop the rotation animation before destroying the screen
    if (ui_Screen3_clutch_img != NULL) {
        lv_anim_delete(ui_Screen3_clutch_img, NULL);
    }

    lv_obj_del(ui_Screen3);
    ui_Screen3 = NULL;
    ui_Screen3_dashboardbutton = NULL;
    ui_Screen3_menubutton = NULL;
    ui_Screen3_clutch_img = NULL;
}
