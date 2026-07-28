---
name: flash-p4
description: Build, flash and crash-triage the t89_ui firmware on the Guition ESP32-P4 display node. Use when asked to build, flash, upload, run or test this node on real hardware, and when the node reboots or a screen crashes and the cause has to be found.
---

# Building and flashing P4Display_node

The default env is `t89_ui`. PlatformIO lives at
`%USERPROFILE%\.platformio\penv\Scripts\platformio.exe` — call it by full path
if `pio` is not on PATH.

```
pio run -e t89_ui
```

## After editing lv_conf_v9.h, clean first

`lv_conf_v9.h` reaches LVGL through `-DLV_CONF_PATH=...`, which PlatformIO's
dependency scanner does not track. Editing it recompiles only the changed
`src/` files; the `lvgl` library keeps its old objects, so a changed
`LV_MEM_SIZE` (or any other option) is silently NOT in the binary and the size
report looks unchanged.

```
pio run -e t89_ui -t clean
pio run -e t89_ui
```

A full rebuild is ~30 s. Confirm the change took effect at runtime (see the
`[HB]` heartbeat below), not from the build output.

## Flash

Do not assume a port number — it differs per machine. Find the board first: it
is the serial device with `VID:PID=303A:1001` (Espressif).

```
pio device list --serial
pio run -e t89_ui -t upload --upload-port COM15
```

Only the board's High-Speed USB-C port enumerates.

# When the node reboots or a screen crashes

The give-away is that it "flashes something and jumps back to the dashboard":
that is a reboot, and Screen1 is what loads at boot.

## 1. Capture the serial log while it is reproduced

`scripts/capture-serial.ps1` survives the USB CDC re-enumeration that a reboot
causes, which a plain `pio device monitor` does not. Run it in the background,
then ask the user to reproduce.

```
powershell -NoProfile -ExecutionPolicy Bypass -File .claude/skills/flash-p4/scripts/capture-serial.ps1 -Port COM15 -Seconds 150 -Out crash.log
```

## 2. Read the reset reason

* `task_wdt: ... IDLE0 (CPU 0)` with `Tasks currently running: CPU 0: lvgl`
  means the LVGL task stopped yielding. Nearly always a failed allocation:
  `LV_USE_ASSERT_MALLOC` is 1 and `LV_ASSERT_HANDLER` is `while(1);`, so LVGL
  spins forever instead of returning an error, and the watchdog reboots.
* A load/store fault or `Guru Meditation` is a genuine pointer bug.

`LV_USE_LOG` is 0, so LVGL itself prints nothing — the register dump is all
there is.

## 3. Decode MEPC

```
riscv32-esp-elf-addr2line -pfiaC -e .pio/build/t89_ui/firmware.elf 0x4001fd92
```

The toolchain is at
`%USERPROFILE%\.platformio\packages\toolchain-riscv32-esp\bin\`. Feed it MEPC
plus any `0x4000xxxx`/`0x4001xxxx` values from the stack dump to get a
backtrace.

To confirm an assert spin, disassemble around MEPC — the signature is a
two-byte jump to itself immediately after a failed allocation:

```
riscv32-esp-elf-objdump -d --start-address=0x4001fd60 --stop-address=0x4001fda4 .pio/build/t89_ui/firmware.elf
```

```
jal   lv_malloc_zeroed
bnez  a0, +2          ; allocation OK?
j     .               ; LV_ASSERT_MALLOC
```

## 4. Watch the LVGL pool

The `[HB]` heartbeat prints `lvFree`, `lvBiggest`, `lvUsed%` and `lvFrag%` for
LVGL's own pool (`LV_MEM_SIZE`, not the system heap). Every screen is built at
boot and never destroyed, so the idle figure is the floor.

Costs worth knowing when a full-screen dial is involved:

* An anti-aliased circle mask for a rounded rect or an arc costs
  `(radius + 1) * 16` plus `radius * 6 + 6` bytes in a single allocation —
  ~8.8 KB at radius 400, and `lv_draw_sw_arc` needs two of them.
* Rotating an image is cheap in memory: `lv_draw_sw_img` transforms in strips
  capped at `MAX_BUF_SIZE` (4 panel lines, 6.4 KB). It is expensive in time,
  so quantise the angle before redrawing.
* An `lv_msgbox`, or anything with opacity or a transform on a parent, renders
  through a draw layer, which asks for one `LV_DRAW_LAYER_SIMPLE_BUF_SIZE`
  (48 KB) block. On failure LVGL retries forever — same watchdog reboot.
