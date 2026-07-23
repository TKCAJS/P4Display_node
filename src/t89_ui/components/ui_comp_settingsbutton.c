// Settings Button Component

#include "../ui.h"

lv_obj_t * ui_settingsbutton_create(lv_obj_t * comp_parent)
{
    lv_obj_t * cui_settingsbutton;
    cui_settingsbutton = lv_button_create(comp_parent);
    lv_obj_set_size(cui_settingsbutton, 160, 100);
    lv_obj_set_align(cui_settingsbutton, LV_ALIGN_CENTER);
    lv_obj_add_flag(cui_settingsbutton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_flag(cui_settingsbutton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cui_settingsbutton, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_settingsbutton, lv_color_hex(0xffaaFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsbutton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(cui_settingsbutton, lv_color_hex(0xff1140), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(cui_settingsbutton, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cui_settingsbutton, lv_color_hex(0xffaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cui_settingsbutton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsbutton, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(cui_settingsbutton, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(cui_settingsbutton, lv_color_hex(0x00414C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(cui_settingsbutton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(cui_settingsbutton, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(cui_settingsbutton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(cui_settingsbutton, -5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(cui_settingsbutton, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_settingsbutton, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_settingsbutton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_settingsbutton, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t ** children = lv_malloc(sizeof(lv_obj_t *) * _UI_COMP_SETTINGSBUTTON_NUM);
    children[UI_COMP_SETTINGSBUTTON_SETTINGSBUTTON] = cui_settingsbutton;
    lv_obj_add_event_cb(cui_settingsbutton, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_settingsbutton, del_component_child_event_cb, LV_EVENT_DELETE, children);
    return cui_settingsbutton;
}
