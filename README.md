# P4Display_node

T89 display node ported to the **Guition JC4880P443C_I_W_Y** (ESP32-P4 + ESP32-C6,
4.3" 480x800 ST7701S MIPI-DSI, GT911 touch). Successor to the ESP32-S3 / Sunton
8048S043C node in [esp32_T89Display](https://github.com/TKCAJS/esp32_T89Display).

Status: display, touch, LVGL 9.5 @ 60 fps (PPA hardware-rotate partial render),
full dashboard UI, SD_MMC logging and pit-mode WiFi (SoftAP log viewer via the
C6 co-processor) all verified on hardware. CAN runs against a stub until the
transceiver is wired to JP1 (TX=GPIO33, RX=GPIO31).

## Build environments (PlatformIO, pioarduino 55.03.36-1)

| env | purpose |
|---|---|
| `t89_ui` (default) | the real dashboard: SquareLine UI + GaugeUpdater + SDLogger + PitServer, stubbed CAN |
| `c6_wifi` | C6 co-processor tool: reports/updates ESP-Hosted slave firmware (embedded image), SoftAP test |
| `colorbar_test` | phase-1 bring-up proof: raw framebuffer color bars + touch echo |
| `lvgl_test` / `lvgl9_test` | LVGL 8.3 / 9.5 demo UI on the PPA-rotate glue |

Flash: `pio run -e t89_ui -t upload` (board enumerates on the **High-Speed**
USB-C port only, as `/dev/ttyACM0`).

## Key hardware facts

- Chip rev v1.3 boots with the BSP's `esp32p4_es` board variant; CPU 360 MHz.
- Platform must be pioarduino **55.03.36-1** (adjacent releases bootloop the P4).
- Display: manual ST7701S DSI init (vendor `esp_lcd_st7701` leaves it black) via
  the vendored [guition-jc4880p4-bsp](lib/guition-jc4880p4-bsp) — carries local
  patches, see its `VENDORED.md`.
- LVGL: logical 800x480 landscape, `RENDER_MODE_PARTIAL`, per-dirty-rect PPA
  hardware rotation into the native 480x800 framebuffer.
- WiFi: C6 over SDIO (ESP-Hosted). Slave firmware must match the Arduino core's
  host stack — run the `c6_wifi` env once to update it (factory boards ship
  older firmware). The hosted transport cannot be re-inited: PitServer therefore
  cycles the radio with `esp_wifi_stop()/start()`, never `WIFI_OFF`.
- SD: SDMMC 4-bit (CLK43/CMD44/D0-3 39-42). TF_VCC (LDO ch4) is owned by the
  display BSP — `SD_MMC.setPowerChannel(-1)`.
- lv_conf gotcha: `LV_CONF_PATH` edits do NOT trigger an LVGL rebuild
  (`rm -rf .pio/build/<env>/lib*/lvgl` to force it).
