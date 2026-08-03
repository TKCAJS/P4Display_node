// Screen 5 - Settings

#include "ui.h"
#include "ui_scale.h"   /* 1.25x layout scale for the 1024x600 panel */
#include "PitServer.h"

lv_obj_t * ui_Screen5;
lv_obj_t * ui_Screen5_pit_switch = NULL;
lv_obj_t * ui_Screen5_dashboardbutton = NULL;
lv_obj_t * ui_Screen5_menubutton = NULL;
lv_obj_t * ui_Screen5_versionMain    = NULL;
lv_obj_t * ui_Screen5_versionRear    = NULL;
lv_obj_t * ui_Screen5_versionDisplay = NULL;
ui_vertical_slider_t ui_Screen5_slider_a = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_b = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_c = {NULL, NULL};
ui_vertical_slider_t ui_Screen5_slider_d = {NULL, NULL};

// PIT MODE toggle.
//
// Switching on pauses SD logging and raises a SoftAP, so it confirms first and
// shows what to connect to. Switching off only resumes logging, so it just goes.
//
// The switch is not the source of truth: pitServerRequestToggle() only queues
// the change, which pitServerService() applies from the protocol task a moment
// later. The switch is held where it was until ui_Screen5_set_pit_state()
// reports what actually happened, so a failed or cancelled toggle can't leave it
// showing a state the node isn't in.
static lv_obj_t * pit_confirm_box = NULL;

// Deliberately hand-built rather than lv_msgbox.
//
// lv_msgbox's modal backdrop and card need LVGL to render them through a layer,
// and layer buffers come from the LVGL pool via lv_draw_buf_create() — one
// LV_DRAW_LAYER_SIMPLE_BUF_SIZE (48 KB) block. That failed outright when the
// pool was 64 KB with ~24 KB free, and LVGL's response to a failure is
// "Allocating layer buffer failed. Try later" — it retries forever, so
// draw_buf_flush() spins waiting for a draw task that can never complete and the
// task watchdog reboots the node. The pool is 128 KB again as of 2026-07-27, but
// a 48 KB contiguous block is still not something to bet a reboot on, so this
// stays hand-built. Everything below is fully opaque with no
// shadow or transform, so it is drawn straight into the frame buffer and never
// asks for a layer.
static void pit_dialog_dismiss(void)
{
    // Deleting from inside a click event on one of its own children would free
    // the tree the event is still walking.
    if (pit_confirm_box) lv_obj_delete_async(pit_confirm_box);
    pit_confirm_box = NULL;
}

static void pit_confirm_enable_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    pitServerRequestToggle();
    pit_dialog_dismiss();
}

static void pit_confirm_cancel_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    pit_dialog_dismiss();
}

static lv_obj_t * pit_dialog_button(lv_obj_t * parent, const char * text,
                                    int32_t x, lv_event_cb_t cb)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_size(btn, 150, 54);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x, -16);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333A48), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);
    return btn;
}

static void pit_switch_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target_obj(e);

    if (!lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        pitServerRequestToggle();
        return;
    }

    // Deliberately NOT forcing the switch back off here: changing a switch's own
    // state from inside its LV_EVENT_VALUE_CHANGED re-enters the widget while it
    // is still running the knob animation for this very click. The switch is
    // left where the user put it and ui_Screen5_set_pit_state(), which main's
    // loop calls every few ms, pulls it back if the toggle is cancelled.
    if (pit_confirm_box) return;

    pit_confirm_box = lv_obj_create(ui_Screen5);
    lv_obj_set_size(pit_confirm_box, 540, 300);
    lv_obj_center(pit_confirm_box);
    lv_obj_remove_flag(pit_confirm_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(pit_confirm_box, lv_color_hex(0x11151C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(pit_confirm_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pit_confirm_box, lv_color_hex(0x4A5262), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pit_confirm_box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(pit_confirm_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(pit_confirm_box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * title = lv_label_create(pit_confirm_box);
    lv_label_set_text(title, "PIT MODE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t * body = lv_label_create(pit_confirm_box);
    lv_label_set_text(body,
                      "SD logging pauses while pit mode is on.\n\n"
                      "WiFi:  " PIT_AP_SSID "\n"
                      "Pass:  " PIT_AP_PASS "\n"
                      "http://" PIT_AP_IP_STR "/");
    lv_obj_set_style_text_color(body, lv_color_hex(0xD8DCE6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 6, 52);

    pit_dialog_button(pit_confirm_box, "ENABLE", -85, pit_confirm_enable_cb);
    pit_dialog_button(pit_confirm_box, "CANCEL",  85, pit_confirm_cancel_cb);
}

void ui_Screen5_set_pit_state(bool active)
{
    if (ui_Screen5_pit_switch == NULL) return;
    if (active) lv_obj_add_state(ui_Screen5_pit_switch, LV_STATE_CHECKED);
    else        lv_obj_remove_state(ui_Screen5_pit_switch, LV_STATE_CHECKED);
}

void ui_Screen5_screen_init(void)
{
    ui_Screen5 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen5, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen5, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create navigation buttons
    ui_screen_nav_buttons_t nav = ui_screen_nav_buttons_create(ui_Screen5);
    ui_Screen5_dashboardbutton = nav.dashboard_btn;
    ui_Screen5_menubutton = nav.menu_btn;

    // Title
    ui_screen_title_create(ui_Screen5, "SETTINGS");

    // Slider A
    ui_Screen5_slider_a = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_a.slider, -225, 40);
    lv_obj_set_align(ui_Screen5_slider_a.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_a.value_label, -225);

    // Slider B
    ui_Screen5_slider_b = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_b.slider, -75, 40);
    lv_obj_set_align(ui_Screen5_slider_b.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_b.value_label, -75);

    // Slider C
    ui_Screen5_slider_c = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_c.slider, 75, 40);
    lv_obj_set_align(ui_Screen5_slider_c.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_c.value_label, 75);

    // Slider D
    ui_Screen5_slider_d = ui_vertical_slider_create(ui_Screen5);
    lv_obj_set_pos(ui_Screen5_slider_d.slider, 225, 40);
    lv_obj_set_align(ui_Screen5_slider_d.slider, LV_ALIGN_CENTER);
    lv_obj_set_x(ui_Screen5_slider_d.value_label, 225);

    // PIT MODE switch — bottom-centre, clear of the sliders above, with its
    // label centred on the same axis
    lv_obj_t * pit_label = lv_label_create(ui_Screen5);
    lv_label_set_text(pit_label, "PIT MODE (WiFi)");
    lv_obj_set_style_text_color(pit_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pit_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pit_label, LV_ALIGN_BOTTOM_MID, 0, -66);

    ui_Screen5_pit_switch = lv_switch_create(ui_Screen5);
    lv_obj_set_size(ui_Screen5_pit_switch, 80, 40);
    lv_obj_align(ui_Screen5_pit_switch, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(ui_Screen5_pit_switch, pit_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    ui_Screen5_set_pit_state(pitServerIsActive());

    // Node versions — bottom-right corner
    ui_Screen5_versionMain = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionMain, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionMain, "Main: --");
    lv_obj_align(ui_Screen5_versionMain, LV_ALIGN_BOTTOM_RIGHT, -10, -60);

    ui_Screen5_versionRear = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionRear, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionRear, "Rear: --");
    lv_obj_align(ui_Screen5_versionRear, LV_ALIGN_BOTTOM_RIGHT, -10, -40);

    ui_Screen5_versionDisplay = lv_label_create(ui_Screen5);
    lv_obj_set_style_text_color(ui_Screen5_versionDisplay, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Screen5_versionDisplay, "Display: " DISPLAY_NODE_VERSION);
    lv_obj_align(ui_Screen5_versionDisplay, LV_ALIGN_BOTTOM_RIGHT, -10, -20);
}

void ui_Screen5_screen_destroy(void)
{
    lv_obj_del(ui_Screen5);
    ui_Screen5 = NULL;
    ui_Screen5_dashboardbutton = NULL;
    ui_Screen5_menubutton = NULL;
    ui_Screen5_slider_a.slider = NULL;
    ui_Screen5_slider_a.value_label = NULL;
    ui_Screen5_slider_b.slider = NULL;
    ui_Screen5_slider_b.value_label = NULL;
    ui_Screen5_slider_c.slider = NULL;
    ui_Screen5_slider_c.value_label = NULL;
    ui_Screen5_slider_d.slider = NULL;
    ui_Screen5_slider_d.value_label = NULL;
    ui_Screen5_versionMain    = NULL;
    ui_Screen5_versionRear    = NULL;
    ui_Screen5_versionDisplay = NULL;
    ui_Screen5_pit_switch     = NULL;
    pit_confirm_box           = NULL;   // a child of the screen, already deleted with it
}
