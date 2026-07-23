// Screen Title Component

#include "ui.h"

lv_obj_t * ui_screen_title_create(lv_obj_t * comp_parent, const char * title_text)
{
    lv_obj_t * title = lv_label_create(comp_parent);
    lv_label_set_text(title, title_text);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    return title;
}
