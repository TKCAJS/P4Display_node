// Vertical Slider Component with Value Display

#include "../ui.h"

static void slider_value_changed_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);

    int32_t value = lv_slider_get_value(slider);
    char buf[10];
    lv_snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(label, buf);
}

ui_vertical_slider_t ui_vertical_slider_create(lv_obj_t * comp_parent)
{
    ui_vertical_slider_t slider_obj;

    // Create vertical slider
    slider_obj.slider = lv_slider_create(comp_parent);
    lv_slider_set_range(slider_obj.slider, 0, 100);
    lv_slider_set_value(slider_obj.slider, 50, LV_ANIM_OFF);
    lv_obj_set_size(slider_obj.slider, 60, 200);
    lv_obj_set_style_radius(slider_obj.slider, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider_obj.slider, lv_color_hex(0x2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(slider_obj.slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider_obj.slider, lv_color_hex(0xB40D0D), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(slider_obj.slider, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_set_style_radius(slider_obj.slider, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(slider_obj.slider, lv_color_hex(0x41BACD), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(slider_obj.slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(slider_obj.slider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(slider_obj.slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(slider_obj.slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(slider_obj.slider, &pot_ver_knob, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(slider_obj.slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(slider_obj.slider, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(slider_obj.slider, 100, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(slider_obj.slider, 30, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(slider_obj.slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(slider_obj.slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(slider_obj.slider, -5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(slider_obj.slider, -1, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(slider_obj.slider, -1, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(slider_obj.slider, -5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(slider_obj.slider, -4, LV_PART_KNOB | LV_STATE_DEFAULT);
    
    lv_obj_set_style_bg_color(slider_obj.slider, lv_color_hex(0xffffff),LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_bg_image_src(slider_obj.slider, &pot_ver_knob, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_bg_image_opa(slider_obj.slider, 255, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(slider_obj.slider, lv_color_hex(0x5000fD), LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(slider_obj.slider, 128, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(slider_obj.slider, 60, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(slider_obj.slider, 20, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_offset_x(slider_obj.slider, 0, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_offset_y(slider_obj.slider, 0, LV_PART_KNOB | LV_STATE_PRESSED);


    // Create value display label
    slider_obj.value_label = lv_label_create(comp_parent);
    lv_label_set_text(slider_obj.value_label, "50");
    lv_obj_set_size(slider_obj.value_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(slider_obj.value_label, 0, 110);
    lv_obj_set_align(slider_obj.value_label, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(slider_obj.value_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(slider_obj.value_label, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Add event callback for value changes
    lv_obj_add_event_cb(slider_obj.slider, slider_value_changed_cb, LV_EVENT_VALUE_CHANGED, slider_obj.value_label);

    return slider_obj;
}
