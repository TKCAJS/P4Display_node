// Temperature Gauge Component — vertical bar + overlaid tick scale

#include "../ui.h"
#include "ui_scale.h"   /* 1.25x layout scale for the 1024x600 panel */

// 26 ticks with a major every 5 -> 6 major ticks, so 6 labels. Static because
// LVGL keeps the pointer rather than copying; every instance shares this array.
static const char * temp_gauge_labels[] = { "0 °C", "25", "50", "75", "100", "125", NULL };

ui_temp_gauge_t ui_temp_gauge_create(lv_obj_t * comp_parent, int32_t x)
{
    ui_temp_gauge_t gauge;

    gauge.bar = lv_bar_create(comp_parent);
    lv_bar_set_range(gauge.bar, UI_TEMP_GAUGE_MIN_C, UI_TEMP_GAUGE_MAX_C);
    lv_bar_set_value(gauge.bar, UI_TEMP_GAUGE_MIN_C, LV_ANIM_OFF);
    lv_bar_set_start_value(gauge.bar, UI_TEMP_GAUGE_MIN_C, LV_ANIM_OFF);
    lv_obj_set_size(gauge.bar, UI_TEMP_GAUGE_W, UI_TEMP_GAUGE_H);
    lv_obj_set_align(gauge.bar, LV_ALIGN_CENTER);
    lv_obj_set_pos(gauge.bar, x, UI_TEMP_GAUGE_Y);

    lv_obj_set_style_radius(gauge.bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gauge.bar, lv_color_hex(UI_TEMP_GAUGE_C_FACE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(gauge.bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(gauge.bar, lv_color_hex(UI_TEMP_GAUGE_C_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(gauge.bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(gauge.bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(gauge.bar, lv_color_hex(UI_TEMP_GAUGE_C_FILL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(gauge.bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(gauge.bar, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(gauge.bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(gauge.bar, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gauge.bar, lv_color_hex(UI_TEMP_GAUGE_C_FILL), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(gauge.bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(gauge.bar, 1, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    //Compensating for LVGL9.1 draw crash with bar/slider max value when top-padding is nonzero and right-padding is 0
    if(lv_obj_get_style_pad_top(gauge.bar, LV_PART_MAIN) > 0)
        lv_obj_set_style_pad_right(gauge.bar, lv_obj_get_style_pad_right(gauge.bar, LV_PART_MAIN) + 1, LV_PART_MAIN);

    // Created after the bar so the ticks draw on top of it.
    gauge.scale = lv_scale_create(comp_parent);
    lv_obj_set_size(gauge.scale, UI_TEMP_GAUGE_SCALE_W, UI_TEMP_GAUGE_H);
    lv_obj_set_align(gauge.scale, LV_ALIGN_CENTER);
    lv_obj_set_pos(gauge.scale, x + UI_TEMP_GAUGE_SCALE_DX, UI_TEMP_GAUGE_Y + UI_TEMP_GAUGE_SCALE_DY + 5);
    lv_scale_set_mode(gauge.scale, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_scale_set_range(gauge.scale, UI_TEMP_GAUGE_MIN_C, UI_TEMP_GAUGE_MAX_C);
    lv_scale_set_total_tick_count(gauge.scale, 26);
    lv_scale_set_major_tick_every(gauge.scale, 5);
    lv_scale_set_label_show(gauge.scale, true);

    lv_obj_set_style_length(gauge.scale, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(gauge.scale, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(gauge.scale, true, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(gauge.scale, lv_color_hex(UI_TEMP_GAUGE_C_TICK), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(gauge.scale, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gauge.scale, lv_color_hex(UI_TEMP_GAUGE_C_IND), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(gauge.scale, lv_color_hex(UI_TEMP_GAUGE_C_IND_BRD), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(gauge.scale, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(gauge.scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(gauge.scale, lv_color_hex(0x00000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_scale_set_text_src(gauge.scale, temp_gauge_labels);

    return gauge;
}
