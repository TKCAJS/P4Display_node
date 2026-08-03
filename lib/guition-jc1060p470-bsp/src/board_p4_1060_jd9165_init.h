/*
 * SPDX-License-Identifier: MIT
 *
 * JD9165 (Jadard JD9165BA) initialisation sequence for the Guition
 * JC1060P470C-I-W-Y, 7" 1024x600 MIPI-DSI.
 *
 * ⚠️ NOT YET RUN ON HARDWARE. Transcribed verbatim from the vendor's own
 * Arduino demo for this exact model (esp_lcd_jd9165.c,
 * vendor_specific_init_default[]) — see VENDORED.md. Two deliberate
 * differences from that driver:
 *
 *   1. MADCTL (0x36) = 0x00 is prepended here. The vendor driver sends it from
 *      panel_jd9165_init() before walking its table; we have no vendor driver,
 *      so it becomes the first table entry. 0x00 = RGB element order, no
 *      mirror — matching LCD_RGB_ELEMENT_ORDER_RGB in the demo.
 *   2. The demo's `esp_lcd_panel_io_rx_param(io, 0x04, ID, 3)` ID read is
 *      dropped. It is diagnostic only, and 0x04 is also a page-1 register in
 *      the table below, so reading it is a good way to confuse the panel.
 *
 * The commands are sent MANUALLY via esp_lcd_panel_io_tx_param() — there is NO
 * esp_lcd_jd9165 vendor component in this path, exactly as the 4.3" sibling
 * BSP does it for the ST7701S. Register 0x30 is the JD9165 PAGE SELECT: the
 * meaning of every other command depends on the page last written, which is why
 * the same command byte legitimately appears many times below.
 *
 * The sequence ENDS with its own 0x11 (SLPOUT, +120ms) and 0x29 (DISPON,
 * +50ms); do NOT add them elsewhere.
 *
 * C ONLY — the table uses file-scope compound literals (C99 §6.5.2.5p5: static
 * storage duration). This is include-once into a single .c translation unit.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

/* Each entry: { dcs cmd, &params, param byte count, post-delay ms }. */
typedef struct {
    uint8_t        cmd;
    const uint8_t *data;
    size_t         len;
    uint16_t       delay_ms;
} board_p4_panel_init_cmd_t;

static const board_p4_panel_init_cmd_t board_p4_panel_init_cmds[] = {
    /* --- memory access control: RGB order, no mirror (see header notes) --- */
    {0x36, (const uint8_t[]){0x00}, 1, 0},

    /* --- page 0: unlock --- */
    {0x30, (const uint8_t[]){0x00}, 1, 0},
    {0xF7, (const uint8_t[]){0x49, 0x61, 0x02, 0x00}, 4, 0},

    /* --- page 1: panel/timing setup --- */
    {0x30, (const uint8_t[]){0x01}, 1, 0},
    {0x04, (const uint8_t[]){0x0C}, 1, 0},
    {0x05, (const uint8_t[]){0x00}, 1, 0},
    {0x06, (const uint8_t[]){0x00}, 1, 0},
    {0x0B, (const uint8_t[]){0x11}, 1, 0},
    {0x17, (const uint8_t[]){0x00}, 1, 0},
    {0x20, (const uint8_t[]){0x04}, 1, 0},
    {0x1F, (const uint8_t[]){0x05}, 1, 0},
    {0x23, (const uint8_t[]){0x00}, 1, 0},
    {0x25, (const uint8_t[]){0x19}, 1, 0},
    {0x28, (const uint8_t[]){0x18}, 1, 0},
    {0x29, (const uint8_t[]){0x04}, 1, 0},
    {0x2A, (const uint8_t[]){0x01}, 1, 0},
    {0x2B, (const uint8_t[]){0x04}, 1, 0},
    {0x2C, (const uint8_t[]){0x01}, 1, 0},

    /* --- page 2: gate/source driver waveforms --- */
    {0x30, (const uint8_t[]){0x02}, 1, 0},
    {0x01, (const uint8_t[]){0x22}, 1, 0},
    {0x03, (const uint8_t[]){0x12}, 1, 0},
    {0x04, (const uint8_t[]){0x00}, 1, 0},
    {0x05, (const uint8_t[]){0x64}, 1, 0},
    {0x0A, (const uint8_t[]){0x08}, 1, 0},
    {0x0B, (const uint8_t[]){0x0A, 0x1A, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x08, 0x1F, 0x1D}, 11, 0},
    {0x0C, (const uint8_t[]){0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}, 11, 0},
    {0x0D, (const uint8_t[]){0x16, 0x1B, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x09, 0x1E, 0x1C}, 11, 0},
    {0x0E, (const uint8_t[]){0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}, 11, 0},
    {0x0F, (const uint8_t[]){0x16, 0x1B, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1C, 0x1E, 0x09, 0x07}, 11, 0},
    {0x10, (const uint8_t[]){0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}, 11, 0},
    {0x11, (const uint8_t[]){0x0A, 0x1A, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1D, 0x1F, 0x08, 0x06}, 11, 0},
    {0x12, (const uint8_t[]){0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}, 11, 0},
    {0x14, (const uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0x18, (const uint8_t[]){0x99}, 1, 0},

    /* --- page 6: gamma --- */
    {0x30, (const uint8_t[]){0x06}, 1, 0},
    {0x12, (const uint8_t[]){0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29}, 14, 0},
    {0x13, (const uint8_t[]){0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29}, 14, 0},

    /* NOTE: the vendor demo has pages 8 and 7 commented out here. Left out
     * deliberately, so this table stays a faithful copy of what ships working. */

    /* --- page 0x0A: power / VCOM --- */
    {0x30, (const uint8_t[]){0x0A}, 1, 0},
    {0x02, (const uint8_t[]){0x4F}, 1, 0},
    {0x0B, (const uint8_t[]){0x40}, 1, 0},
    {0x12, (const uint8_t[]){0x3E}, 1, 0},
    {0x13, (const uint8_t[]){0x78}, 1, 0},

    /* --- page 0x0D --- */
    {0x30, (const uint8_t[]){0x0D}, 1, 0},
    {0x0D, (const uint8_t[]){0x04}, 1, 0},
    {0x10, (const uint8_t[]){0x0C}, 1, 0},
    {0x11, (const uint8_t[]){0x0C}, 1, 0},
    {0x12, (const uint8_t[]){0x0C}, 1, 0},
    {0x13, (const uint8_t[]){0x0C}, 1, 0},

    /* --- back to page 0, then sleep-out + display-on --- */
    {0x30, (const uint8_t[]){0x00}, 1, 0},
    {0x11, (const uint8_t[]){0x00}, 1, 120},
    {0x29, (const uint8_t[]){0x00}, 1, 50},
};

#define BOARD_P4_PANEL_INIT_CMDS_SIZE \
    (sizeof(board_p4_panel_init_cmds) / sizeof(board_p4_panel_init_cmds[0]))
