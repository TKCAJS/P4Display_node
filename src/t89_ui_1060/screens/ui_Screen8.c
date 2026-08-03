// Screen 8 - RPM 3: one 800 px semicircle, pivoted on the bottom edge.
//
// The dial is a full 800x800 circle whose centre is pushed down to the bottom
// edge of the 800x480 panel, so exactly its top half is on screen and the sweep
// runs corner to corner. Same widget budget as Screen6 — lv_scale for the dial,
// an lv_line for the needle — plus a shift bar in the black above the arc.
//
// Nothing here rotates or fades an object: transforms and opacity on a parent
// make LVGL render through a draw layer, and there is no room in the 64 KB pool
// for an 800x800 one.

#include "ui.h"
#include "ui_scale.h"   /* 1.25x layout scale for the 1024x600 panel */
#include <math.h>

#define PANEL_H         480
#define DIAL_SIZE       800                 // = panel width; radius is the half
#define DIAL_R          (DIAL_SIZE / 2)
// Push the dial centre half a panel down from the screen centre and it lands on
// the bottom edge, so the arc peaks at 480 - 400 = 80 px from the top and that
// 80 px band of black is left for the shift bar.
#define DIAL_Y_OFFSET   (PANEL_H / 2)
#define DIAL_ROT_DEG    180                 // 0 RPM at 9 o'clock ...
#define DIAL_SWEEP_DEG  180                 // ... sweeping clockwise to 3 o'clock

#define NEEDLE_LEN      310                 // pivot -> tip, stops inside the labels
#define NEEDLE_WIDTH    8

#define DEG2RAD         0.017453292f

lv_obj_t * ui_Screen8;
lv_obj_t * ui_Screen8_dashboardbutton = NULL;
lv_obj_t * ui_Screen8_menubutton = NULL;

static lv_obj_t * dial_needle   = NULL;
static lv_obj_t * dial_readout  = NULL;
static lv_obj_t * dial_shiftbar = NULL;

// lv_line keeps the pointer it is given rather than copying, so this must
// outlive every set_points() call.
static lv_point_precise_t needle_points[2];

// Dial state, all reset by ui_Screen8_screen_init().
static float   needle_rpm   = 0.0f;         // smoothed, trails the raw value
static int32_t needle_a10   = INT32_MIN;    // last drawn angle, tenths of a degree
static int32_t needle_zone  = -1;           // 0 = silver, 1 = amber, 2 = red
static int32_t shown_rpm    = INT32_MIN;
static bool    shift_lit    = false;

static lv_style_t style_redline_arc;
static lv_style_t style_redline_major;
static lv_style_t style_redline_minor;

// Thousands, one entry per major tick — 8 majors for 29 ticks at
// major_tick_every 4.
static const char * tick_labels[] = {"0", "2", "4", "6", "8", "10", "12", "14", NULL};

static lv_color_t zone_color(int32_t zone)
{
    return (zone == 2) ? lv_color_hex(0xFF2A2A)
           : (zone == 1) ? lv_color_hex(0xFFB020)
           : lv_color_hex(0xE8E8F0);
}

// Point the needle at `rpm`, re-basing it on its own bounding box before
// placing it — see the long note in ui_Screen6.c: anchoring the line to the dial
// instead invalidates the whole 800x800 circle on every move.
static void needle_point_at(float rpm)
{
    const float a  = (DIAL_ROT_DEG + DIAL_SWEEP_DEG * rpm / RPM_GAUGE_MAX) * DEG2RAD;

    const int32_t tip_x = DIAL_R + (int32_t)lroundf(cosf(a) * NEEDLE_LEN);
    const int32_t tip_y = DIAL_R + (int32_t)lroundf(sinf(a) * NEEDLE_LEN);

    const int32_t min_x = LV_MIN(tip_x, DIAL_R);
    const int32_t min_y = LV_MIN(tip_y, DIAL_R);

    // Scaled here rather than by ui_scale.h — lv_line_set_points() keeps the
    // caller's array instead of copying it, so it can't be shimmed through a
    // temporary. See the matching note in ui_Screen6.c.
    needle_points[0].x = ui_s(DIAL_R) - ui_s(min_x);
    needle_points[0].y = ui_s(DIAL_R) - ui_s(min_y);
    needle_points[1].x = ui_s(tip_x)  - ui_s(min_x);
    needle_points[1].y = ui_s(tip_y)  - ui_s(min_y);

    lv_line_set_points(dial_needle, needle_points, 2);
    lv_obj_set_pos(dial_needle, min_x, min_y);
}

void ui_Screen8_set_rpm(uint16_t rpm)
{
    // Screen8 is built once at boot but only one screen is ever on the panel;
    // repainting a needle nobody can see just burns framebuffer bandwidth.
    if(dial_needle == NULL || lv_screen_active() != ui_Screen8) return;

    if(rpm > RPM_GAUGE_MAX) rpm = RPM_GAUGE_MAX;

    // Needle inertia: a first-order lag, so the needle sweeps into place rather
    // than snapping between samples. At the 30 ms call rate this is ~110 ms.
    needle_rpm += ((float)rpm - needle_rpm) * 0.25f;
    if(fabsf((float)rpm - needle_rpm) < 5.0f) needle_rpm = (float)rpm;

    // Skip the redraw entirely once the needle has settled: below a tenth of a
    // degree the line lands on the same pixels anyway.
    const int32_t a10 = (int32_t)lroundf(needle_rpm * (DIAL_SWEEP_DEG * 10.0f) / RPM_GAUGE_MAX);
    if(a10 != needle_a10) {
        needle_a10 = a10;
        needle_point_at(needle_rpm);
    }

    const int32_t zone = (rpm >= RPM_GAUGE_REDLINE) ? 2 : (rpm >= RPM_GAUGE_AMBER) ? 1 : 0;
    if(zone != needle_zone) {
        needle_zone = zone;
        lv_obj_set_style_line_color(dial_needle, zone_color(zone), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(dial_readout, zone_color(zone), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Digital readout, quantised to 100 RPM so it stays legible while sweeping
    // and doesn't repaint a 5-digit DSEG label on every frame.
    const int32_t rounded = ((int32_t)rpm + 50) / 100 * 100;
    if(rounded != shown_rpm) {
        shown_rpm = rounded;
        lv_label_set_text_fmt(dial_readout, "%d", (int)rounded);
    }

    const bool lit = (rpm >= RPM_GAUGE_REDLINE);
    if(lit != shift_lit) {
        shift_lit = lit;
        lv_obj_set_style_bg_color(dial_shiftbar,
                                  lit ? lv_color_hex(0xFF1020) : lv_color_hex(0x2A1015),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(dial_shiftbar, lit ? 20 : 0,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(dial_shiftbar,
                                    lit ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x6A3038),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_Screen8_screen_init(void)
{
    ui_Screen8 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen8, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen8, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen8);
    ui_Screen8_dashboardbutton = nav.dashboard_btn;
    ui_Screen8_menubutton = nav.menu_btn;

    ui_rpm_carousel_attach(ui_Screen8);

    // No ui_screen_title_create() here: the dial owns the whole panel, and
    // ui_rpm_carousel_attach() has already put the view's name and the position
    // dots in the strip of black next to the nav button.

    // Shift bar, in the black above the arc and clear of both nav buttons.
    // Dark until the redline.
    dial_shiftbar = lv_label_create(ui_Screen8);
    lv_label_set_text(dial_shiftbar, "SHIFT");
    lv_obj_set_size(dial_shiftbar, 240, 44);
    lv_obj_align(dial_shiftbar, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_text_align(dial_shiftbar, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(dial_shiftbar, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dial_shiftbar, lv_color_hex(0x6A3038), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(dial_shiftbar, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dial_shiftbar, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dial_shiftbar, lv_color_hex(0x2A1015), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dial_shiftbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(dial_shiftbar, lv_color_hex(0x55202A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dial_shiftbar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(dial_shiftbar, lv_color_hex(0xFF1020), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(dial_shiftbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Dial container: zero padding, so every child's coordinates are dial-local
    // and the centre is exactly (DIAL_R, DIAL_R). Aligned by its centre rather
    // than an edge, so it is immune to any padding the theme puts on a screen.
    lv_obj_t * cont = lv_obj_create(ui_Screen8);
    lv_obj_set_size(cont, DIAL_SIZE, DIAL_SIZE);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, DIAL_Y_OFFSET);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Face: a dark disc behind the ticks, so the half dial reads as an
    // instrument rather than as marks floating on black.
    lv_obj_t * face = lv_obj_create(cont);
    lv_obj_set_size(face, DIAL_SIZE, DIAL_SIZE);
    lv_obj_set_pos(face, 0, 0);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(face, lv_color_hex(0x11151C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(face, lv_color_hex(0x04060A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(face, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(face, lv_color_hex(0x3A4150), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(face, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Redline styling for the 12k-14k section: arc, major ticks and minor ticks.
    lv_style_init(&style_redline_arc);
    lv_style_set_line_color(&style_redline_arc, lv_color_hex(0xFF2A2A));
    lv_style_set_line_width(&style_redline_arc, 6);

    lv_style_init(&style_redline_major);
    lv_style_set_line_color(&style_redline_major, lv_color_hex(0xFF2A2A));
    lv_style_set_line_width(&style_redline_major, 6);
    lv_style_set_text_color(&style_redline_major, lv_color_hex(0xFF5555));

    lv_style_init(&style_redline_minor);
    lv_style_set_line_color(&style_redline_minor, lv_color_hex(0xB01818));
    lv_style_set_line_width(&style_redline_minor, 3);

    // Dial. 29 ticks over 0..14000 puts a minor every 500 RPM; every 4th is a
    // major, i.e. one labelled tick per 2000 RPM — the same scale as Screen6
    // and Screen7, just stretched over the full width.
    lv_obj_t * scale = lv_scale_create(cont);
    lv_obj_set_size(scale, DIAL_SIZE, DIAL_SIZE);
    lv_obj_set_pos(scale, 0, 0);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(scale, 0, RPM_GAUGE_MAX);
    lv_scale_set_total_tick_count(scale, 29);
    lv_scale_set_major_tick_every(scale, 4);
    lv_scale_set_angle_range(scale, DIAL_SWEEP_DEG);
    lv_scale_set_rotation(scale, DIAL_ROT_DEG);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_text_src(scale, tick_labels);

    lv_obj_set_style_bg_opa(scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(scale, lv_color_hex(0x4A5262), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0xE8E8F0), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 30, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(scale, lv_color_hex(0xD8DCE6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_28, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_radial(scale, 14, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0x6C7486), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 18, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_scale_section_t * redline = lv_scale_add_section(scale);
    lv_scale_section_set_range(redline, RPM_GAUGE_REDLINE, RPM_GAUGE_MAX);
    lv_scale_section_set_style(redline, LV_PART_MAIN, &style_redline_arc);
    lv_scale_section_set_style(redline, LV_PART_INDICATOR, &style_redline_major);
    lv_scale_section_set_style(redline, LV_PART_ITEMS, &style_redline_minor);

    // Digital readout, high in the empty middle of the half dial. Fixed width
    // and centred text, so the invalidated area is the same every update
    // instead of growing and shrinking with the digits.
    dial_readout = lv_label_create(cont);
    lv_obj_set_width(dial_readout, 360);
    lv_obj_set_style_text_align(dial_readout, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dial_readout, lv_color_hex(0xE8E8F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(dial_readout, &ui_font_DSEG60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(dial_readout, "0");
    lv_obj_align(dial_readout, LV_ALIGN_CENTER, 0, -140);

    lv_obj_t * unit = lv_label_create(cont);
    lv_label_set_text(unit, "RPM");
    lv_obj_set_style_text_color(unit, lv_color_hex(0x7A8290), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(unit, LV_ALIGN_CENTER, 0, -85);

    // Needle, then the hub over the pivot: creation order is z-order, so the
    // needle passes over the readout and the hub covers the needle's root.
    dial_needle = lv_line_create(cont);
    lv_obj_set_style_line_width(dial_needle, NEEDLE_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(dial_needle, lv_color_hex(0xE8E8F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(dial_needle, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Hub, half of it below the bottom edge of the panel.
    lv_obj_t * hub = lv_obj_create(cont);
    lv_obj_set_size(hub, 120, 120);
    lv_obj_align(hub, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0x9AA2B4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(hub, lv_color_hex(0x30353F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(hub, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(hub, lv_color_hex(0x1A1D24), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(hub, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    needle_rpm  = 0.0f;
    needle_a10  = INT32_MIN;
    needle_zone = -1;
    shown_rpm   = INT32_MIN;
    shift_lit   = false;
    needle_point_at(0.0f);
}

void ui_Screen8_screen_destroy(void)
{
    lv_obj_del(ui_Screen8);
    ui_Screen8 = NULL;
    ui_Screen8_dashboardbutton = NULL;
    ui_Screen8_menubutton = NULL;
    dial_needle   = NULL;
    dial_readout  = NULL;
    dial_shiftbar = NULL;
}
