// Screen 6 - Test: rev counter drawn with stock LVGL widgets
//
// lv_scale supplies the dial (arc, ticks, labels, redline section); the needle
// is an lv_line whose two points and object position are recomputed per frame.
// Nothing here needs vector graphics, so LV_MEM_SIZE stays at its stock 64 KB.

#include "ui.h"
#include <math.h>

#define GAUGE_SIZE      470                 // 480 screen - 5 px top and bottom
#define GAUGE_R         (GAUGE_SIZE / 2)
// Rotated so the 12 sits at 12 o'clock: 270 deg (straight up) minus the
// 231.4 deg the sweep has covered by 12000 RPM leaves 38.6, rounded to 39 —
// within half a degree, about 1.5 px out at this radius. Same 270 deg sweep and
// tick spacing as before, so the uncovered quadrant now faces right.
#define GAUGE_ROT_DEG   39
#define GAUGE_SWEEP_DEG 270

// Scale thresholds live in ui.h so the bar tacho on Screen7 reads the same.
#define RPM_MAX         RPM_GAUGE_MAX
#define RPM_AMBER       RPM_GAUGE_AMBER
#define RPM_REDLINE     RPM_GAUGE_REDLINE

#define NEEDLE_LEN      180                 // pivot -> tip, stops just inside the ticks
#define NEEDLE_TAIL     42                  // counterweight behind the pivot
#define NEEDLE_WIDTH    6

#define DEG2RAD         0.017453292f

lv_obj_t * ui_Screen6;
lv_obj_t * ui_Screen6_dashboardbutton = NULL;
lv_obj_t * ui_Screen6_menubutton = NULL;

static lv_obj_t * gauge_needle   = NULL;
static lv_obj_t * gauge_readout  = NULL;
static lv_obj_t * gauge_shiftled = NULL;

// lv_line keeps the pointer it is given rather than copying, so these must
// outlive every set_points() call.
static lv_point_precise_t needle_points[2];

// Needle state, all reset by ui_Screen6_screen_init().
static float   needle_rpm  = 0.0f;   // smoothed, trails the raw value
static int32_t needle_a10  = INT32_MIN;   // last drawn angle, tenths of a degree
static int32_t needle_zone = -1;     // 0 = silver, 1 = amber, 2 = red
static int32_t shown_rpm   = INT32_MIN;
static bool    shift_lit   = false;

static lv_style_t style_redline_arc;
static lv_style_t style_redline_major;
static lv_style_t style_redline_minor;

// Thousands, so the dial reads like a tach rather than a counter. One entry per
// major tick — 8 majors for 29 ticks at major_tick_every 4.
static const char * tick_labels[] = {"0", "2", "4", "6", "8", "10", "12", "14", NULL};

// Point the needle at `rpm`, moving the line object to sit exactly on its own
// bounding box.
//
// This is why the needle isn't driven by lv_scale_set_line_needle_value(): that
// helper anchors the line to the scale's top-left and keeps the pivot in the
// point list, so an LV_SIZE_CONTENT line ends up as large as the dial and every
// frame invalidates up to ~290 KB of RGB565. Re-basing the points on their own
// bounding box each frame invalidates only the needle itself. lv_line already
// reports ext_draw_size = line_width, which covers the rounded caps.
static void needle_point_at(float rpm)
{
    const float a  = (GAUGE_ROT_DEG + GAUGE_SWEEP_DEG * rpm / RPM_MAX) * DEG2RAD;
    const float ux = cosf(a);
    const float uy = sinf(a);

    const int32_t tip_x  = GAUGE_R + (int32_t)lroundf(ux * NEEDLE_LEN);
    const int32_t tip_y  = GAUGE_R + (int32_t)lroundf(uy * NEEDLE_LEN);
    const int32_t tail_x = GAUGE_R - (int32_t)lroundf(ux * NEEDLE_TAIL);
    const int32_t tail_y = GAUGE_R - (int32_t)lroundf(uy * NEEDLE_TAIL);

    const int32_t min_x = LV_MIN(tip_x, tail_x);
    const int32_t min_y = LV_MIN(tip_y, tail_y);

    needle_points[0].x = tail_x - min_x;
    needle_points[0].y = tail_y - min_y;
    needle_points[1].x = tip_x - min_x;
    needle_points[1].y = tip_y - min_y;

    lv_line_set_points(gauge_needle, needle_points, 2);
    lv_obj_set_pos(gauge_needle, min_x, min_y);
}

void ui_Screen6_set_rpm(uint16_t rpm)
{
    // Screen6 is built once at boot but only one screen is ever on the panel;
    // repainting a needle nobody can see just burns framebuffer bandwidth.
    if(gauge_needle == NULL || lv_screen_active() != ui_Screen6) return;

    if(rpm > RPM_MAX) rpm = RPM_MAX;

    // Needle inertia: a first-order lag, so the needle sweeps into place rather
    // than snapping between samples. At the 30 ms call rate this is ~110 ms.
    needle_rpm += ((float)rpm - needle_rpm) * 0.25f;
    if(fabsf((float)rpm - needle_rpm) < 5.0f) needle_rpm = (float)rpm;

    // Skip the redraw entirely once the needle has settled: below a tenth of a
    // degree the line lands on the same pixels anyway.
    const int32_t a10 = (int32_t)lroundf(needle_rpm * (GAUGE_SWEEP_DEG * 10.0f) / RPM_MAX);
    if(a10 != needle_a10) {
        needle_a10 = a10;
        needle_point_at(needle_rpm);
    }

    const int32_t zone = (rpm >= RPM_REDLINE) ? 2 : (rpm >= RPM_AMBER) ? 1 : 0;
    if(zone != needle_zone) {
        needle_zone = zone;
        lv_color_t c = (zone == 2) ? lv_color_hex(0xFF2A2A)
                       : (zone == 1) ? lv_color_hex(0xFFB020)
                       : lv_color_hex(0xE8E8F0);
        lv_obj_set_style_line_color(gauge_needle, c, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Digital readout, quantised to 100 RPM so it stays legible while sweeping
    // and doesn't repaint a 5-digit DSEG label on every frame.
    const int32_t rounded = ((int32_t)rpm + 50) / 100 * 100;
    if(rounded != shown_rpm) {
        shown_rpm = rounded;
        lv_label_set_text_fmt(gauge_readout, "%d", (int)rounded);
    }

    const bool lit = (rpm >= RPM_REDLINE);
    if(lit != shift_lit) {
        shift_lit = lit;
        lv_obj_set_style_bg_color(gauge_shiftled,
                                  lit ? lv_color_hex(0xFF1020) : lv_color_hex(0x2A1015),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(gauge_shiftled, lit ? 18 : 0,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

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

    // Gauge container: zero padding, so every child's coordinates are dial-local
    // and the centre is exactly (GAUGE_R, GAUGE_R). Centred on the 480 px height,
    // which leaves the 5 px top and bottom gap a bezel can later sit in.
    lv_obj_t * cont = lv_obj_create(ui_Screen6);
    lv_obj_set_size(cont, GAUGE_SIZE, GAUGE_SIZE);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Bezel: a dark dished disc behind the ticks so the dial reads as an
    // instrument rather than as floating marks.
    lv_obj_t * bezel = lv_obj_create(cont);
    lv_obj_set_size(bezel, GAUGE_SIZE, GAUGE_SIZE);
    lv_obj_set_pos(bezel, 0, 0);
    lv_obj_remove_flag(bezel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bezel, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bezel, lv_color_hex(0x161A22), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(bezel, lv_color_hex(0x05070A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bezel, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bezel, lv_color_hex(0x3A4150), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bezel, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Redline styling for the 12k-14k section: arc, major ticks and minor ticks.
    lv_style_init(&style_redline_arc);
    lv_style_set_line_color(&style_redline_arc, lv_color_hex(0xFF2A2A));
    lv_style_set_line_width(&style_redline_arc, 6);

    lv_style_init(&style_redline_major);
    lv_style_set_line_color(&style_redline_major, lv_color_hex(0xFF2A2A));
    lv_style_set_line_width(&style_redline_major, 5);
    lv_style_set_text_color(&style_redline_major, lv_color_hex(0xFF5555));

    lv_style_init(&style_redline_minor);
    lv_style_set_line_color(&style_redline_minor, lv_color_hex(0xB01818));
    lv_style_set_line_width(&style_redline_minor, 3);

    // Dial. 29 ticks over 0..14000 puts a minor every 500 RPM; every 4th is a
    // major, i.e. one labelled tick per 2000 RPM.
    lv_obj_t * scale = lv_scale_create(cont);
    lv_obj_set_size(scale, GAUGE_SIZE, GAUGE_SIZE);
    lv_obj_set_pos(scale, 0, 0);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(scale, 0, RPM_MAX);
    lv_scale_set_total_tick_count(scale, 29);
    lv_scale_set_major_tick_every(scale, 4);
    lv_scale_set_angle_range(scale, GAUGE_SWEEP_DEG);
    lv_scale_set_rotation(scale, GAUGE_ROT_DEG);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_text_src(scale, tick_labels);

    lv_obj_set_style_bg_opa(scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(scale, lv_color_hex(0x4A5262), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0xE8E8F0), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 26, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(scale, lv_color_hex(0xD8DCE6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_28, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    // Labels sit LV_SCALE_DEFAULT_LABEL_GAP (15 px) in from the tick ends, which
    // the 28 px font closes up; pad_radial adds to that gap.
    lv_obj_set_style_pad_radial(scale, 16, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(0x6C7486), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 14, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_scale_section_t * redline = lv_scale_add_section(scale);
    lv_scale_section_set_range(redline, RPM_REDLINE, RPM_MAX);
    lv_scale_section_set_style(redline, LV_PART_MAIN, &style_redline_arc);
    lv_scale_section_set_style(redline, LV_PART_INDICATOR, &style_redline_major);
    lv_scale_section_set_style(redline, LV_PART_ITEMS, &style_redline_minor);

    // Shift light, above the pivot. Dark until the redline.
    gauge_shiftled = lv_obj_create(cont);
    lv_obj_set_size(gauge_shiftled, 26, 26);
    lv_obj_align(gauge_shiftled, LV_ALIGN_CENTER, 0, -96);
    lv_obj_remove_flag(gauge_shiftled, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(gauge_shiftled, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gauge_shiftled, lv_color_hex(0x2A1015), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(gauge_shiftled, lv_color_hex(0x55202A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(gauge_shiftled, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(gauge_shiftled, lv_color_hex(0xFF1020), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(gauge_shiftled, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Digital readout. Fixed width and centred text, so the invalidated area is
    // the same every update instead of growing and shrinking with the digits.
    gauge_readout = lv_label_create(cont);
    lv_obj_set_width(gauge_readout, 240);
    lv_obj_set_style_text_align(gauge_readout, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(gauge_readout, lv_color_hex(0x30E0FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(gauge_readout, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(gauge_readout, "0");
    lv_obj_align(gauge_readout, LV_ALIGN_CENTER, 0, 85);

    lv_obj_t * unit = lv_label_create(cont);
    lv_label_set_text(unit, "RPM  x1000");
    lv_obj_set_style_text_color(unit, lv_color_hex(0x7A8290), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(unit, LV_ALIGN_CENTER, 0, 140);

    // Needle, then the hub over its pivot. Creation order is z-order, so the hub
    // hides the tail's inner end and the readout stays legible under a sweep.
    gauge_needle = lv_line_create(cont);
    lv_obj_set_style_line_width(gauge_needle, NEEDLE_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(gauge_needle, lv_color_hex(0xE8E8F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(gauge_needle, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * hub = lv_obj_create(cont);
    lv_obj_set_size(hub, 52, 52);
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

void ui_Screen6_screen_destroy(void)
{
    lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
    ui_Screen6_dashboardbutton = NULL;
    ui_Screen6_menubutton = NULL;
    gauge_needle   = NULL;
    gauge_readout  = NULL;
    gauge_shiftled = NULL;
}
