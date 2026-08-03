// Screen 5

#ifndef UI_SCREEN5_H
#define UI_SCREEN5_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen5
extern void ui_Screen5_screen_init(void);
extern void ui_Screen5_screen_destroy(void);
extern lv_obj_t * ui_Screen5;
extern lv_obj_t * ui_Screen5_dashboardbutton;
extern lv_obj_t * ui_Screen5_menubutton;
extern lv_obj_t * ui_Screen5_versionMain;
extern lv_obj_t * ui_Screen5_versionRear;
extern lv_obj_t * ui_Screen5_versionDisplay;
extern lv_obj_t * ui_Screen5_pit_switch;

// Drive the PIT MODE switch from the server's real state. Call under the LVGL
// lock; safe before the screen exists.
void ui_Screen5_set_pit_state(bool active);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
