// Screen 9 - RPM 4: the Screen8 half-dial done as a brass-and-parchment
// instrument, with fancypointer.png as the needle.
//
// The needle is the one thing here that is not drawn from primitives: it is an
// lv_image rotated about the centre of its boss. That is cheap in memory —
// lv_draw_sw_img transforms in strips capped at MAX_BUF_SIZE (4 lines of the
// panel, 6.4 KB) rather than buffering the whole rotated image — but it is not
// cheap in time, so the angle is quantised to a whole degree before anything is
// asked to redraw.
//
// The image is pre-scaled by the converter (see images/fancypointer.c) so the
// boss-to-tip distance is exactly NEEDLE_LEN on the panel; scaling at runtime
// would stack a second transform on every frame for no gain.

#include "ui.h"
#include <math.h>

#define PANEL_H         480
#define DIAL_SIZE       800
#define DIAL_R          (DIAL_SIZE / 2)
#define DIAL_Y_OFFSET   (PANEL_H / 2)       // dial centre on the bottom edge
#define DIAL_ROT_DEG    180                 // 0 RPM at 9 o'clock ...
#define DIAL_SWEEP_DEG  180                 // ... sweeping clockwise to 3 o'clock

// Rotation pivot for fancypointer.c, in image coordinates. The boss centre is at
// (23.5, 26.2) but the pivot is set 35 px OUTSIDE the left edge, so the pointer
// reaches 369 px from the dial centre rather than the image's own 310 — the tip
// now runs right up to the inner end of the ticks at 370.
//
// A negative pivot is fine: lv_pct_to_px() only reinterprets a coordinate whose
// type bits say LV_COORD_TYPE_SPEC, and a negative int carries PX_NEG, so it is
// passed through as plain pixels. lv_image ext_draw_size comes from the actual
// transformed corners, so nothing gets clipped either.
//
// The cost is that the boss now orbits the dial centre at a radius of 58.5 px
// rather than spinning in place, and with its own ~22 px radius it reaches 80.5
// px out — well past the 55 px hub, so the ball swings clear of the brass as it
// sweeps. Deliberate: covering it needs a 170 px hub, which reads as far too
// heavy for this dial.
#define POINTER_PIVOT_X (-35)
#define POINTER_PIVOT_Y 26
#define HUB_SIZE        110

// Aged ivory face, brass bezel, iron-gall lettering.
#define COL_FACE        0xE8DFC8
#define COL_FACE_AGED   0xCFC0A0
#define COL_BRASS       0x8C6A32
#define COL_INK         0x2E2519
#define COL_INK_LIGHT   0x6B5A44
#define COL_RED         0x8E2018

lv_obj_t * ui_Screen9;
lv_obj_t * ui_Screen9_dashboardbutton = NULL;
lv_obj_t * ui_Screen9_menubutton = NULL;

static lv_obj_t * dial_pointer = NULL;
static lv_obj_t * dial_readout = NULL;

// Dial state, all reset by ui_Screen9_screen_init().
static float   needle_rpm = 0.0f;       // smoothed, trails the raw value
static int32_t needle_deg = INT32_MIN;  // last drawn angle, whole degrees
static int32_t shown_rpm  = INT32_MIN;

static lv_style_t style_redline_arc;
static lv_style_t style_redline_major;
static lv_style_t style_redline_minor;

// Thousands, one entry per major tick — 8 majors for 29 ticks at
// major_tick_every 4, the same scale as the other three rev counters.
static const char * tick_labels[] = {"0", "2", "4", "6", "8", "10", "12", "14", NULL};

void ui_Screen9_set_rpm(uint16_t rpm)
{
    // Screen9 is built once at boot but only one screen is ever on the panel;
    // rotating a needle nobody can see is the most expensive thing this file
    // can do for no reason at all.
    if(dial_pointer == NULL || lv_screen_active() != ui_Screen9) return;

    if(rpm > RPM_GAUGE_MAX) rpm = RPM_GAUGE_MAX;

    // Needle inertia: a first-order lag, so the pointer swings into place rather
    // than snapping between samples. At the 30 ms call rate this is ~110 ms.
    needle_rpm += ((float)rpm - needle_rpm) * 0.25f;
    if(fabsf((float)rpm - needle_rpm) < 5.0f) needle_rpm = (float)rpm;

    // A whole degree is ~5 px at the tip. Finer than that is below what the
    // rotation can resolve anyway, and every step costs a full transform of the
    // pointer's bounding box.
    const int32_t deg = DIAL_ROT_DEG +
                        (int32_t)lroundf(needle_rpm * (float)DIAL_SWEEP_DEG / RPM_GAUGE_MAX);
    if(deg != needle_deg) {
        needle_deg = deg;
        lv_image_set_rotation(dial_pointer, deg * 10);   // LVGL angles are 0.1 deg
    }

    // Quantised to 100 RPM so the plate is not re-lettered on every frame.
    const int32_t rounded = ((int32_t)rpm + 50) / 100 * 100;
    if(rounded != shown_rpm) {
        shown_rpm = rounded;
        lv_label_set_text_fmt(dial_readout, "%d", (int)rounded);
    }
}

void ui_Screen9_screen_init(void)
{
    ui_Screen9 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen9, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen9, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen9, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen9);
    ui_Screen9_dashboardbutton = nav.dashboard_btn;
    ui_Screen9_menubutton = nav.menu_btn;

    ui_rpm_carousel_attach(ui_Screen9);

    // Dial container: zero padding, so every child's coordinates are dial-local
    // and the centre is exactly (DIAL_R, DIAL_R). Aligned by its centre rather
    // than an edge, so it is immune to any padding the theme puts on a screen.
    lv_obj_t * cont = lv_obj_create(ui_Screen9);
    lv_obj_set_size(cont, DIAL_SIZE, DIAL_SIZE);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, DIAL_Y_OFFSET);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Face: ivory, darkening towards the bottom like aged paper, in a brass
    // bezel.
    lv_obj_t * face = lv_obj_create(cont);
    lv_obj_set_size(face, DIAL_SIZE, DIAL_SIZE);
    lv_obj_set_pos(face, 0, 0);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(face, lv_color_hex(COL_FACE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(face, lv_color_hex(COL_FACE_AGED), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(face, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(face, lv_color_hex(COL_BRASS), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(face, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    // The hairline ring inside the ticks that every dial of this era has. Even
    // diameter only, so it stays exactly concentric in the 800 px container.
    lv_obj_t * ring = lv_obj_create(cont);
    lv_obj_set_size(ring, 716, 716);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ring, lv_color_hex(COL_INK_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ring, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Redline sector, painted on rather than lit up: deep red, no glow.
    lv_style_init(&style_redline_arc);
    lv_style_set_line_color(&style_redline_arc, lv_color_hex(COL_RED));
    lv_style_set_line_width(&style_redline_arc, 8);

    lv_style_init(&style_redline_major);
    lv_style_set_line_color(&style_redline_major, lv_color_hex(COL_RED));
    lv_style_set_line_width(&style_redline_major, 6);
    lv_style_set_text_color(&style_redline_major, lv_color_hex(COL_RED));

    lv_style_init(&style_redline_minor);
    lv_style_set_line_color(&style_redline_minor, lv_color_hex(COL_RED));
    lv_style_set_line_width(&style_redline_minor, 3);

    // Dial. 29 ticks over 0..14000 puts a minor every 500 RPM; every 4th is a
    // major, i.e. one labelled tick per 2000 RPM.
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
    lv_obj_set_style_line_color(scale, lv_color_hex(COL_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(COL_INK), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 30, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(scale, lv_color_hex(COL_INK), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_28, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_radial(scale, 14, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(scale, lv_color_hex(COL_INK_LIGHT), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(scale, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_length(scale, 16, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_scale_section_t * redline = lv_scale_add_section(scale);
    lv_scale_section_set_range(redline, RPM_GAUGE_REDLINE, RPM_GAUGE_MAX);
    lv_scale_section_set_style(redline, LV_PART_MAIN, &style_redline_arc);
    lv_scale_section_set_style(redline, LV_PART_INDICATOR, &style_redline_major);
    lv_scale_section_set_style(redline, LV_PART_ITEMS, &style_redline_minor);

    // Maker's signature, in the hand-lettered style these dials were finished
    // with. The pointer sweeps over it, as it would on the real thing.
    lv_obj_t * maker = lv_label_create(cont);
    lv_label_set_text(maker, "T89 Motor Co.");
    lv_obj_set_style_text_color(maker, lv_color_hex(COL_INK_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(maker, &ui_font_inkfree40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(maker, LV_ALIGN_CENTER, 0, -230);

    // Fixed width and centred text, so the invalidated area is the same every
    // update instead of growing and shrinking with the digits.
    dial_readout = lv_label_create(cont);
    lv_obj_set_width(dial_readout, 320);
    lv_obj_set_style_text_align(dial_readout, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dial_readout, lv_color_hex(COL_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(dial_readout, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(dial_readout, "0");
    lv_obj_align(dial_readout, LV_ALIGN_CENTER, 0, -150);

    lv_obj_t * unit = lv_label_create(cont);
    lv_label_set_text(unit, "R.P.M.");
    lv_obj_set_style_text_color(unit, lv_color_hex(COL_INK_LIGHT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(unit, LV_ALIGN_CENTER, 0, -100);

    // Pointer, placed so its boss sits on the dial centre, then the hub over
    // the boss. Creation order is z-order.
    dial_pointer = lv_image_create(cont);
    lv_image_set_src(dial_pointer, &fancypointer);
    lv_image_set_pivot(dial_pointer, POINTER_PIVOT_X, POINTER_PIVOT_Y);
    lv_obj_set_pos(dial_pointer, DIAL_R - POINTER_PIVOT_X, DIAL_R - POINTER_PIVOT_Y);

    lv_obj_t * hub = lv_obj_create(cont);
    lv_obj_set_size(hub, HUB_SIZE, HUB_SIZE);
    lv_obj_align(hub, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0xC9A227), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(hub, lv_color_hex(0x6E5219), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(hub, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(hub, lv_color_hex(0x3A2E14), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(hub, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    needle_rpm = 0.0f;
    needle_deg = INT32_MIN;
    shown_rpm  = INT32_MIN;
    lv_image_set_rotation(dial_pointer, DIAL_ROT_DEG * 10);
}

void ui_Screen9_screen_destroy(void)
{
    lv_obj_del(ui_Screen9);
    ui_Screen9 = NULL;
    ui_Screen9_dashboardbutton = NULL;
    ui_Screen9_menubutton = NULL;
    dial_pointer = NULL;
    dial_readout = NULL;
}
