// Screen 4 - Cooling

#include "ui.h"
#include "ui_scale.h"   /* 1.25x layout scale for the 1024x600 panel */
#include <stdio.h>

// Slider -> pump target temp: update the readout and hand the value to the
// CAN layer (ui_pump_target_changed, GaugeUpdater.cpp), which transmits it
// to the sensor node from the protocol task.
static void ui_event_target_slider(lv_event_t * e)
{
    int32_t v = lv_slider_get_value(lv_event_get_target_obj(e));
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(ui_Screen4_target_label, buf);
    ui_pump_target_changed(v);
}

lv_obj_t * ui_Screen4;
lv_obj_t * ui_Screen4_dashboardbutton = NULL;
lv_obj_t * ui_Screen4_menubutton = NULL;
lv_obj_t * ui_Screen4_temp_gauge = NULL;
lv_obj_t * ui_Screen4_target_slider = NULL;
lv_obj_t * ui_Screen4_target_label = NULL;
lv_obj_t * ui_Screen4_pump_arc = NULL;
lv_obj_t * ui_Screen4_pump_label = NULL;
lv_obj_t * ui_Screen4_engine_arc = NULL;
lv_obj_t * ui_Screen4_engine_label = NULL;
lv_obj_t * ui_Screen4_radout_arc = NULL;
lv_obj_t * ui_Screen4_radout_label = NULL;
lv_obj_t * ui_Screen4_radiator_arc = NULL;
lv_obj_t * ui_Screen4_radiator_label = NULL;

void ui_Screen4_screen_init(void)
{
    ui_Screen4 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen4);
    ui_Screen4_dashboardbutton = nav.dashboard_btn;
    ui_Screen4_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen4, "COOLING");

    // Temperature gauge
    ui_Screen4_temp_gauge = lv_bar_create(ui_Screen4);
    lv_bar_set_range(ui_Screen4_temp_gauge, 0, 130);
    lv_bar_set_value(ui_Screen4_temp_gauge, 50, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Screen4_temp_gauge, 0, LV_ANIM_OFF);
    lv_obj_set_size(ui_Screen4_temp_gauge, 50, 250);
    lv_obj_set_pos(ui_Screen4_temp_gauge, -300, 50);
    lv_obj_set_align(ui_Screen4_temp_gauge, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Screen4_temp_gauge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen4_temp_gauge, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4_temp_gauge, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Screen4_temp_gauge, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Screen4_temp_gauge, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen4_temp_gauge, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_Screen4_temp_gauge, lv_color_hex(0x0A92D4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_Screen4_temp_gauge, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_Screen4_temp_gauge, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_Screen4_temp_gauge, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_Screen4_temp_gauge, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen4_temp_gauge, lv_color_hex(0x0A92D4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4_temp_gauge, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen4_temp_gauge, 1, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    if(lv_obj_get_style_pad_top(ui_Screen4_temp_gauge, LV_PART_MAIN) > 0) lv_obj_set_style_pad_right(ui_Screen4_temp_gauge,
                                                                                                lv_obj_get_style_pad_right(ui_Screen4_temp_gauge, LV_PART_MAIN) + 1, LV_PART_MAIN);

    // Pump target temperature slider — sets the sensor node's pump curve
    // center over CAN. Range mirrors PUMP_TARGET_MIN/MAX; 82 = default curve.
    ui_Screen4_target_slider = lv_slider_create(ui_Screen4);
    lv_slider_set_range(ui_Screen4_target_slider, 65, 105);
    lv_slider_set_value(ui_Screen4_target_slider, 82, LV_ANIM_OFF);
    lv_obj_set_size(ui_Screen4_target_slider, 50, 300);
    lv_obj_set_pos(ui_Screen4_target_slider, -200, 50);
    lv_obj_set_align(ui_Screen4_target_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Screen4_target_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen4_target_slider, lv_color_hex(0x1a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4_target_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_Screen4_target_slider, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen4_target_slider, lv_color_hex(0x1111ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4_target_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_Screen4_target_slider, lv_color_hex(0x41BACD), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4_target_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Screen4_target_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen4_target_slider, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_Screen4_target_slider, ui_event_target_slider, LV_EVENT_VALUE_CHANGED, NULL);

    // Current target readout above the slider
    ui_Screen4_target_label = lv_label_create(ui_Screen4);
    lv_label_set_text(ui_Screen4_target_label, "82");
    lv_obj_set_pos(ui_Screen4_target_label, -200, -130);
    lv_obj_set_align(ui_Screen4_target_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_Screen4_target_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen4_target_label, &ui_font_DSEG20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * target_caption = lv_label_create(ui_Screen4);
    lv_label_set_text(target_caption, "TARGET C");
    lv_obj_set_pos(target_caption, -200, 225);
    lv_obj_set_align(target_caption, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(target_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(target_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Pump duty arc (0-100 %) — fed from CAN pump status; same style as the
    // temp arcs, green indicator carried over from the old bar. Shrunk from
    // 240 to match the temp arcs so the radiator arc fits above it (2x2 grid).
    ui_Screen4_pump_arc = lv_arc_create(ui_Screen4);
    lv_arc_set_rotation(ui_Screen4_pump_arc, 135);
    lv_arc_set_bg_angles(ui_Screen4_pump_arc, 0, 270);
    lv_arc_set_range(ui_Screen4_pump_arc, 0, 100);
    lv_arc_set_value(ui_Screen4_pump_arc, 0);
    lv_obj_set_size(ui_Screen4_pump_arc, 190, 190);
    lv_obj_set_pos(ui_Screen4_pump_arc, 235, 120);
    lv_obj_set_align(ui_Screen4_pump_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen4_pump_arc, NULL, LV_PART_KNOB);        // gauge, not input
    lv_obj_remove_flag(ui_Screen4_pump_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen4_pump_arc, lv_color_hex(0x6a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_pump_arc, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_Screen4_pump_arc, lv_color_hex(0x33CC55), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_pump_arc, 15, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Integer % readout in the middle of the arc
    ui_Screen4_pump_label = lv_label_create(ui_Screen4_pump_arc);
    lv_label_set_text(ui_Screen4_pump_label, "--");
    lv_obj_set_align(ui_Screen4_pump_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_Screen4_pump_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen4_pump_label, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * pump_caption = lv_label_create(ui_Screen4_pump_arc);
    lv_label_set_text(pump_caption, "PUMP %");
    lv_obj_align(pump_caption, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(pump_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pump_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Engine temp arc (engine NTC, sensor node via CAN) — display only.
    // Stacked with the rad-out arc below in the same column, so both are
    // 190 px instead of the pump arc's 240.
    ui_Screen4_engine_arc = lv_arc_create(ui_Screen4);
    lv_arc_set_rotation(ui_Screen4_engine_arc, 135);
    lv_arc_set_bg_angles(ui_Screen4_engine_arc, 0, 270);
    lv_arc_set_range(ui_Screen4_engine_arc, 0, 130);
    lv_arc_set_value(ui_Screen4_engine_arc, 0);
    lv_obj_set_size(ui_Screen4_engine_arc, 190, 190);
    lv_obj_set_pos(ui_Screen4_engine_arc, -25, -75);
    lv_obj_set_align(ui_Screen4_engine_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen4_engine_arc, NULL, LV_PART_KNOB);      // gauge, not input
    lv_obj_remove_flag(ui_Screen4_engine_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen4_engine_arc, lv_color_hex(0x1a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_engine_arc, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_Screen4_engine_arc, lv_color_hex(0x0A92D4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_engine_arc, 15, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Integer °C readout in the middle of the arc
    ui_Screen4_engine_label = lv_label_create(ui_Screen4_engine_arc);
    lv_label_set_text(ui_Screen4_engine_label, "--");
    lv_obj_set_align(ui_Screen4_engine_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_Screen4_engine_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen4_engine_label, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * engine_caption = lv_label_create(ui_Screen4_engine_arc);
    lv_label_set_text(engine_caption, "ENGINE C");
    lv_obj_align(engine_caption, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(engine_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(engine_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Radiator outlet temp arc (rad-out NTC, sensor node via CAN) — same
    // style as the engine arc, directly below it
    ui_Screen4_radout_arc = lv_arc_create(ui_Screen4);
    lv_arc_set_rotation(ui_Screen4_radout_arc, 135);
    lv_arc_set_bg_angles(ui_Screen4_radout_arc, 0, 270);
    lv_arc_set_range(ui_Screen4_radout_arc, 0, 130);
    lv_arc_set_value(ui_Screen4_radout_arc, 0);
    lv_obj_set_size(ui_Screen4_radout_arc, 190, 190);
    lv_obj_set_pos(ui_Screen4_radout_arc, -25, 120);
    lv_obj_set_align(ui_Screen4_radout_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen4_radout_arc, NULL, LV_PART_KNOB);      // gauge, not input
    lv_obj_remove_flag(ui_Screen4_radout_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen4_radout_arc, lv_color_hex(0x1a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_radout_arc, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_Screen4_radout_arc, lv_color_hex(0x0A92D4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_radout_arc, 15, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Integer °C readout in the middle of the arc
    ui_Screen4_radout_label = lv_label_create(ui_Screen4_radout_arc);
    lv_label_set_text(ui_Screen4_radout_label, "--");
    lv_obj_set_align(ui_Screen4_radout_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_Screen4_radout_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen4_radout_label, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * radout_caption = lv_label_create(ui_Screen4_radout_arc);
    lv_label_set_text(radout_caption, "RAD OUT C");
    lv_obj_align(radout_caption, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(radout_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(radout_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Radiator temp arc (Dallas DS18B20, sensor node via CAN) — numeric
    // readout for the value the left bar shows graphically; above the pump arc
    ui_Screen4_radiator_arc = lv_arc_create(ui_Screen4);
    lv_arc_set_rotation(ui_Screen4_radiator_arc, 135);
    lv_arc_set_bg_angles(ui_Screen4_radiator_arc, 0, 270);
    lv_arc_set_range(ui_Screen4_radiator_arc, 0, 130);
    lv_arc_set_value(ui_Screen4_radiator_arc, 0);
    lv_obj_set_size(ui_Screen4_radiator_arc, 190, 190);
    lv_obj_set_pos(ui_Screen4_radiator_arc, 235, -75);
    lv_obj_set_align(ui_Screen4_radiator_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen4_radiator_arc, NULL, LV_PART_KNOB);    // gauge, not input
    lv_obj_remove_flag(ui_Screen4_radiator_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen4_radiator_arc, lv_color_hex(0x1a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_radiator_arc, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_Screen4_radiator_arc, lv_color_hex(0x0A92D4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen4_radiator_arc, 15, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Integer °C readout in the middle of the arc
    ui_Screen4_radiator_label = lv_label_create(ui_Screen4_radiator_arc);
    lv_label_set_text(ui_Screen4_radiator_label, "--");
    lv_obj_set_align(ui_Screen4_radiator_label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_Screen4_radiator_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen4_radiator_label, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * radiator_caption = lv_label_create(ui_Screen4_radiator_arc);
    lv_label_set_text(radiator_caption, "RADIATOR C");
    lv_obj_align(radiator_caption, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(radiator_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(radiator_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Screen4_screen_destroy(void)
{
    lv_obj_del(ui_Screen4);
    ui_Screen4 = NULL;
    ui_Screen4_dashboardbutton = NULL;
    ui_Screen4_menubutton = NULL;
    ui_Screen4_temp_gauge = NULL;
    ui_Screen4_target_slider = NULL;
    ui_Screen4_target_label = NULL;
    ui_Screen4_pump_arc = NULL;
    ui_Screen4_pump_label = NULL;
    ui_Screen4_engine_arc = NULL;
    ui_Screen4_engine_label = NULL;
    ui_Screen4_radout_arc = NULL;
    ui_Screen4_radout_label = NULL;
    ui_Screen4_radiator_arc = NULL;
    ui_Screen4_radiator_label = NULL;
}
