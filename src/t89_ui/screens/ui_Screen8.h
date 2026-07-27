// Screen 8 - RPM 3

#ifndef UI_SCREEN8_H
#define UI_SCREEN8_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen8
extern void ui_Screen8_screen_init(void);
extern void ui_Screen8_screen_destroy(void);
extern lv_obj_t * ui_Screen8;
extern lv_obj_t * ui_Screen8_dashboardbutton;
extern lv_obj_t * ui_Screen8_menubutton;

// Feed the half-dial. Smooths internally and is a no-op unless Screen8 is the
// screen currently on the panel, so it is safe to call every gauge tick.
void ui_Screen8_set_rpm(uint16_t rpm);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
