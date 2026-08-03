/*
 * SPDX-License-Identifier: MIT
 *
 * board_p4_1060_pins.h — GPIO / LDO map for the Guition JC1060P470C-I-W-Y
 * (module JC-ESP32P4-M3, ESP32-P4 + ESP32-C6), 7" 1024x600 JD9165 MIPI-DSI.
 *
 * ⚠️ NOT YET VERIFIED ON HARDWARE. The display + touch pins come from the
 * vendor's own Arduino demo (pins_config.h / jd9165_lcd.cpp) shipped for this
 * exact model — see VENDORED.md for the source — so they are as good as
 * datasheet, but nothing here has been run on a real board. The SD, C6-SDIO,
 * audio and UART groups are NOT in the vendor demo at all: they are carried
 * over unchanged from the 4.3" JC4880P443 sibling on the assumption that the
 * shared JC-ESP32P4-M3 module wires them the same way. Treat those as a
 * starting guess and re-check against the schematic when the board arrives.
 *
 * Every pin is an overridable #ifndef default — pass -DBOARD_P4_xxx=<gpio> in
 * build_flags if the schematic differs.
 */
#pragma once

/* ==================== Display — JD9165, MIPI-DSI (vendor demo) ============= */
/* Differs from the 4.3" board: reset moved 5 -> 27. Backlight stayed on 23. */
#ifndef BOARD_P4_LCD_RST          /* panel reset. -1 = none. */
#define BOARD_P4_LCD_RST      27
#endif
#ifndef BOARD_P4_LCD_BL           /* backlight, driven by LEDC PWM (5kHz/8-bit). -1 = skip. */
#define BOARD_P4_LCD_BL       23
#endif
/* DSI is 2 data lanes @ 750 Mbps, DPI 56 MHz (h 40/160/160, v 10/23/12); the
 * lanes are MIPI-DSI PHY pins, not GPIOs. PHY is powered by the LDO below. */

/* ==================== On-chip LDO regulator channels ====================== */
#ifndef BOARD_P4_DSI_LDO_CHAN     /* DSI-PHY supply (VDD_MIPI_DPHY = LDO_VO3) */
#define BOARD_P4_DSI_LDO_CHAN 3
#endif
#ifndef BOARD_P4_DSI_LDO_MV
#define BOARD_P4_DSI_LDO_MV   2500
#endif
#ifndef BOARD_P4_SD_LDO_CHAN      /* TF_VCC rail — ASSUMED same as the 4.3" board */
#define BOARD_P4_SD_LDO_CHAN  4
#endif
#ifndef BOARD_P4_SD_LDO_MV
#define BOARD_P4_SD_LDO_MV    3300
#endif

/* ==================== Touch — GT911, I2C (vendor demo) ==================== */
/* Differs from the 4.3" board: RST 3 -> 22, and INT is a real pin (21), not -1. */
#ifndef BOARD_P4_TOUCH_SDA
#define BOARD_P4_TOUCH_SDA    7
#endif
#ifndef BOARD_P4_TOUCH_SCL
#define BOARD_P4_TOUCH_SCL    8
#endif
#ifndef BOARD_P4_TOUCH_RST        /* GT911 reset. -1 = none. */
#define BOARD_P4_TOUCH_RST    22
#endif
#ifndef BOARD_P4_TOUCH_INT        /* GT911 INT. -1 = poll. */
#define BOARD_P4_TOUCH_INT    21
#endif
#ifndef BOARD_P4_TOUCH_I2C_ADDR   /* GT911 primary address (probe 0x14 as fallback) */
#define BOARD_P4_TOUCH_I2C_ADDR   0x5D
#endif
#ifndef BOARD_P4_TOUCH_I2C_ADDR_ALT
#define BOARD_P4_TOUCH_I2C_ADDR_ALT 0x14
#endif

/* ==================== microSD / TF card — SDMMC 4-bit ===================== */
/* ⚠️ ASSUMED identical to the 4.3" board (same P4 module). Unverified. */
#ifndef BOARD_P4_SD_CLK
#define BOARD_P4_SD_CLK       43
#endif
#ifndef BOARD_P4_SD_CMD
#define BOARD_P4_SD_CMD       44
#endif
#ifndef BOARD_P4_SD_D0
#define BOARD_P4_SD_D0        39
#endif
#ifndef BOARD_P4_SD_D1
#define BOARD_P4_SD_D1        40
#endif
#ifndef BOARD_P4_SD_D2
#define BOARD_P4_SD_D2        41
#endif
#ifndef BOARD_P4_SD_D3
#define BOARD_P4_SD_D3        42
#endif

/* ==================== Wi-Fi/BT — ESP32-C6 over ESP-Hosted/SDIO ============ */
/* ⚠️ ASSUMED identical to the 4.3" board. Unverified. */
#ifndef BOARD_P4_WIFI_SDIO_CLK
#define BOARD_P4_WIFI_SDIO_CLK 18
#endif
#ifndef BOARD_P4_WIFI_SDIO_CMD
#define BOARD_P4_WIFI_SDIO_CMD 19
#endif
#ifndef BOARD_P4_WIFI_SDIO_D0
#define BOARD_P4_WIFI_SDIO_D0  14
#endif
#ifndef BOARD_P4_WIFI_SDIO_D1
#define BOARD_P4_WIFI_SDIO_D1  15
#endif
#ifndef BOARD_P4_WIFI_SDIO_D2
#define BOARD_P4_WIFI_SDIO_D2  16
#endif
#ifndef BOARD_P4_WIFI_SDIO_D3
#define BOARD_P4_WIFI_SDIO_D3  17
#endif
#ifndef BOARD_P4_C6_RESET
#define BOARD_P4_C6_RESET     54
#endif

/* ==================== Buttons / LED / UART =============================== */
/* ⚠️ ASSUMED identical to the 4.3" board. Unverified. */
#ifndef BOARD_P4_BOOT_BTN
#define BOARD_P4_BOOT_BTN     35
#endif
#ifndef BOARD_P4_UART0_TX
#define BOARD_P4_UART0_TX     37
#endif
#ifndef BOARD_P4_UART0_RX
#define BOARD_P4_UART0_RX     38
#endif
