// Vertical Slider Component with Value Display

#ifndef _UI_COMP_VERTICAL_SLIDER_H
#define _UI_COMP_VERTICAL_SLIDER_H

#include "../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t * slider;
    lv_obj_t * value_label;
} ui_vertical_slider_t;

ui_vertical_slider_t ui_vertical_slider_create(lv_obj_t * comp_parent);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
