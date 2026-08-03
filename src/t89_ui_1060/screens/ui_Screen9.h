// Screen 9 - RPM 4 (antique)

#ifndef UI_SCREEN9_H
#define UI_SCREEN9_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen9
extern void ui_Screen9_screen_init(void);
extern void ui_Screen9_screen_destroy(void);
extern lv_obj_t * ui_Screen9;
extern lv_obj_t * ui_Screen9_dashboardbutton;
extern lv_obj_t * ui_Screen9_menubutton;

// Feed the antique dial. Smooths internally and is a no-op unless Screen9 is
// the screen currently on the panel, so it is safe to call every gauge tick.
void ui_Screen9_set_rpm(uint16_t rpm);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
