/*
 * SPDX-License-Identifier: MIT
 *
 * board_p4_1060.h — public BSP API for the Guition JC1060P470C-I-W-Y
 * (ESP32-P4): JD9165 1024x600 MIPI-DSI panel + GT911 I2C touch.
 *
 * ⚠️ NOT YET RUN ON HARDWARE — written from the vendor's Arduino demo for this
 * model plus the proven structure of the 4.3" sibling BSP. See VENDORED.md.
 *
 * RELATIONSHIP TO guition-jc4880p4-bsp (the 4.3" board)
 * ----------------------------------------------------
 * The function NAMES are deliberately identical to board_p4.h so that the two
 * src/ trees (t89_ui and t89_ui_1060) differ only in which header they include
 * and stay easy to diff against each other. The two BSPs are never linked
 * together: each PlatformIO env includes exactly one of board_p4.h /
 * board_p4_1060.h, and the LDF pulls in only that library.
 *
 * THE BIG DIFFERENCE: NO ROTATION.
 * The 4.3" glass is wired portrait (480x800), so that BSP renders a logical
 * 800x480 landscape frame and PPA-rotates every dirty region into the native
 * framebuffer. This 7" glass is natively LANDSCAPE 1024x600, so logical and
 * native coordinates are the same thing. There is no PPA client, no rotation
 * angle, and no board_p4_present_rotated() / board_p4_flush_region_rotated()
 * here — the plain board_p4_flush_region() + board_p4_present() pair is the
 * whole flush path. Touch coordinates need no remapping either.
 *
 * Display bring-up follows the same manual recipe as the sibling BSP:
 * LDO -> dsi_bus -> dbi -> dpi -> panel reset -> PANEL_INIT_CMDS ->
 * panel_init -> backlight. NO esp_lcd_jd9165 vendor component. The DPI panel is
 * DOUBLE-buffered (num_fbs=2): LVGL renders into the off-screen BACK buffer,
 * then board_p4_present() ping-pongs it to the scanned buffer so the panel only
 * ever shows COMPLETE frames.
 *
 * Implemented in C (board_p4_1060.c) so the esp_lcd headers parse in C mode,
 * dodging the C++ overload conflict in esp_lcd_io_i2c.h.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Native panel geometry (landscape, as the glass is wired) ---- */
#define BOARD_P4_LCD_H_RES   1024
#define BOARD_P4_LCD_V_RES   600

/* ---- Pins / LDO channels (defaults; override any with -DBOARD_P4_xxx=...) ---- */
#include "board_p4_1060_pins.h"

/* ===========================================================================
 * Display
 * ===========================================================================*/

/**
 * @brief Bring up the MIPI-DSI panel with the manual JD9165 recipe:
 *        acquire DSI-PHY LDO -> dsi_bus (2 lanes, 750Mbps) -> DBI io ->
 *        DPI panel (1024x600, RGB565, 2 FBs, 56MHz) -> GPIO panel reset ->
 *        PANEL_INIT_CMDS -> esp_lcd_panel_init() -> backlight on.
 * @return ESP_OK on success. Heavy ESP_LOG output on every step.
 */
esp_err_t board_p4_display_init(void);

/** @brief esp_lcd panel handle from board_p4_display_init(), or NULL. */
esp_lcd_panel_handle_t board_p4_get_panel_handle(void);

/**
 * @brief Pointer to the current BACK DPI framebuffer (RGB565, the off-screen one
 *        of the num_fbs=2 pair), H_RES*V_RES uint16_t. NULL until
 *        board_p4_display_init() succeeds. Write directly then flush via
 *        board_p4_flush_region(); the value changes after each board_p4_present()
 *        swap, so re-read it rather than caching it.
 */
uint16_t *board_p4_get_framebuffer(void);

/**
 * @brief Pointer to the CURRENTLY-SCANNED (front) framebuffer — the pixels the
 *        DPI is displaying (1024x600 RGB565, 2 B/px). Sets *w/*h to the panel
 *        dims. Cache-invalidates (M2C) the frame before returning so the CPU
 *        sees what actually landed in PSRAM. NULL before init. Intended for dev
 *        tooling (serial framebuffer dump); unlike the 4.3" board the image
 *        needs NO un-rotation on the host.
 */
const uint16_t *board_p4_front_fb(int *w, int *h);

/**
 * @brief Copy an RGB565 source rectangle into the BACK framebuffer at (x,y,w,h)
 *        honouring the FB stride, then esp_cache_msync() just that row band so
 *        the DPI scan-out sees it. This is what the LVGL flush_cb calls.
 * @param x,y    top-left of the destination rect (landscape coords).
 * @param w,h    rect size in pixels.
 * @param src    w*h contiguous RGB565 pixels (row-major, stride == w).
 */
void board_p4_flush_region(int x, int y, int w, int h, const uint16_t *src);

/**
 * @brief Present the completed back framebuffer: swap it to be the one the DPI
 *        bridge scans out (ping-pong), then make the other buffer the new back
 *        buffer and re-sync it so partial-mode LVGL refreshes stay coherent.
 *        Call this exactly ONCE per LVGL refresh, on the LAST flush part
 *        (lv_display_flush_is_last()), AFTER all board_p4_flush_region() calls
 *        for that refresh. This is what makes the panel only ever scan COMPLETE
 *        frames.
 */
void board_p4_present(void);

/** @brief Push an RGB565 bitmap to a rectangle via the panel draw path. */
esp_err_t board_p4_draw_bitmap(int x1, int y1, int x2, int y2, const void *data);

/** @brief Turn the backlight on (full PWM duty) / off (0). No-op if BL pin is -1. */
void board_p4_backlight(bool on);

/**
 * @brief Set backlight brightness via LEDC PWM on BOARD_P4_LCD_BL.
 * @param duty 8-bit PWM duty (0..255). 0 = off; any non-zero value is clamped up
 *             to a ~10% floor so the panel never goes fully black. Lazily inits
 *             the LEDC timer/channel on first call. No-op if BL pin is -1.
 */
void board_p4_set_brightness(uint8_t duty);

/* ===========================================================================
 * Touch (GT911)
 * ===========================================================================*/

/**
 * @brief Bring up I2C + GT911. Probes 0x5D then 0x14 and uses whichever ACKs.
 * @return ESP_OK if a GT911 was found and initialised.
 */
esp_err_t board_p4_touch_init(void);

/** @brief esp_lcd_touch handle from board_p4_touch_init(), or NULL. */
esp_lcd_touch_handle_t board_p4_get_touch_handle(void);

/**
 * @brief Read one touch point.
 * @param x,y      filled with landscape (1024x600) coords when pressed — the
 *                 same space LVGL uses, no remapping needed.
 * @param pressed  set true if a finger is down. May be NULL.
 * @return true if a finger is down (same as *pressed).
 */
bool board_p4_touch_read(int *x, int *y, bool *pressed);

#ifdef __cplusplus
}
#endif
