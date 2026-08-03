// Screen 6 - Test

#ifndef UI_SCREEN6_H
#define UI_SCREEN6_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen6
extern void ui_Screen6_screen_init(void);
extern void ui_Screen6_screen_destroy(void);
extern lv_obj_t * ui_Screen6;
extern lv_obj_t * ui_Screen6_dashboardbutton;
extern lv_obj_t * ui_Screen6_menubutton;

// Feed the rev counter. Smooths internally and is a no-op unless Screen6 is the
// screen currently on the panel, so it is safe to call every gauge tick.
void ui_Screen6_set_rpm(uint16_t rpm);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
