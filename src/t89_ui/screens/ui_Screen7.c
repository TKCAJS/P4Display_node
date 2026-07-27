// Screen 7 - RPM 2: the same rev signal as Screen6, drawn as a linear bar
// tacho instead of a round dial.
//
// Everything here is a stock lv_bar / lv_scale / lv_label, so there is no
// per-frame geometry to recompute and nothing that needs a draw layer — only
// the bar indicator and the readout label repaint as the revs move.

#include "ui.h"

#define BAR_W           700                 // 800 screen - 50 px each side
#define BAR_H           54
#define BAR_Y           70                  // offset from screen centre
#define ZONE_STRIP_H    8

lv_obj_t * ui_Screen7;
lv_obj_t * ui_Screen7_dashboardbutton = NULL;
lv_obj_t * ui_Screen7_menubutton = NULL;

static lv_obj_t * rpm_bar     = NULL;
static lv_obj_t * rpm_readout = NULL;

// Bar/readout state, all reset by ui_Screen7_screen_init().
static int32_t shown_bar  = INT32_MIN;
static int32_t shown_rpm  = INT32_MIN;
static int32_t bar_zone   = -1;     // 0 = cyan, 1 = amber, 2 = red

// Thousands, one entry per major tick — 8 majors for 29 ticks at
// major_tick_every 4, matching the round dial on Screen6.
static const char * bar_tick_labels[] = {"0", "2", "4", "6", "8", "10", "12", "14", NULL};

// A coloured strip above the bar marking where the amber and redline zones
// start, so the zone is readable before the indicator reaches it.
static void zone_strip_create(lv_obj_t * parent, int32_t from_rpm, int32_t to_rpm, uint32_t color)
{
    const int32_t x_from = (int32_t)((int64_t)BAR_W * from_rpm / RPM_GAUGE_MAX);
    const int32_t x_to   = (int32_t)((int64_t)BAR_W * to_rpm / RPM_GAUGE_MAX);

    lv_obj_t * strip = lv_obj_create(parent);
    lv_obj_set_size(strip, x_to - x_from, ZONE_STRIP_H);
    // Centre of the strip relative to the centre of the bar.
    lv_obj_align(strip, LV_ALIGN_CENTER,
                 (x_from + x_to) / 2 - BAR_W / 2,
                 BAR_Y - BAR_H / 2 - ZONE_STRIP_H);
    lv_obj_remove_flag(strip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(strip, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(strip, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(strip, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(strip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(strip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Screen7_set_rpm(uint16_t rpm)
{
    // Screen7 is built once at boot but only one screen is ever on the panel;
    // repainting a bar nobody can see just burns framebuffer bandwidth.
    if(rpm_bar == NULL || lv_screen_active() != ui_Screen7) return;

    if(rpm > RPM_GAUGE_MAX) rpm = RPM_GAUGE_MAX;

    // 50 RPM is ~2.5 px of the 700 px bar, so anything finer redraws the same
    // pixels.
    const int32_t stepped = rpm / 50 * 50;
    if(stepped != shown_bar) {
        shown_bar = stepped;
        lv_bar_set_value(rpm_bar, stepped, LV_ANIM_OFF);
    }

    const int32_t zone = (rpm >= RPM_GAUGE_REDLINE) ? 2 : (rpm >= RPM_GAUGE_AMBER) ? 1 : 0;
    if(zone != bar_zone) {
        bar_zone = zone;
        lv_color_t c = (zone == 2) ? lv_color_hex(0xFF2A2A)
                       : (zone == 1) ? lv_color_hex(0xFFB020)
                       : lv_color_hex(0x30E0FF);
        lv_obj_set_style_bg_color(rpm_bar, c, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    // Quantised to 100 so a 5-digit DSEG label isn't repainted every tick.
    const int32_t rounded = ((int32_t)rpm + 50) / 100 * 100;
    if(rounded != shown_rpm) {
        shown_rpm = rounded;
        lv_label_set_text_fmt(rpm_readout, "%d", (int)rounded);
    }
}

void ui_Screen7_screen_init(void)
{
    ui_Screen7 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen7, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen7, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen7, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen7);
    ui_Screen7_dashboardbutton = nav.dashboard_btn;
    ui_Screen7_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen7, "RPM 2");

    // Digital readout. Fixed width and centred text, so the invalidated area is
    // the same every update instead of growing and shrinking with the digits.
    rpm_readout = lv_label_create(ui_Screen7);
    lv_obj_set_width(rpm_readout, 400);
    lv_obj_set_style_text_align(rpm_readout, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(rpm_readout, lv_color_hex(0x30E0FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(rpm_readout, &ui_font_DSEG72, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(rpm_readout, "0");
    lv_obj_align(rpm_readout, LV_ALIGN_CENTER, 0, -55);

    lv_obj_t * unit = lv_label_create(ui_Screen7);
    lv_label_set_text(unit, "RPM");
    lv_obj_set_style_text_color(unit, lv_color_hex(0x7A8290), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(unit, LV_ALIGN_CENTER, 0, 10);

    zone_strip_create(ui_Screen7, RPM_GAUGE_AMBER, RPM_GAUGE_REDLINE, 0xFFB020);
    zone_strip_create(ui_Screen7, RPM_GAUGE_REDLINE, RPM_GAUGE_MAX, 0xFF2A2A);

    rpm_bar = lv_bar_create(ui_Screen7);
    lv_bar_set_range(rpm_bar, 0, RPM_GAUGE_MAX);
    lv_bar_set_value(rpm_bar, 0, LV_ANIM_OFF);
    lv_obj_set_size(rpm_bar, BAR_W, BAR_H);
    lv_obj_align(rpm_bar, LV_ALIGN_CENTER, 0, BAR_Y);
    lv_obj_set_style_radius(rpm_bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(rpm_bar, lv_color_hex(0x161A22), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(rpm_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(rpm_bar, lv_color_hex(0x3A4150), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(rpm_bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(rpm_bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(rpm_bar, lv_color_hex(0x30E0FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(rpm_bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Tick scale under the bar: 29 ticks over 0..14000 puts a minor every
    // 500 RPM, every 4th major, i.e. one labelled tick per 2000 RPM.
    lv_obj_t * scale = lv_scale_create(ui_Screen7);
    lv_obj_set_size(scale, BAR_W, 50);
    lv_obj_align(scale, LV_ALIGN_CENTER, 0, BAR_Y + BAR_H / 2 + 30);
    lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_range(scale, 0, RPM_GAUGE_MAX);
    lv_scale_set_total_tick_count(scale, 29);
    lv_scale_set_major_tick_every(scale, 4);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_text_src(scale, bar_tick_labels);

    lv_obj_set_style_bg_opa(scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0xE8E8F0), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 16, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(scale, lv_color_hex(0xD8DCE6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_20, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0x6C7486), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 2, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 9, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_obj_t * hint = lv_label_create(ui_Screen7);
    lv_label_set_text(hint, "x1000");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7A8290), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_RIGHT, -20, -10);

    shown_bar = INT32_MIN;
    shown_rpm = INT32_MIN;
    bar_zone  = -1;
}

void ui_Screen7_screen_destroy(void)
{
    lv_obj_del(ui_Screen7);
    ui_Screen7 = NULL;
    ui_Screen7_dashboardbutton = NULL;
    ui_Screen7_menubutton = NULL;
    rpm_bar     = NULL;
    rpm_readout = NULL;
}
