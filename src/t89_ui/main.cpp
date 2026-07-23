/*
 * t89_ui — the real T89 dashboard (SquareLine screens, DSEG fonts,
 * GaugeUpdater) on the Guition JC4880P443C ESP32-P4, driven by the LVGL 9 glue
 * (PPA hardware-rotate partial rendering, logical 800x480 landscape).
 *
 * CAN is stubbed (see DisplayCan.h here): the snapshot animates from the
 * sensors.h simulators, so everything runs without a transceiver wired.
 * SD logging (SD_MMC) and pit mode (SoftAP + log viewer, C6 via ESP-Hosted)
 * are the real ported modules.
 *
 * Task layout: the glue's service task (core 0) drives lv_timer_handler();
 * Arduino loop() (core 1) services gauges/logger/pit server, mirroring the
 * S3 main.cpp's protocolTask+displayTask split.
 */

#include <Arduino.h>
#include "board_p4.h"
#include "lvgl_glue.h"
#include "ui.h"
#include "GaugeUpdater.h"
#include "SDLogger.h"
#include "PitServer.h"

// Hold anywhere on the screen for 3 s to toggle pit mode
#define PIT_LONGPRESS_MS 3000

static void pitOverlaySync(lv_obj_t** label) {
    bool on = pitServerIsActive();
    if (on && !*label) {
        *label = lv_label_create(lv_layer_top());
        lv_label_set_text(*label,
            "PIT MODE  -  logging paused\n"
            "WiFi: " PIT_AP_SSID "  pass: " PIT_AP_PASS "\n"
            "http://" PIT_AP_IP_STR "/   (hold 3s to exit)");
        lv_obj_set_style_bg_color(*label, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(*label, LV_OPA_80, 0);
        lv_obj_set_style_text_color(*label, lv_color_white(), 0);
        lv_obj_set_style_pad_all(*label, 10, 0);
        lv_obj_set_style_radius(*label, 6, 0);
        lv_obj_align(*label, LV_ALIGN_TOP_MID, 0, 8);
    } else if (!on && *label) {
        lv_obj_delete(*label);
        *label = nullptr;
    }
}

// Long-press detection — call under the LVGL lock
static void pitLongPressCheck(void) {
    static unsigned long pressStart = 0;
    static bool longPressFired = false;

    bool pressed = false;
    lv_indev_t* indev = lv_indev_get_next(NULL);
    if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER &&
        lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) {
        pressed = true;
    }

    unsigned long now = millis();
    if (pressed) {
        if (pressStart == 0) pressStart = now;
        else if (!longPressFired && now - pressStart >= PIT_LONGPRESS_MS) {
            pitServerRequestToggle();
            longPressFired = true;
        }
    } else {
        pressStart = 0;
        longPressFired = false;
    }
}

void setup(void) {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[T89-P4] T89 dashboard UI port (stubbed CAN, SD_MMC log, pit mode)");

    if (board_p4_display_init() != ESP_OK) {
        Serial.println("[T89-P4] board_p4_display_init FAILED");
        return;
    }
    if (!lvgl_glue_start(true /* service task drives lv_timer_handler */)) {
        Serial.println("[T89-P4] lvgl_glue_start FAILED");
        return;
    }

    if (lvgl_glue_lock(0)) {
        ui_init();
        lvgl_glue_unlock();
    }

    sdLoggerBegin();
    canBegin();
    Serial.println("[T89-P4] up — long-press 3s (or send 'p' on serial) for pit mode");
}

void loop(void) {
    // Serial 'p' mirrors the 3 s long-press — lets pit mode be exercised from
    // the host without touching the screen.
    while (Serial.available()) {
        if (Serial.read() == 'p') pitServerRequestToggle();
    }

    canUpdate();
    sdLoggerUpdate();
    pitServerService();

    static lv_obj_t* pitLabel = nullptr;
    if (lvgl_glue_lock(0)) {
        updateGauges();
        pitLongPressCheck();
        pitOverlaySync(&pitLabel);
        lvgl_glue_unlock();
    }
    delay(5);
}
