// Screen Navigation Buttons Component

#ifndef _UI_COMP_SCREEN_NAV_BUTTONS_H
#define _UI_COMP_SCREEN_NAV_BUTTONS_H

#include "../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t * dashboard_btn;
    lv_obj_t * menu_btn;
} ui_screen_nav_buttons_t;

ui_screen_nav_buttons_t ui_screen_nav_buttons_create(lv_obj_t * comp_parent);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
