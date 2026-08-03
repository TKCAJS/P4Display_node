// Screen Navigation Buttons Component

#include "ui.h"
#include "ui_scale.h"   /* 1.25x layout scale for the 1024x600 panel */

ui_screen_nav_buttons_t ui_screen_nav_buttons_create(lv_obj_t * comp_parent)
{
    ui_screen_nav_buttons_t nav;
    
    // Dashboard button at (0, 0)
    nav.dashboard_btn = ui_dashboardbutton_create(comp_parent);
    lv_obj_set_pos(nav.dashboard_btn, 0, 0);
    
    // Menu button at (720, 0)
    nav.menu_btn = ui_menubutton_create(comp_parent);
    lv_obj_set_pos(nav.menu_btn, 720, 0);
    
    return nav;
}
