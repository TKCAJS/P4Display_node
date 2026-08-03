# Sources for guition-jc1060p470-bsp

⚠️ **Nothing in this library has been run on hardware yet.** It was written for a
Guition JC1060P470C-I-W-Y that had been ordered but not yet delivered. Treat
every value as "sourced but unverified".

## Where the panel numbers come from

The vendor's own Arduino demo for this exact model, mirrored at
<https://github.com/cheops/JC1060P470C_I_W> (path
`1-Demo/Demo_Arduino/1_1_Lvgl_V8/esp32p4_arduino_mipi-dsi_lvgl/`):

| Value | Source file | Notes |
|---|---|---|
| 1024x600, LCD_RST=27, LCD_LED=23, TP SDA=7 SCL=8 RST=22 INT=21 | `pins_config.h` | |
| DSI 2 lanes @ 750 Mbps | `src/lcd/esp_lcd_jd9165.h` — `JD9165_PANEL_BUS_DSI_2CH_CONFIG` | |
| DPI 56 MHz, h 40/160/160, v 10/23/12 | `src/lcd/esp_lcd_jd9165.h` — `JD9165_1024_600_PANEL_60HZ_DPI_CONFIG` | ~62.7 Hz refresh |
| DSI-PHY LDO chan 3 @ 2500 mV | `src/lcd/jd9165_lcd.cpp` | |
| JD9165 init command table | `src/lcd/esp_lcd_jd9165.c` — `vendor_specific_init_default[]` | transcribed verbatim |
| JD9165 reset shape (hi 5ms / lo 10ms / hi 120ms) | `src/lcd/esp_lcd_jd9165.c` — `panel_jd9165_reset()` | |

The overall BSP structure (double-buffered present, dirty-band cache sync,
LEDC-PWM backlight, GT911 probe-both-addresses) is a port of the on-hardware
-proven `guition-jc4880p4-bsp` in this same repo.

## Deliberate departures from the vendor demo

1. **No `esp_lcd_jd9165` vendor component.** The init table is walked manually
   with `esp_lcd_panel_io_tx_param()`, matching how the 4.3" BSP handles the
   ST7701S. MADCTL=0x00 is prepended to the table since there is no vendor
   driver to send it.
2. **The demo's `rx_param(0x04)` panel-ID read is dropped** — diagnostic only,
   and 0x04 is also a page-1 register in the init table.
3. **GT911 `x_max`/`y_max` are 1024/600, not the demo's 600/1024.** The demo has
   them swapped relative to its own panel orientation; with `swap_xy = 0` that
   would clamp x at 600.
4. **Backlight is LEDC PWM, not a plain GPIO**, so `board_p4_set_brightness()`
   works the same as on the 4.3" board.
5. **`num_fbs = 2`, not the demo's 1** — needed for the tear-free present path.

## Unverified guesses

`board_p4_1060_pins.h` carries the SD, C6-SDIO, audio and UART pin groups over
unchanged from the 4.3" JC4880P443, on the assumption that the shared
JC-ESP32P4-M3 module wires them identically. The vendor demo says nothing about
them. Re-check against the schematic before trusting SD logging or pit mode on
this board.

`boards/jc1060p4.json` also inherits `chip_variant: esp32p4_es` from the 4.3"
board definition. That flag was needed for the rev-v1.3 ES silicon in the other
unit; if this board ships production silicon and fails to boot with an "Illegal
instruction" from the 2nd-stage bootloader, that field is the first thing to
try removing.
