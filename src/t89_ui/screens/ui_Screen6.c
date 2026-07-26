// Screen 6 - Test Screen: ThorVG rev counter experiment
//
// Dial/track/labels are plain LVGL widgets; the needle is drawn fresh every
// frame as a ThorVG vector path (lv_draw_vector) so it can sweep smoothly and
// glow red near the redline without needing a pre-rendered image per angle.

#include "ui.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define REV_MAX_RPM      14000.0f
#define REV_REDLINE_RPM  9000.0f
#define REV_SWEEP_START  135.0f   /* dial angle (arc convention) at 0 RPM   */
#define REV_SWEEP_SPAN   270.0f   /* total sweep across the dial           */

/* Needle outline in dial-local coordinates, pivot at (0,0), pointing up (-Y)
 * before rotation. Shared by the draw callback and the invalidate-area maths
 * so the drawn shape and the redrawn region can't drift apart. */
static const float REV_NEEDLE_PTS[][2] = {
    { -9.0f,    0.0f },
    { -3.0f, -140.0f },
    {  0.0f, -162.0f },
    {  3.0f, -140.0f },
    {  9.0f,    0.0f },
    {  7.0f,   32.0f },
    { -7.0f,   32.0f },
};
#define REV_NEEDLE_PT_CNT (sizeof(REV_NEEDLE_PTS) / sizeof(REV_NEEDLE_PTS[0]))
#define REV_HUB_R          22.0f  /* outer hub disc, always drawn at the pivot */
#define REV_DRAW_MARGIN     4.0f  /* stroke width + anti-aliasing slack       */

/* "Nothing on screen yet" — forces a full invalidate rather than a swept box */
#define REV_RPM_UNDRAWN    (-1.0e9f)

lv_obj_t * ui_Screen6;
lv_obj_t * ui_Screen6_dashboardbutton = NULL;
lv_obj_t * ui_Screen6_menubutton = NULL;
lv_obj_t * ui_Screen6_bezel = NULL;
lv_obj_t * ui_Screen6_dial = NULL;
lv_obj_t * ui_Screen6_track_arc = NULL;
lv_obj_t * ui_Screen6_redline_arc = NULL;
lv_obj_t * ui_Screen6_needle = NULL;
lv_obj_t * ui_Screen6_rpm_label = NULL;

static lv_timer_t * ui_Screen6_needle_timer = NULL;
static float ui_Screen6_target_rpm = 0.0f;
static float ui_Screen6_displayed_rpm = 0.0f;
static float ui_Screen6_drawn_rpm = REV_RPM_UNDRAWN;  /* what's actually on screen */

// Dial angle the needle sits at for a given RPM, in degrees.
static float rev_needle_angle_deg(float rpm)
{
    if(rpm < 0.0f) rpm = 0.0f;
    if(rpm > REV_MAX_RPM) rpm = REV_MAX_RPM;
    return REV_SWEEP_START - 270.0f + REV_SWEEP_SPAN * (rpm / REV_MAX_RPM);
}

// Screen-space bounding box of the needle + hub at a given RPM.
//
// This is what keeps the gauge affordable. lv_draw_sw_vector() sizes its
// ARGB8888 scratch buffer from the layer being rendered, which in partial
// mode is exactly the invalidated area (lv_refr.c:870) — and it allocates,
// memzero-clears and alpha-blends that whole rectangle every frame. The
// needle only ever covers a thin diagonal sliver of the 380px dial, so
// invalidating its real bounds instead of the whole object cuts that work by
// roughly an order of magnitude.
static void rev_needle_bbox(float rpm, const lv_area_t * obj_coords, lv_area_t * out)
{
    float a = rev_needle_angle_deg(rpm) * (float)M_PI / 180.0f;
    float c = cosf(a);
    float s = sinf(a);

    /* Seed with the hub, which is drawn at the pivot whatever the angle */
    float min_x = -REV_HUB_R, max_x = REV_HUB_R;
    float min_y = -REV_HUB_R, max_y = REV_HUB_R;

    for(uint32_t i = 0; i < REV_NEEDLE_PT_CNT; i++) {
        float px = REV_NEEDLE_PTS[i][0];
        float py = REV_NEEDLE_PTS[i][1];
        float x = c * px - s * py;   /* matches lv_matrix_rotate() */
        float y = s * px + c * py;
        if(x < min_x) min_x = x;
        if(x > max_x) max_x = x;
        if(y < min_y) min_y = y;
        if(y > max_y) max_y = y;
    }

    float cx = (obj_coords->x1 + obj_coords->x2) / 2.0f;
    float cy = (obj_coords->y1 + obj_coords->y2) / 2.0f;
    out->x1 = (int32_t)floorf(cx + min_x - REV_DRAW_MARGIN);
    out->y1 = (int32_t)floorf(cy + min_y - REV_DRAW_MARGIN);
    out->x2 = (int32_t)ceilf( cx + max_x + REV_DRAW_MARGIN);
    out->y2 = (int32_t)ceilf( cy + max_y + REV_DRAW_MARGIN);
}

#if LV_USE_VECTOR_GRAPHIC

// Needle glows from cool silver to hot red as RPM climbs into the redline
static lv_color_t rev_needle_color(float rpm)
{
    if(rpm <= REV_REDLINE_RPM) return lv_color_hex(0xE8ECEF);
    float t = (rpm - REV_REDLINE_RPM) / (REV_MAX_RPM - REV_REDLINE_RPM);
    if(t > 1.0f) t = 1.0f;
    uint8_t r = (uint8_t)(0xE8 + t * (0xFF - 0xE8));
    uint8_t g = (uint8_t)(0xEC + t * (0x2A - 0xEC));
    uint8_t b = (uint8_t)(0xEF + t * (0x1E - 0xEF));
    return lv_color_make(r, g, b);
}

// ThorVG needle: a tapered dagger with a counterweight tail, swept and
// colored by the current (smoothed) RPM, plus a small metal hub on top.
static void rev_needle_draw_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target_obj(e);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    float cx = (area.x1 + area.x2) / 2.0f;
    float cy = (area.y1 + area.y2) / 2.0f;

    float rpm = ui_Screen6_displayed_rpm;
    if(rpm < 0.0f) rpm = 0.0f;
    if(rpm > REV_MAX_RPM) rpm = REV_MAX_RPM;

    lv_layer_t * layer = lv_event_get_layer(e);
    lv_draw_vector_dsc_t * dsc = lv_draw_vector_dsc_create(layer);
    lv_draw_vector_dsc_identity(dsc);
    lv_draw_vector_dsc_translate(dsc, cx, cy);
    lv_draw_vector_dsc_rotate(dsc, rev_needle_angle_deg(rpm));

    // Needle body: a dagger pointing "up" (-Y) before rotation is applied
    lv_vector_path_t * needle = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_fpoint_t p = { REV_NEEDLE_PTS[0][0], REV_NEEDLE_PTS[0][1] };
    lv_vector_path_move_to(needle, &p);
    for(uint32_t i = 1; i < REV_NEEDLE_PT_CNT; i++) {
        p.x = REV_NEEDLE_PTS[i][0];
        p.y = REV_NEEDLE_PTS[i][1];
        lv_vector_path_line_to(needle, &p);
    }
    lv_vector_path_close(needle);

    lv_draw_vector_dsc_set_fill_color(dsc, rev_needle_color(rpm));
    lv_draw_vector_dsc_set_fill_opa(dsc, LV_OPA_COVER);
    lv_draw_vector_dsc_set_stroke_color(dsc, lv_color_hex(0x141414));
    lv_draw_vector_dsc_set_stroke_width(dsc, 2.5f);
    lv_draw_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_ROUND);
    lv_draw_vector_dsc_add_path(dsc, needle);
    lv_vector_path_delete(needle);

    // Hub: dark outer ring + bright pivot highlight
    lv_fpoint_t hub_c = { 0.0f, 0.0f };

    lv_vector_path_t * hub_outer = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_append_circle(hub_outer, &hub_c, REV_HUB_R, REV_HUB_R);
    lv_draw_vector_dsc_set_fill_color(dsc, lv_color_hex(0x2b2f33));
    lv_draw_vector_dsc_set_stroke_color(dsc, lv_color_hex(0x6a6f73));
    lv_draw_vector_dsc_set_stroke_width(dsc, 2.0f);
    lv_draw_vector_dsc_add_path(dsc, hub_outer);
    lv_vector_path_delete(hub_outer);

    lv_vector_path_t * hub_inner = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_append_circle(hub_inner, &hub_c, 9.0f, 9.0f);
    lv_draw_vector_dsc_set_fill_color(dsc, lv_color_hex(0xB9C4CC));
    lv_draw_vector_dsc_set_stroke_width(dsc, 0.0f);
    lv_draw_vector_dsc_add_path(dsc, hub_inner);
    lv_vector_path_delete(hub_inner);

    lv_draw_vector(dsc);
    lv_draw_vector_dsc_delete(dsc);
}

#endif /* LV_USE_VECTOR_GRAPHIC */

// Smooths the needle toward the latest RPM sample and repaints just the part
// of the dial it moved across — see rev_needle_bbox() for why that matters so
// much here. Runs at 30 Hz and goes quiet entirely once the needle has caught
// up with its target.
static void ui_Screen6_needle_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);

    ui_Screen6_displayed_rpm += (ui_Screen6_target_rpm - ui_Screen6_displayed_rpm) * 0.22f;
    if(fabsf(ui_Screen6_target_rpm - ui_Screen6_displayed_rpm) < 1.0f) {
        ui_Screen6_displayed_rpm = ui_Screen6_target_rpm;
    }

    if(lv_screen_active() != ui_Screen6) {
        /* Keep smoothing while off-screen so the needle is current when we come
         * back, but LVGL repaints the whole screen on load — so forget where
         * the needle was drawn rather than sweeping from a stale position. */
        ui_Screen6_drawn_rpm = REV_RPM_UNDRAWN;
        return;
    }

    bool undrawn = (ui_Screen6_drawn_rpm == REV_RPM_UNDRAWN);
    if(!undrawn && fabsf(ui_Screen6_displayed_rpm - ui_Screen6_drawn_rpm) < 2.0f) {
        return; /* not visibly moved */
    }

    float prev_rpm = ui_Screen6_drawn_rpm;
    ui_Screen6_drawn_rpm = ui_Screen6_displayed_rpm;

    static int last_shown_rpm = -1;
    int rpm_int = (int)(ui_Screen6_displayed_rpm + 0.5f);
    if(rpm_int != last_shown_rpm) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", rpm_int);
        lv_label_set_text(ui_Screen6_rpm_label, buf);
        last_shown_rpm = rpm_int;
    }

    if(undrawn) {
        lv_obj_invalidate(ui_Screen6_needle);   /* position on screen unknown */
        return;
    }

    /* Repaint the union of where the needle was and where it now is: the old
     * box lets the dial behind it come back, the new one draws the needle. */
    lv_area_t needle_coords, inv, prev_box;
    lv_obj_get_coords(ui_Screen6_needle, &needle_coords);
    rev_needle_bbox(ui_Screen6_displayed_rpm, &needle_coords, &inv);
    rev_needle_bbox(prev_rpm, &needle_coords, &prev_box);
    if(prev_box.x1 < inv.x1) inv.x1 = prev_box.x1;
    if(prev_box.y1 < inv.y1) inv.y1 = prev_box.y1;
    if(prev_box.x2 > inv.x2) inv.x2 = prev_box.x2;
    if(prev_box.y2 > inv.y2) inv.y2 = prev_box.y2;
    lv_obj_invalidate_area(ui_Screen6_needle, &inv);
}

void ui_Screen6_set_rpm(int rpm)
{
    ui_Screen6_target_rpm = (float)rpm;
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

    // Title
    ui_screen_title_create(ui_Screen6, "REV COUNTER");

    // Outer bezel — almost fills the screen height
    ui_Screen6_bezel = lv_obj_create(ui_Screen6);
    lv_obj_remove_flag(ui_Screen6_bezel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui_Screen6_bezel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_Screen6_bezel, 420, 420);
    lv_obj_set_pos(ui_Screen6_bezel, 0, 25);
    lv_obj_set_align(ui_Screen6_bezel, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Screen6_bezel, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen6_bezel, lv_color_hex(0x24282c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen6_bezel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Screen6_bezel, lv_color_hex(0x565c62), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen6_bezel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_Screen6_bezel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_Screen6_bezel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_Screen6_bezel, 140, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Inner dial face
    ui_Screen6_dial = lv_obj_create(ui_Screen6_bezel);
    lv_obj_remove_flag(ui_Screen6_dial, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui_Screen6_dial, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_Screen6_dial, 380, 380);
    lv_obj_set_align(ui_Screen6_dial, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Screen6_dial, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen6_dial, lv_color_hex(0x03060a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen6_dial, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Screen6_dial, lv_color_hex(0x1a3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen6_dial, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Tick marks — major every 2000 RPM, minor every 500 RPM. Built once as
    // small rotated rectangles rather than ThorVG paths: they never change, so
    // there's no reason to re-tessellate them inside the needle's per-frame
    // draw callback, and a plain lv_obj recomposites more cheaply than a
    // vector path does.
    for(int step = 0; step <= 28; step++) {
        float rpm = step * 500.0f;
        float theta_deg = REV_SWEEP_START + REV_SWEEP_SPAN * (rpm / REV_MAX_RPM);
        float theta = theta_deg * (float)M_PI / 180.0f;
        bool is_major = (step % 4 == 0); /* every 2000 RPM */
        float r_out = 165.0f;
        float r_in = is_major ? 142.0f : 152.0f;
        float r_mid = (r_out + r_in) / 2.0f;

        lv_obj_t * tick = lv_obj_create(ui_Screen6_dial);
        lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(tick, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(tick, is_major ? 5 : 3, (int)(r_out - r_in));
        lv_obj_set_style_radius(tick, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tick, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(tick, is_major ? lv_color_hex(0xCFE8F5) : lv_color_hex(0x3d5560),
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_rotation(tick, (int32_t)lroundf((theta_deg - 270.0f) * 10.0f),
                                             LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_pos(tick, (int)lroundf(r_mid * cosf(theta)), (int)lroundf(r_mid * sinf(theta)));
        lv_obj_set_align(tick, LV_ALIGN_CENTER);
    }

    // Static track ring (0-14000 RPM sweep, 90 deg gap at the bottom)
    ui_Screen6_track_arc = lv_arc_create(ui_Screen6_dial);
    lv_arc_set_rotation(ui_Screen6_track_arc, 135);
    lv_arc_set_bg_angles(ui_Screen6_track_arc, 0, 270);
    lv_arc_set_range(ui_Screen6_track_arc, 0, 14000);
    lv_arc_set_value(ui_Screen6_track_arc, 0);
    lv_obj_set_size(ui_Screen6_track_arc, 350, 350);
    lv_obj_set_align(ui_Screen6_track_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen6_track_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ui_Screen6_track_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen6_track_arc, lv_color_hex(0x0A92D4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen6_track_arc, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Redline zone overlay — last 5000 RPM of the sweep, painted red
    int redline_rotation = 135 + (int)lroundf(270.0f * (REV_REDLINE_RPM / REV_MAX_RPM));
    int redline_span = (int)lroundf(270.0f * ((REV_MAX_RPM - REV_REDLINE_RPM) / REV_MAX_RPM));
    ui_Screen6_redline_arc = lv_arc_create(ui_Screen6_dial);
    lv_arc_set_rotation(ui_Screen6_redline_arc, redline_rotation);
    lv_arc_set_bg_angles(ui_Screen6_redline_arc, 0, redline_span);
    lv_arc_set_range(ui_Screen6_redline_arc, 0, 100);
    lv_arc_set_value(ui_Screen6_redline_arc, 0);
    lv_obj_set_size(ui_Screen6_redline_arc, 350, 350);
    lv_obj_set_align(ui_Screen6_redline_arc, LV_ALIGN_CENTER);
    lv_obj_remove_style(ui_Screen6_redline_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ui_Screen6_redline_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_Screen6_redline_arc, lv_color_hex(0xE0332A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_Screen6_redline_arc, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Major-tick RPM labels (x1000 RPM), red once inside the redline
    for(int i = 0; i <= 7; i++) {
        float rpm = i * 2000.0f;
        float theta = (REV_SWEEP_START + REV_SWEEP_SPAN * (rpm / REV_MAX_RPM)) * (float)M_PI / 180.0f;
        float r = 118.0f;

        lv_obj_t * lbl = lv_label_create(ui_Screen6_dial);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", i * 2);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, &ui_font_DSEG20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, rpm >= REV_REDLINE_RPM ? lv_color_hex(0xE0332A) : lv_color_hex(0xCFE8F5),
                                     LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_pos(lbl, (int)lroundf(r * cosf(theta)), (int)lroundf(r * sinf(theta)));
        lv_obj_set_align(lbl, LV_ALIGN_CENTER);
    }

    // Digital RPM readout, tucked into the gap at the bottom of the dial
    ui_Screen6_rpm_label = lv_label_create(ui_Screen6_dial);
    lv_label_set_text(ui_Screen6_rpm_label, "0");
    lv_obj_set_style_text_font(ui_Screen6_rpm_label, &ui_font_DSEG48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Screen6_rpm_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_Screen6_rpm_label, 0, 132);
    lv_obj_set_align(ui_Screen6_rpm_label, LV_ALIGN_CENTER);

    lv_obj_t * rpm_caption = lv_label_create(ui_Screen6_dial);
    lv_label_set_text(rpm_caption, "RPM");
    lv_obj_set_style_text_font(rpm_caption, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(rpm_caption, lv_color_hex(0xA8CFEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(rpm_caption, 0, 168);
    lv_obj_set_align(rpm_caption, LV_ALIGN_CENTER);

    // Needle overlay — transparent, redrawn every tick via ThorVG
    ui_Screen6_needle = lv_obj_create(ui_Screen6_bezel);
    lv_obj_remove_flag(ui_Screen6_needle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui_Screen6_needle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_Screen6_needle, 380, 380);
    lv_obj_set_align(ui_Screen6_needle, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_Screen6_needle, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Screen6_needle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
#if LV_USE_VECTOR_GRAPHIC
    lv_obj_add_event_cb(ui_Screen6_needle, rev_needle_draw_cb, LV_EVENT_DRAW_MAIN_END, NULL);
#endif

    ui_Screen6_displayed_rpm = 0.0f;
    ui_Screen6_target_rpm = 0.0f;
    ui_Screen6_drawn_rpm = REV_RPM_UNDRAWN;
    ui_Screen6_needle_timer = lv_timer_create(ui_Screen6_needle_timer_cb, 33, NULL);
}

void ui_Screen6_screen_destroy(void)
{
    if(ui_Screen6_needle_timer != NULL) {
        lv_timer_delete(ui_Screen6_needle_timer);
        ui_Screen6_needle_timer = NULL;
    }

    lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
    ui_Screen6_dashboardbutton = NULL;
    ui_Screen6_menubutton = NULL;
    ui_Screen6_bezel = NULL;
    ui_Screen6_dial = NULL;
    ui_Screen6_track_arc = NULL;
    ui_Screen6_redline_arc = NULL;
    ui_Screen6_needle = NULL;
    ui_Screen6_rpm_label = NULL;
}
