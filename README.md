# P4Display_node

T89 display node for two Guition ESP32-P4 + ESP32-C6 touch displays running the
same dashboard. Successor to the ESP32-S3 / Sunton 8048S043C node in
[esp32_T89Display](https://github.com/TKCAJS/esp32_T89Display).

| board | panel | role |
|---|---|---|
| **JC4880P443C_I_W_Y** | 4.3" 480x800 ST7701S MIPI-DSI, GT911 | test rig |
| **JC1060P470C-I-W-Y** | 7" 1024x600 JD9165 MIPI-DSI, GT911 | car |

Status — **4.3"**: display, touch, LVGL 9.5 @ 60 fps (PPA hardware-rotate
partial render), full dashboard UI, SD_MMC logging and pit-mode WiFi (SoftAP log
viewer via the C6 co-processor) all verified on hardware. CAN runs against a stub
until the transceiver is wired to JP1 (TX=GPIO33, RX=GPIO31).

Status — **7"**: builds, **never run on hardware**. Written from the vendor's
Arduino demo for that model while the board was still in the post. See
[lib/guition-jc1060p470-bsp/VENDORED.md](lib/guition-jc1060p470-bsp/VENDORED.md)
for exactly which values are sourced and which are guesses.

## Build environments (PlatformIO, pioarduino 55.03.36-1)

| env | purpose |
|---|---|
| `t89_ui` (default) | 4.3" dashboard: SquareLine UI + GaugeUpdater + SDLogger + PitServer, stubbed CAN |
| `t89_ui_1060` | the same dashboard on the 7" board, scaled 1.25x |
| `c6_wifi` | C6 co-processor tool: reports/updates ESP-Hosted slave firmware (embedded image), SoftAP test |

Phase-1/2 bring-up envs (`colorbar_test`, `lvgl_test`, `lvgl9_test`) were removed once
`t89_ui` superseded them — see git history if that scaffolding is needed again.

Flash: `pio run -e t89_ui -t upload` (boards enumerate on the **High-Speed**
USB-C port only, as `/dev/ttyACM0`).

## How the two UI trees relate

`src/t89_ui_1060` is a copy of `src/t89_ui` with three deliberate differences —
everything else is byte-identical, so porting a screen change across is a plain
file copy:

1. **`ui_scale.h`**, included by every screen and component, redefines the LVGL
   geometry setters to multiply their arguments by 1.25. The screens keep their
   original 800x480 coordinates. 600/480 is exactly 1.25, so the height fills;
   the width lands at 1000 px inside 1024, leaving a 12 px black margin each
   side that the centre-relative layout absorbs for free.
2. **`fonts/`** holds 1.25x recuts of the same typefaces under the *same symbol
   names* — `ui_font_DSEG20` simply is 25 px there. `lv_font_montserrat_*` is
   remapped by `ui_scale.h` to the next enabled cut (18/24/36 were switched on
   in `lv_conf_v9.h` for this).
3. **`lvgl_glue.c`** has no rotation: that glass is natively landscape, so LVGL's
   coordinates are the framebuffer's and touch needs no remap.

`images/` is **not** duplicated — the 7" env compiles the artwork straight out of
`src/t89_ui/images` and `ui_scale.h` draws it at 1.25x.

The three places where geometry is computed rather than written as a literal
(the Screen6/Screen8 needle point lists and the Screen9 pointer pivot) can't go
through the macro shim and are scaled explicitly, with a comment at each site.

## Key hardware facts

- Chip rev v1.3 boots with the BSP's `esp32p4_es` board variant; CPU 360 MHz.
  `boards/jc1060p4.json` inherits that flag **unverified** — if the 7" board
  ships production silicon and the 2nd-stage bootloader hits "Illegal
  instruction", drop `chip_variant` there first.
- Board definitions live in [boards/](boards), not in the BSP libs (each BSP
  keeps its own copy as the upstream reference, but `boards_dir` points here).
- Platform must be pioarduino **55.03.36-1** (adjacent releases bootloop the P4).
- Display 4.3": manual ST7701S DSI init (vendor `esp_lcd_st7701` leaves it black)
  via the vendored [guition-jc4880p4-bsp](lib/guition-jc4880p4-bsp) — carries
  local patches, see its `VENDORED.md`.
- Display 7": manual JD9165 DSI init via
  [guition-jc1060p470-bsp](lib/guition-jc1060p470-bsp). 2 lanes @ 750 Mbps, DPI
  56 MHz, h 40/160/160, v 10/23/12. LCD reset is GPIO **27** (not 5) and touch
  reset GPIO **22** with a real INT on **21**.
- LVGL 4.3": logical 800x480 landscape, `RENDER_MODE_PARTIAL`, per-dirty-rect PPA
  hardware rotation into the native 480x800 framebuffer.
- LVGL 7": 1024x600, `RENDER_MODE_PARTIAL`, plain memcpy flush — no PPA, no
  rotation, no dirty-area alignment constraint.
- WiFi: C6 over SDIO (ESP-Hosted). Slave firmware must match the Arduino core's
  host stack — run the `c6_wifi` env once to update it (factory boards ship
  older firmware). The hosted transport cannot be re-inited: PitServer therefore
  cycles the radio with `esp_wifi_stop()/start()`, never `WIFI_OFF`.
- SD: SDMMC 4-bit (CLK43/CMD44/D0-3 39-42). TF_VCC (LDO ch4) is owned by the
  display BSP — `SD_MMC.setPowerChannel(-1)`.
- lv_conf gotcha: `LV_CONF_PATH` edits do NOT trigger an LVGL rebuild
  (`rm -rf .pio/build/<env>/lib*/lvgl` to force it).
