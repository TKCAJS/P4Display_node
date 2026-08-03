// Screen 7 - RPM 2

#ifndef UI_SCREEN7_H
#define UI_SCREEN7_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen7
extern void ui_Screen7_screen_init(void);
extern void ui_Screen7_screen_destroy(void);
extern lv_obj_t * ui_Screen7;
extern lv_obj_t * ui_Screen7_dashboardbutton;
extern lv_obj_t * ui_Screen7_menubutton;

// Feed the bar tacho. Like ui_Screen6_set_rpm() this is a no-op unless Screen7
// is the screen currently on the panel, so it is safe to call every gauge tick.
void ui_Screen7_set_rpm(uint16_t rpm);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
