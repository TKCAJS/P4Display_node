#include "GaugeUpdater.h"
#include "lvgl_glue.h"
#include <esp_heap_caps.h>
#include "screens/ui_Screen4.h"
#include "screens/ui_Screen5.h"

// Screen4 target slider (C code) -> pump target temp; DisplayCan re-sends it
// to the sensor node every second from the protocol task.
extern "C" void ui_pump_target_changed(int32_t targetC) {
    canSetPumpTarget((uint8_t)targetC);
}

static unsigned long last_temp_update = 0;
static unsigned long last_fuel_update = 0;
static unsigned long last_rpm_update = 0;
static unsigned long last_gear_update = 0;
static uint8_t last_gear = GEAR_UNKNOWN;

// Helper function for threshold-based LED pairs
static void updateLEDPair(uint16_t rpmValue, uint16_t threshold, lv_obj_t* led_left, lv_obj_t* led_right, bool* is_active) {
    if (rpmValue >= threshold && !*is_active) {
        lv_led_on(led_left);
        lv_led_on(led_right);
        *is_active = true;
    } else if (rpmValue < threshold && *is_active) {
        lv_led_off(led_left);
        lv_led_off(led_right);
        *is_active = false;
    }
}

// LED flash state (12k RPM) - independent timer
static unsigned long last_led_flash_update = 0;
static bool led_12k_active = false;
static bool led_12k_flash_state = false;

static void updateLEDFlash(uint16_t rpmValue) {
    const uint32_t LED_FLASH_MS = 100;
    unsigned long now = millis();

    if (now - last_led_flash_update >= LED_FLASH_MS) {
        if (rpmValue >= 11500) {
            if (!led_12k_active) {
                led_12k_active = true;
            }
            led_12k_flash_state = !led_12k_flash_state;
            if (led_12k_flash_state) {
                lv_led_on(ui_Button1);
            } else {
                lv_led_off(ui_Button1);
            }
        } else {
            if (led_12k_active) {
                lv_led_off(ui_Button1);
                led_12k_active = false;
                led_12k_flash_state = false;
            }
        }
        last_led_flash_update = now;
    }
}

void updateGauges() {
    unsigned long now = millis();

    static unsigned long lastHb = 0;
    if (now - lastHb >= 2000) {
        CanSnapshot hb = canGetSnapshot();
        Serial.printf("[HB] valid=%d gear=%d rpm=%d temp=%.1f pump=%d stack=%d "
                      "lvglStackFree=%u internalFree=%u\n",
                      hb.valid, hb.gear, hb.rpm, hb.radiatorTemp, hb.pumpDuty, hb.stackTarget,
                      lvgl_glue_stack_high_water(),
                      (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        lastHb = now;
    }

    CanSnapshot can = canGetSnapshot();

    if (!can.valid) {
        return;
    }

    // Update temperature gauge (radiator temp from sensor node Dallas DS18B20)
    // and the cooling-page engine/rad-out temp (NTC) and pump duty arcs.
    //
    // All cooling-page updates are hysteresis-gated: sensor noise flipping a
    // value across a rounding boundary (±0.1 °C, ±1 % duty) would otherwise
    // repaint a full anti-aliased arc + DSEG48 label every tick, and those
    // PSRAM framebuffer bursts visibly jitter the RGB panel.
    if (now - last_temp_update >= TEMP_UPDATE_MS) {
        static float shown_radiator = -1000.0f;
        if (fabsf(can.radiatorTemp - shown_radiator) >= 0.7f) {
            shown_radiator = can.radiatorTemp;
            lv_bar_set_value(ui_gaugetemp, (int)can.radiatorTemp, LV_ANIM_OFF);
            lv_bar_set_value(ui_Screen4_temp_gauge, (int)can.radiatorTemp, LV_ANIM_OFF);
            int radiatorC = (int)(can.radiatorTemp + 0.5f);
            lv_arc_set_value(ui_Screen4_radiator_arc, radiatorC);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", radiatorC);
            lv_label_set_text(ui_Screen4_radiator_label, buf);
        }

        static float shown_engine = -1000.0f;
        if (fabsf(can.engineTemp - shown_engine) >= 0.7f) {
            shown_engine = can.engineTemp;
            int engineC = (int)(can.engineTemp + 0.5f);
            lv_arc_set_value(ui_Screen4_engine_arc, engineC);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", engineC);
            lv_label_set_text(ui_Screen4_engine_label, buf);
        }

        static float shown_radout = -1000.0f;
        if (fabsf(can.radOutTemp - shown_radout) >= 0.7f) {
            shown_radout = can.radOutTemp;
            int radOutC = (int)(can.radOutTemp + 0.5f);
            lv_arc_set_value(ui_Screen4_radout_arc, radOutC);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", radOutC);
            lv_label_set_text(ui_Screen4_radout_label, buf);
        }

        static int shown_pump_duty = INT_MIN;
        if (abs((int)can.pumpDuty - shown_pump_duty) >= 2) {
            shown_pump_duty = can.pumpDuty;
            lv_arc_set_value(ui_Screen4_pump_arc, can.pumpDuty);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", can.pumpDuty);
            lv_label_set_text(ui_Screen4_pump_label, buf);
        }
        last_temp_update = now;
    }

    // Update fuel gauge (oil temp)
    if (now - last_fuel_update >= FUEL_UPDATE_MS) {
        lv_bar_set_value(ui_gaugefuel1, (int)can.oilTemp, LV_ANIM_OFF);
        last_fuel_update = now;
    }

    // Update RPM gauge
    if (now - last_rpm_update >= RPM_UPDATE_MS) {
        uint16_t rpmgaugevalue = can.rpm;

        lv_slider_set_value(ui_gaugerpm, rpmgaugevalue, LV_ANIM_OFF);
        ui_Screen6_set_rpm(rpmgaugevalue);

        static uint8_t rpm_color_state = 0; // 0=blue, 1=purple, 2=red
        uint8_t new_state = (rpmgaugevalue >= 12000) ? 2 : (rpmgaugevalue >= 10000) ? 1 : 0;
        if (new_state != rpm_color_state) {
            if      (new_state == 2) lv_obj_set_style_bg_color(ui_gaugerpm, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
            else if (new_state == 1) lv_obj_set_style_bg_color(ui_gaugerpm, lv_color_hex(0xD569E6), LV_PART_INDICATOR);
            else                     lv_obj_set_style_bg_color(ui_gaugerpm, lv_color_hex(0x0EC7F0), LV_PART_INDICATOR);
            rpm_color_state = new_state;
        }

        static bool stage_1 = false;
        updateLEDPair(rpmgaugevalue, 6000, ui_Button7, ui_Button5, &stage_1);

        static bool stage_2 = false;
        updateLEDPair(rpmgaugevalue, 8000, ui_Button4, ui_Button6, &stage_2);

        static bool stage_3 = false;
        updateLEDPair(rpmgaugevalue, 10000, ui_Button2, ui_Button3, &stage_3);

        updateLEDFlash(rpmgaugevalue);
        last_rpm_update = now;
    }

    // Update gear indicator and stacked downshift display
    if (now - last_gear_update >= RPM_UPDATE_MS) {
        uint8_t gear        = can.gear;
        uint8_t stackTarget = can.stackTarget;

        if (gear != last_gear) {
            if (gear == GEAR_NEUTRAL) {
                lv_label_set_text(ui_gearnum, "N");
            } else if (gear >= GEAR_1 && gear <= GEAR_6) {
                char gearStr[2];
                snprintf(gearStr, sizeof(gearStr), "%d", gear);
                lv_label_set_text(ui_gearnum, gearStr);
            } else if (gear == GEAR_BETWEEN) {
                lv_label_set_text(ui_gearnum, "-");
            } else {
                lv_label_set_text(ui_gearnum, "?");
            }
            last_gear = gear;
        }

        // Stacked gears: only update when gear is settled (not BETWEEN/unknown).
        // Always show or clear — never leave a stale value on screen.
        if (gear >= GEAR_1 && gear <= GEAR_6) {
            if (stackTarget > 0 && stackTarget < gear) {
                char buf[16] = {};
                int pos = 0;
                for (uint8_t g = stackTarget; g < gear; g++) {
                    if (pos > 0) buf[pos++] = ' ';
                    buf[pos++] = '0' + g;
                }
                lv_label_set_text(ui_stackedgears, buf);
            } else {
                lv_label_set_text(ui_stackedgears, "");
            }
        }

        last_gear_update = now;
    }

    // Update node versions on Screen5 (2s interval, only when labels exist)
    static unsigned long lastVersionUpdate = 0;
    if (now - lastVersionUpdate >= 2000) {
        if (ui_Screen5_versionMain && can.versionMain > 0) {
            char buf[24];
            snprintf(buf, sizeof(buf), "Main: v%d", can.versionMain);
            lv_label_set_text(ui_Screen5_versionMain, buf);
        }
        if (ui_Screen5_versionRear && can.versionRear > 0) {
            char buf[24];
            snprintf(buf, sizeof(buf), "Rear: v%d", can.versionRear);
            lv_label_set_text(ui_Screen5_versionRear, buf);
        }
        lastVersionUpdate = now;
    }
}
