/*
 * SPDX-License-Identifier: MIT
 *
 * board_p4_1060.c — Guition JC1060P470C-I-W-Y (ESP32-P4) BSP: JD9165 1024x600
 * MIPI-DSI panel + GT911 I2C touch.
 *
 * ⚠️ NOT YET RUN ON HARDWARE. Structure is a direct port of the on-hardware
 * -proven guition-jc4880p4-bsp board_p4.c; the panel-specific numbers (lane
 * rate, DPI clock, timings, init table, reset pin) come from the vendor's own
 * Arduino demo for this model. See VENDORED.md.
 *
 * DISPLAY: manual bring-up, NO esp_lcd_jd9165 vendor component:
 *   LDO(chan 3, 2500mV) -> dsi_bus(2 lanes, 750Mbps) -> DBI io ->
 *   DPI panel(1024x600 RGB565, num_fbs=2, 56MHz) -> GPIO27 panel reset ->
 *   PANEL_INIT_CMDS via tx_param -> esp_lcd_panel_init() -> backlight(GPIO23).
 *
 * NO ROTATION: the glass is natively landscape, so LVGL's logical coordinates
 * ARE the framebuffer coordinates. Everything the 4.3" BSP needed the PPA for
 * (present_rotated, flush_region_rotated, the 32px invalidate-alignment dance)
 * is simply absent here. Rendering = write into the back FB, msync the dirty
 * row band, present on the last flush.
 *
 * TOUCH: GT911 over I2C (SDA=7 SCL=8 RST=22 INT=21), probe 0x5D then 0x14.
 *
 * Compiled as C (see board_p4_1060.h) to dodge the esp_lcd_io_i2c.h C++
 * overload, and because the init table uses file-scope compound literals.
 */

#include "board_p4_1060.h"

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_rom_sys.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"            // LEDC PWM for backlight dimming
#include "esp_ldo_regulator.h"

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_touch_gt911.h"

#include "board_p4_1060_jd9165_init.h"   // board_p4_panel_init_cmds[] (include-once)

static const char *TAG = "board_p4_1060";

/* Vendor demo values for this panel (JD9165_PANEL_BUS_DSI_2CH_CONFIG and
 * JD9165_1024_600_PANEL_60HZ_DPI_CONFIG). Refresh works out at
 * 56e6 / (1024+40+160+160) / (600+10+23+12) = ~62.7 Hz. */
#define BOARD_P4_LCD_MIPI_DSI_LANE_NUM   2
#define BOARD_P4_LCD_MIPI_DSI_LANE_MBPS  750
#define BOARD_P4_LCD_DPI_CLK_MHZ         56

// ---- module state ----
static esp_ldo_channel_handle_t  s_ldo_phy    = NULL;
static esp_ldo_channel_handle_t  s_ldo_sd     = NULL;   // TF_VCC (SD power)
static esp_lcd_dsi_bus_handle_t  s_dsi_bus    = NULL;
static esp_lcd_panel_io_handle_t s_dbi_io     = NULL;
static esp_lcd_panel_handle_t    s_panel      = NULL;

// --- Double framebuffer (num_fbs=2) ping-pong state ---
// The DPI bridge continuously scans whichever FB the driver's cur_fb_index
// points at. We render the next frame into the OFF-SCREEN (back) buffer, then
// "present" it with esp_lcd_panel_draw_bitmap(panel, ..., back_fb): the DPI
// driver recognises that the pointer is one of its own FBs and, with NO copy,
// just retargets scan-out to it. So the panel never shows a half-written frame.
//
// s_framebuffer ALWAYS points at the current BACK buffer (the one LVGL writes
// into), so board_p4_get_framebuffer() means "the buffer you draw into".
static uint16_t                 *s_fb[2]       = { NULL, NULL };
static int                       s_back_idx    = 0;     // index LVGL draws into
static uint16_t                 *s_framebuffer = NULL;  // == s_fb[s_back_idx]

// Dirty bounding box accumulated by board_p4_flush_region since the last
// present. After the swap we re-sync ONLY the OTHER (now-back) buffer over the
// SAME rows that just changed, so the two buffers stay coherent for
// partial-mode LVGL WITHOUT a full-frame copy each refresh. Tracked as a row
// band [y0,y1) at full stride, which keeps the coherency copy contiguous.
static int                       s_dirty_y0    = 0;     // inclusive top row
static int                       s_dirty_y1    = 0;     // exclusive bottom row (0 => empty)
// Previous refresh's dirty band — the new back buffer also differs from the
// just-presented one over the PREVIOUS refresh's rows, so present() copies the
// union of both bands to restore coherency. (Both 0 => nothing to carry.)
static int                       s_prev_dirty_y0 = 0;
static int                       s_prev_dirty_y1 = 0;

static i2c_master_bus_handle_t   s_i2c_bus    = NULL;
static esp_lcd_panel_io_handle_t s_touch_io   = NULL;
static esp_lcd_touch_handle_t    s_touch      = NULL;

// ===========================================================================
// Display
// ===========================================================================

static void board_p4_panel_reset(void)
{
#if BOARD_P4_LCD_RST >= 0
    // JD9165 reset shape from the vendor driver's panel_jd9165_reset():
    // high 5ms, low 10ms, high 120ms (active-low reset).
    gpio_config_t rst = {
        .pin_bit_mask = 1ULL << BOARD_P4_LCD_RST,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&rst);
    gpio_set_level(BOARD_P4_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(BOARD_P4_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BOARD_P4_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_LOGI(TAG, "panel reset pulsed on GPIO%d (high 5ms, low 10ms, high 120ms)",
             BOARD_P4_LCD_RST);
#endif
}

static esp_err_t board_p4_send_init_seq(void)
{
    for (size_t i = 0; i < BOARD_P4_PANEL_INIT_CMDS_SIZE; i++) {
        const board_p4_panel_init_cmd_t *e = &board_p4_panel_init_cmds[i];
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(s_dbi_io, e->cmd, e->data, e->len),
                            TAG, "tx_param cmd 0x%02X failed", e->cmd);
        if (e->delay_ms) {
            vTaskDelay(pdMS_TO_TICKS(e->delay_ms));
        }
    }
    return ESP_OK;
}

esp_err_t board_p4_display_init(void)
{
    ESP_LOGI(TAG, "=== display init: JD9165 1024x600 MIPI-DSI (manual recipe) ===");

    // --- 1. DSI PHY power via internal LDO (chan 3, 2500mV) ---
    ESP_LOGI(TAG, "acquire DSI-PHY LDO chan=%d %dmV", BOARD_P4_DSI_LDO_CHAN, BOARD_P4_DSI_LDO_MV);
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = BOARD_P4_DSI_LDO_CHAN,
        .voltage_mv = BOARD_P4_DSI_LDO_MV,
    };
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &s_ldo_phy), TAG, "LDO acquire failed");

    // --- 1b. TF-card (SD) power: TF_VCC on the on-chip LDO. Non-fatal — the
    //     display works regardless. ASSUMED to be wired like the 4.3" board. ---
    {
        esp_ldo_channel_config_t sd_ldo_cfg = {
            .chan_id    = BOARD_P4_SD_LDO_CHAN,
            .voltage_mv = BOARD_P4_SD_LDO_MV,
        };
        esp_err_t e = esp_ldo_acquire_channel(&sd_ldo_cfg, &s_ldo_sd);
        ESP_LOGI(TAG, "acquire TF_VCC LDO chan=%d %dmV -> %s",
                 BOARD_P4_SD_LDO_CHAN, BOARD_P4_SD_LDO_MV,
                 e == ESP_OK ? "ok" : esp_err_to_name(e));
    }

    // --- 2. DSI bus: 2 lanes @ 750 Mbps ---
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = BOARD_P4_LCD_MIPI_DSI_LANE_NUM,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BOARD_P4_LCD_MIPI_DSI_LANE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &s_dsi_bus), TAG, "new_dsi_bus failed");

    // --- 3. DBI control IO (in-band command channel over DSI) ---
    esp_lcd_dbi_io_config_t dbi_cfg = {
        .virtual_channel = 0,
        .lcd_cmd_bits    = 8,
        .lcd_param_bits  = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(s_dsi_bus, &dbi_cfg, &s_dbi_io), TAG,
                        "new_panel_io_dbi failed");

    // --- 4. DPI video config: 1024x600, 56MHz, RGB565, TWO framebuffers ---
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = BOARD_P4_LCD_DPI_CLK_MHZ,
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .in_color_format    = LCD_COLOR_FMT_RGB565,
        .out_color_format   = LCD_COLOR_FMT_RGB565,
        .num_fbs            = 2,   // double-buffer: scan a complete frame only
        .video_timing = {
            .h_size            = BOARD_P4_LCD_H_RES,   // 1024
            .v_size            = BOARD_P4_LCD_V_RES,   // 600
            .hsync_pulse_width = 40,
            .hsync_back_porch  = 160,
            .hsync_front_porch = 160,
            .vsync_pulse_width = 10,
            .vsync_back_porch  = 23,
            .vsync_front_porch = 12,
        },
        .flags.use_dma2d = 1,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_dpi(s_dsi_bus, &dpi_cfg, &s_panel), TAG,
                        "new_panel_dpi failed");

    // --- 5. Panel reset AFTER dpi, BEFORE init sequence ---
    board_p4_panel_reset();

    // --- 6. JD9165 init sequence (manual DCS writes) + panel init ---
    ESP_RETURN_ON_ERROR(board_p4_send_init_seq(), TAG, "init sequence failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel_init failed");
    ESP_LOGI(TAG, "  panel up");

    // --- 7. Grab BOTH streaming framebuffers (num_fbs=2) for ping-pong ---
    void *fb0 = NULL, *fb1 = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(s_panel, 2, &fb0, &fb1), TAG,
                        "get_frame_buffer failed");
    s_fb[0] = (uint16_t *)fb0;
    s_fb[1] = (uint16_t *)fb1;

    // Clear BOTH to black so neither buffer ever scans out garbage. After
    // new_panel_dpi the driver scans fb[0], so fb[0] is the initial FRONT
    // (visible) buffer and fb[1] is the BACK (LVGL draws here).
    const size_t fb_bytes = (size_t)BOARD_P4_LCD_H_RES * BOARD_P4_LCD_V_RES * sizeof(uint16_t);
    for (int i = 0; i < 2; i++) {
        memset(s_fb[i], 0, fb_bytes);
        esp_cache_msync(s_fb[i], fb_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    }
    s_back_idx    = 1;
    s_framebuffer = s_fb[s_back_idx];
    ESP_LOGI(TAG, "  framebuffers fb0=%p fb1=%p (%dx%d RGB565, double-buffered, %u KB each)",
             s_fb[0], s_fb[1], BOARD_P4_LCD_H_RES, BOARD_P4_LCD_V_RES,
             (unsigned)(fb_bytes / 1024));

    // --- 8. Backlight on ---
    board_p4_backlight(true);

    ESP_LOGI(TAG, "=== display init done ===");
    return ESP_OK;
}

esp_lcd_panel_handle_t board_p4_get_panel_handle(void) { return s_panel; }

uint16_t *board_p4_get_framebuffer(void) { return s_framebuffer; }

const uint16_t *board_p4_front_fb(int *w, int *h)
{
    if (w) *w = BOARD_P4_LCD_H_RES;
    if (h) *h = BOARD_P4_LCD_V_RES;
    if (!s_fb[0] || !s_fb[1]) return NULL;

    // present() hands the back buffer to the DPI and then flips s_back_idx to
    // the OTHER buffer, so: back = s_fb[s_back_idx], front = s_fb[1 - back].
    const uint16_t *front = s_fb[1 - s_back_idx];

    // Invalidate (M2C) so the CPU read sees what actually landed in PSRAM, not
    // a stale cache line. Whole-frame, cache-aligned addr/size.
    esp_cache_msync((void *)front,
                    (size_t)BOARD_P4_LCD_H_RES * BOARD_P4_LCD_V_RES * sizeof(uint16_t),
                    ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    return front;
}

void board_p4_flush_region(int x, int y, int w, int h, const uint16_t *src)
{
    // Writes into the BACK framebuffer (s_framebuffer == s_fb[s_back_idx]). The
    // back buffer is NOT being scanned, so partial half-written stripes here are
    // invisible. The completed frame becomes visible only on board_p4_present().
    if (!s_framebuffer || !src) return;

    // Clip to the panel (defensive; LVGL should already keep us in bounds).
    // Note src is advanced using the ORIGINAL row stride, so the left/top clip
    // must be applied before w is narrowed.
    if (x < 0) { w += x; src -= x; x = 0; }
    if (y < 0) { h += y; src -= (size_t)y * (w > 0 ? w : 0); y = 0; }
    if (x + w > BOARD_P4_LCD_H_RES) w = BOARD_P4_LCD_H_RES - x;
    if (y + h > BOARD_P4_LCD_V_RES) h = BOARD_P4_LCD_V_RES - y;
    if (w <= 0 || h <= 0) return;

    const int stride = BOARD_P4_LCD_H_RES;  // FB row stride in pixels
    for (int row = 0; row < h; row++) {
        uint16_t *dst = s_framebuffer + (size_t)(y + row) * stride + x;
        memcpy(dst, src + (size_t)row * w, (size_t)w * sizeof(uint16_t));
    }

    // Flush the touched span back to PSRAM so DPI scan-out sees it. We sync the
    // contiguous byte range covering the whole row band [y, y+h) at full stride
    // (super-set of the rect); UNALIGNED lets us pass a non-cache-line addr/size.
    uint16_t *region = s_framebuffer + (size_t)y * stride;
    size_t    bytes  = (size_t)h * stride * sizeof(uint16_t);
    esp_cache_msync(region, bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

    // Grow the per-refresh dirty row band so present() only has to make the
    // OTHER buffer coherent over these rows (not the whole 600-row frame).
    if (s_dirty_y1 == 0) {              // empty -> first stripe of this refresh
        s_dirty_y0 = y;
        s_dirty_y1 = y + h;
    } else {
        if (y < s_dirty_y0)       s_dirty_y0 = y;
        if (y + h > s_dirty_y1)   s_dirty_y1 = y + h;
    }
}

void board_p4_present(void)
{
    if (!s_panel || !s_fb[0] || !s_fb[1]) return;

    // 1. PRESENT: hand the freshly-completed back buffer to the DPI driver.
    //    Because this pointer is one of the driver's own framebuffers, the
    //    driver does NOT copy — it just retargets scan-out to this buffer. The
    //    bridge starts scanning the complete frame on its next line.
    //    We pass ONLY the dirty row band as the rect: the driver still flips
    //    cur_fb_index, but its internal cache write-back then covers just those
    //    rows instead of re-syncing the whole 1.2MB buffer (flush_region already
    //    synced exactly these rows).
    uint16_t *just_drawn = s_fb[s_back_idx];
    int present_y0 = s_dirty_y0;
    int present_y1 = (s_dirty_y1 > 0) ? s_dirty_y1 : 1;  // non-empty rect for the swap
    esp_lcd_panel_draw_bitmap(s_panel, 0, present_y0,
                              BOARD_P4_LCD_H_RES, present_y1, just_drawn);

    // 2. FLIP: the other buffer becomes the new back buffer for the next frame.
    s_back_idx    ^= 1;
    s_framebuffer  = s_fb[s_back_idx];

    // 3. COHERENCY (minimal traffic): LVGL renders PARTIAL — only invalidated
    //    rows are redrawn each refresh. After the swap the new back buffer holds
    //    the frame from TWO presents ago; the two buffers differ ONLY in the rows
    //    drawn in the last two refreshes. So to make the new back buffer match
    //    the just-presented frame we only need to re-copy the UNION of THIS
    //    refresh's dirty rows and the PREVIOUS one's — not the whole frame.
    int cy0 = s_dirty_y0, cy1 = s_dirty_y1;          // this refresh's band
    if (s_prev_dirty_y1 != 0) {                       // union previous band
        if (s_prev_dirty_y0 < cy0) cy0 = s_prev_dirty_y0;
        if (s_prev_dirty_y1 > cy1) cy1 = s_prev_dirty_y1;
    }
    if (cy0 < 0) cy0 = 0;
    if (cy1 > BOARD_P4_LCD_V_RES) cy1 = BOARD_P4_LCD_V_RES;

    if (cy1 > cy0) {
        const int    stride    = BOARD_P4_LCD_H_RES;
        const size_t row_bytes = (size_t)stride * sizeof(uint16_t);
        const size_t off_px    = (size_t)cy0 * stride;
        const size_t bytes     = (size_t)(cy1 - cy0) * row_bytes;
        memcpy(s_framebuffer + off_px, just_drawn + off_px, bytes);
        esp_cache_msync(s_framebuffer + off_px, bytes,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }

    // Remember this refresh's band for the next present's union, then reset the
    // accumulator for the next refresh.
    s_prev_dirty_y0 = s_dirty_y0;
    s_prev_dirty_y1 = s_dirty_y1;
    s_dirty_y0 = 0;
    s_dirty_y1 = 0;
}

esp_err_t board_p4_draw_bitmap(int x1, int y1, int x2, int y2, const void *data)
{
    if (!s_panel) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2, y2, data);
}

// --- Backlight: LEDC PWM dimmer on BOARD_P4_LCD_BL (active-high) ---
// The vendor demo drives this pin as a plain GPIO; we run an LEDC PWM channel
// on it instead so brightness % maps to duty, matching the 4.3" BSP's API.
// Timer 1 / channel 1, 5 kHz, 8-bit resolution (0..255 duty).
#if BOARD_P4_LCD_BL >= 0
#define BOARD_P4_BL_LEDC_TIMER   LEDC_TIMER_1
#define BOARD_P4_BL_LEDC_CHAN    LEDC_CHANNEL_1
#define BOARD_P4_BL_LEDC_FREQ_HZ 5000
#define BOARD_P4_BL_LEDC_RES     LEDC_TIMER_8_BIT   // duty 0..255
static bool s_bl_ledc_ready = false;

static void board_p4_bl_ledc_init(void)
{
    if (s_bl_ledc_ready) return;
    ledc_timer_config_t tcfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BOARD_P4_BL_LEDC_RES,
        .timer_num       = BOARD_P4_BL_LEDC_TIMER,
        .freq_hz         = BOARD_P4_BL_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&tcfg);
    ledc_channel_config_t ccfg = {
        .gpio_num   = BOARD_P4_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BOARD_P4_BL_LEDC_CHAN,
        .timer_sel  = BOARD_P4_BL_LEDC_TIMER,
        .duty       = 255,           // full brightness until told otherwise
        .hpoint     = 0,
    };
    ledc_channel_config(&ccfg);
    s_bl_ledc_ready = true;
    ESP_LOGI(TAG, "backlight LEDC PWM on GPIO%d (timer%d ch%d %dHz 8-bit)",
             BOARD_P4_LCD_BL, BOARD_P4_BL_LEDC_TIMER, BOARD_P4_BL_LEDC_CHAN,
             BOARD_P4_BL_LEDC_FREQ_HZ);
}
#endif

void board_p4_set_brightness(uint8_t duty)
{
#if BOARD_P4_LCD_BL >= 0
    board_p4_bl_ledc_init();
    uint8_t d = duty;
    if (d != 0 && d < 26) d = 26;   // ~10% floor: never let the BL go black
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BOARD_P4_BL_LEDC_CHAN, d);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BOARD_P4_BL_LEDC_CHAN);
#else
    ESP_LOGW(TAG, "backlight pin disabled (BOARD_P4_LCD_BL < 0)");
    (void)duty;
#endif
}

void board_p4_backlight(bool on)
{
#if BOARD_P4_LCD_BL >= 0
    board_p4_bl_ledc_init();
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BOARD_P4_BL_LEDC_CHAN, on ? 255 : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BOARD_P4_BL_LEDC_CHAN);
    ESP_LOGI(TAG, "backlight %s", on ? "ON" : "OFF");
#else
    ESP_LOGW(TAG, "backlight pin disabled (BOARD_P4_LCD_BL < 0)");
    (void)on;
#endif
}

// ===========================================================================
// Touch (GT911)
// ===========================================================================

esp_err_t board_p4_touch_init(void)
{
    ESP_LOGI(TAG, "=== touch init: GT911 (SDA=%d SCL=%d RST=%d INT=%d) ===",
             BOARD_P4_TOUCH_SDA, BOARD_P4_TOUCH_SCL, BOARD_P4_TOUCH_RST, BOARD_P4_TOUCH_INT);

    // --- 1. New I2C master bus ---
    i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_NUM_0,
        .sda_io_num                   = BOARD_P4_TOUCH_SDA,
        .scl_io_num                   = BOARD_P4_TOUCH_SCL,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_i2c_bus), TAG, "i2c_new_master_bus failed");

    // --- 2. Release GT911 from reset so it shows up on the bus ---
#if BOARD_P4_TOUCH_RST >= 0
    {
        gpio_config_t rst = { .pin_bit_mask = 1ULL << BOARD_P4_TOUCH_RST, .mode = GPIO_MODE_OUTPUT };
        gpio_config(&rst);
        gpio_set_level(BOARD_P4_TOUCH_RST, 0);
        esp_rom_delay_us(10 * 1000);
        gpio_set_level(BOARD_P4_TOUCH_RST, 1);
        esp_rom_delay_us(50 * 1000);   // GT911 boot
        ESP_LOGI(TAG, "GT911 reset pulsed on GPIO%d", BOARD_P4_TOUCH_RST);
    }
#endif

    // --- 3. Probe address: 0x5D then 0x14 ---
    uint8_t addr = 0;
    if (i2c_master_probe(s_i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 100) == ESP_OK) {
        addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;        // 0x5D
    } else if (i2c_master_probe(s_i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, 100) == ESP_OK) {
        addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP; // 0x14
    } else {
        ESP_LOGE(TAG, "no GT911 ACK at 0x5D or 0x14 — check SDA/SCL/RST/INT");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "GT911 found at 0x%02X", addr);

    // --- 4. Touch panel IO over I2C ---
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.dev_addr      = addr;
    io_cfg.scl_speed_hz  = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &s_touch_io), TAG,
                        "new_panel_io_i2c failed");

    // --- 5. GT911 driver ---
    // NOTE: the vendor demo passes x_max=600 / y_max=1024 here, which is its own
    // width/height swapped. For a landscape 1024x600 panel with swap_xy=0 that
    // is wrong (it would clamp x at 600 and never reach the right-hand ~40% of
    // the screen), so we use the panel's real dims. If touch comes back
    // transposed on hardware, the fix is swap_xy/mirror below, not these maxima.
    esp_lcd_touch_config_t tp_cfg = {
        .x_max        = BOARD_P4_LCD_H_RES,   // 1024
        .y_max        = BOARD_P4_LCD_V_RES,   // 600
        .rst_gpio_num = BOARD_P4_TOUCH_RST,
        .int_gpio_num = BOARD_P4_TOUCH_INT,
        .levels   = { .reset = 0, .interrupt = 0 },
        .flags    = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(s_touch_io, &tp_cfg, &s_touch), TAG,
                        "gt911 new failed");

    ESP_LOGI(TAG, "=== touch init done ===");
    return ESP_OK;
}

esp_lcd_touch_handle_t board_p4_get_touch_handle(void) { return s_touch; }

bool board_p4_touch_read(int *x, int *y, bool *pressed)
{
    if (pressed) *pressed = false;
    if (!s_touch) return false;
    esp_lcd_touch_read_data(s_touch);

    uint16_t tx[1] = {0}, ty[1] = {0}, str[1] = {0};
    uint8_t  cnt = 0;
    bool down = esp_lcd_touch_get_coordinates(s_touch, tx, ty, str, &cnt, 1);
    if (down && cnt > 0) {
        if (x) *x = tx[0];
        if (y) *y = ty[0];
        if (pressed) *pressed = true;
        return true;
    }
    return false;
}
