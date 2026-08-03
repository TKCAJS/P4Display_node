/*
 * ui_scale.h — 1.25x layout scale for the 7" (1024x600) variant.
 *
 * WHY THIS EXISTS
 * ---------------
 * The SquareLine screens under this directory are byte-for-byte copies of the
 * ones in src/t89_ui, which were authored for the 4.3" board's logical 800x480
 * canvas. Rather than rewrite ~600 coordinate literals — which would make the
 * two trees undiffable and every future change a manual re-port — this header
 * redefines the handful of LVGL geometry setters to scale their arguments on
 * the way through. Copying a screen change from src/t89_ui to here stays a
 * plain file copy.
 *
 * WHY 1.25 AND NOT 1.28
 * ---------------------
 *   600 / 480 = 1.25 exactly.   1024 / 800 = 1.28.
 * Scaling both axes uniformly by 1.25 keeps the design's proportions and fills
 * the height exactly; the width comes out at 1000 px, leaving a 12 px black
 * margin either side. Everything on these screens is LV_ALIGN_CENTER-relative,
 * so that margin lands symmetrically with no extra work. Stretching x by 1.28
 * instead would fill the width but distort every circle and square on the
 * dashboard.
 *
 * The 7" panel's pixels are also physically ~1.26x bigger than the 4.3"
 * panel's, so a 1.25x pixel scale means UI elements end up roughly 1.6x
 * physically larger on the car display than on the test rig. That is the point.
 *
 * WHAT IS SCALED
 * --------------
 *   - object position / size / alignment offsets
 *   - style lengths: padding, radius, border, outline, line, arc, shadow, tick
 *   - images, via lv_image_set_src() also setting a 1.25x transform scale
 *   - fonts, by remapping each font name to the next size up (see below)
 *
 * WHAT IS NOT
 * -----------
 * Anything that isn't a pixel length: bar/slider/arc/scale RANGES and VALUES,
 * angles, rotations, tick counts, colours, opacities. Those keep their own
 * function names and are simply not shimmed. LV_SIZE_CONTENT and LV_PCT() pass
 * through untouched — ui_s() checks LV_COORD_IS_PX() first.
 *
 * HOW TO USE
 * ----------
 * Include it AFTER "ui.h" in each screen/component .c file:
 *
 *     #include "../ui.h"
 *     #include "../ui_scale.h"
 *
 * Deliberately NOT included from ui.h itself, because ui_helpers.c must stay
 * unscaled: its animation callbacks feed values straight back from
 * lv_obj_get_x()/get_width() into lv_obj_set_x()/set_width(), and those are
 * already in device pixels. Scaling them would compound every animation frame.
 *
 * TO TURN IT OFF (e.g. to check a layout 1:1 against the 4.3" board), set
 * UI_SCALE_NUM to 4 — everything below becomes an identity.
 */
#pragma once

#include "lvgl.h"

/* ---- the ratio ---- */
#define UI_SCALE_NUM   5
#define UI_SCALE_DEN   4

/* Design canvas these screens were drawn on, and where they land here. */
#define UI_DESIGN_W    800
#define UI_DESIGN_H    480
#define UI_SCALED_W    ((UI_DESIGN_W * UI_SCALE_NUM) / UI_SCALE_DEN)   /* 1000 */
#define UI_SCALED_H    ((UI_DESIGN_H * UI_SCALE_NUM) / UI_SCALE_DEN)   /*  600 */

/**
 * Scale one coordinate. Special LVGL coordinate encodings (LV_SIZE_CONTENT,
 * LV_PCT(), LV_COORD_MAX...) carry type bits above the value and must pass
 * through unscaled — multiplying them would produce garbage. Plain pixel
 * values, positive or negative, are LV_COORD_IS_PX and get scaled.
 */
static inline int32_t ui_s(int32_t v)
{
    return LV_COORD_IS_PX(v) ? (v * UI_SCALE_NUM) / UI_SCALE_DEN : v;
}

/* Image transform scale: LVGL's 256 == 1.0, so 1.25x == 320. */
#define UI_IMG_SCALE   ((256 * UI_SCALE_NUM) / UI_SCALE_DEN)

/* =====================================================================
 * Geometry setters.
 *
 * Each macro expands to a call to the very function it is named after. That
 * is not infinite recursion: a function-like macro is not re-expanded inside
 * its own replacement list (C11 6.10.3.4p2, the "blue paint" rule), so the
 * inner name is left alone and resolves to the real LVGL function.
 * ===================================================================== */

/* ---- position / size / alignment ---- */
#define lv_obj_set_pos(o, x, y)             lv_obj_set_pos(o, ui_s(x), ui_s(y))
#define lv_obj_set_x(o, x)                  lv_obj_set_x(o, ui_s(x))
#define lv_obj_set_y(o, y)                  lv_obj_set_y(o, ui_s(y))
#define lv_obj_set_size(o, w, h)            lv_obj_set_size(o, ui_s(w), ui_s(h))
#define lv_obj_set_width(o, w)              lv_obj_set_width(o, ui_s(w))
#define lv_obj_set_height(o, h)             lv_obj_set_height(o, ui_s(h))
#define lv_obj_align(o, a, x, y)            lv_obj_align(o, a, ui_s(x), ui_s(y))
#define lv_obj_align_to(o, b, a, x, y)      lv_obj_align_to(o, b, a, ui_s(x), ui_s(y))

/* ---- style lengths (obj, value, selector) ---- */
#define lv_obj_set_style_pad_all(o, v, s)        lv_obj_set_style_pad_all(o, ui_s(v), s)
#define lv_obj_set_style_pad_top(o, v, s)        lv_obj_set_style_pad_top(o, ui_s(v), s)
#define lv_obj_set_style_pad_bottom(o, v, s)     lv_obj_set_style_pad_bottom(o, ui_s(v), s)
#define lv_obj_set_style_pad_left(o, v, s)       lv_obj_set_style_pad_left(o, ui_s(v), s)
#define lv_obj_set_style_pad_right(o, v, s)      lv_obj_set_style_pad_right(o, ui_s(v), s)
#define lv_obj_set_style_pad_row(o, v, s)        lv_obj_set_style_pad_row(o, ui_s(v), s)
#define lv_obj_set_style_pad_column(o, v, s)     lv_obj_set_style_pad_column(o, ui_s(v), s)
#define lv_obj_set_style_pad_radial(o, v, s)     lv_obj_set_style_pad_radial(o, ui_s(v), s)
#define lv_obj_set_style_margin_all(o, v, s)     lv_obj_set_style_margin_all(o, ui_s(v), s)

#define lv_obj_set_style_border_width(o, v, s)   lv_obj_set_style_border_width(o, ui_s(v), s)
#define lv_obj_set_style_outline_width(o, v, s)  lv_obj_set_style_outline_width(o, ui_s(v), s)
#define lv_obj_set_style_outline_pad(o, v, s)    lv_obj_set_style_outline_pad(o, ui_s(v), s)
#define lv_obj_set_style_line_width(o, v, s)     lv_obj_set_style_line_width(o, ui_s(v), s)
#define lv_obj_set_style_arc_width(o, v, s)      lv_obj_set_style_arc_width(o, ui_s(v), s)
#define lv_obj_set_style_length(o, v, s)         lv_obj_set_style_length(o, ui_s(v), s)

#define lv_obj_set_style_shadow_width(o, v, s)   lv_obj_set_style_shadow_width(o, ui_s(v), s)
#define lv_obj_set_style_shadow_spread(o, v, s)  lv_obj_set_style_shadow_spread(o, ui_s(v), s)
#define lv_obj_set_style_shadow_offset_x(o, v, s) lv_obj_set_style_shadow_offset_x(o, ui_s(v), s)
#define lv_obj_set_style_shadow_offset_y(o, v, s) lv_obj_set_style_shadow_offset_y(o, ui_s(v), s)

#define lv_obj_set_style_translate_x(o, v, s)    lv_obj_set_style_translate_x(o, ui_s(v), s)
#define lv_obj_set_style_translate_y(o, v, s)    lv_obj_set_style_translate_y(o, ui_s(v), s)

#define lv_obj_set_style_max_width(o, v, s)      lv_obj_set_style_max_width(o, ui_s(v), s)
#define lv_obj_set_style_min_width(o, v, s)      lv_obj_set_style_min_width(o, ui_s(v), s)
#define lv_obj_set_style_max_height(o, v, s)     lv_obj_set_style_max_height(o, ui_s(v), s)
#define lv_obj_set_style_min_height(o, v, s)     lv_obj_set_style_min_height(o, ui_s(v), s)

#define lv_obj_set_style_text_letter_space(o, v, s) lv_obj_set_style_text_letter_space(o, ui_s(v), s)
#define lv_obj_set_style_text_line_space(o, v, s)   lv_obj_set_style_text_line_space(o, ui_s(v), s)

/* Radius: LV_RADIUS_CIRCLE is a sentinel, not a length — pass it through. */
#define lv_obj_set_style_radius(o, v, s) \
    lv_obj_set_style_radius(o, (v) == LV_RADIUS_CIRCLE ? LV_RADIUS_CIRCLE : ui_s(v), s)

/* =====================================================================
 * Images
 *
 * The artwork is raster and sized for the 800x480 canvas (the RPM strip is
 * 701 px wide, the title 800). Left alone it would sit undersized inside a
 * 1.25x layout, so setting a source also sets a 1.25x transform scale. The
 * default image pivot is the centre, and every image on these screens is
 * centre-aligned, so it grows symmetrically about its anchor.
 *
 * Pivots are NOT scaled: lv_image_set_pivot() is in SOURCE image pixels, which
 * this does not change.
 * ===================================================================== */
#define lv_image_set_src(o, src)                        \
    do {                                                \
        lv_image_set_src(o, src);                       \
        lv_image_set_scale(o, UI_IMG_SCALE);            \
    } while(0)

/* =====================================================================
 * Fonts
 *
 * Bitmap fonts can't be scaled by arithmetic, so each name is remapped to a
 * bigger cut of the same typeface.
 *
 *   - The ui_font_* family is REGENERATED at 1.25x into this directory's
 *     fonts/ (see the generator note in that folder). Same symbol names, so
 *     nothing below is needed for them — "ui_font_DSEG20" simply *is* 25 px
 *     here.
 *   - lv_font_montserrat_* comes from LVGL itself and can't be renamed, so the
 *     names are remapped to the nearest enabled larger cut. 18/24/36 had to be
 *     switched on in lv_conf_v9.h for this; 48 has no larger cut and stays.
 *
 * These are object-like macros on the identifier, so they work through the
 * `&lv_font_montserrat_20` address-of that the screens use. lvgl.h is included
 * above, so the real declarations are already in scope by this point.
 * ===================================================================== */
#define lv_font_montserrat_14   lv_font_montserrat_18   /* 14 * 1.25 = 17.5 */
#define lv_font_montserrat_20   lv_font_montserrat_24   /* 20 * 1.25 = 25   */
#define lv_font_montserrat_28   lv_font_montserrat_36   /* 28 * 1.25 = 35   */
/* montserrat_48 -> 60 would be needed but LVGL ships no 60; left at 48. */
