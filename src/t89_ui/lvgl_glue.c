/*
 * lvgl_glue.c — LVGL 9 glue for guition-jc4880p4-bsp (P4Display_node).
 *
 * Port of the BSP's LVGL 8.3 example glue (examples/LvglUi/src/lvgl_glue.c)
 * to the LVGL 9 API: lv_display_t/lv_display_create + lv_display_set_buffers
 * (LV_DISPLAY_RENDER_MODE_FULL) replace lv_disp_drv_t/full_refresh, and
 * lv_indev_create replaces lv_indev_drv_t. Rotation contract is unchanged:
 *
 *   - NATIVE glass (portrait):   480 x 800  (BOARD_P4_LCD_H_RES / _V_RES)
 *   - LOGICAL (what LVGL sees):   800 x 480  (landscape)
 *
 * LVGL renders UN-rotated at 800x480 in RENDER_MODE_PARTIAL; each dirty area
 * is PPA-rotated into the native FB by board_p4_flush_region_rotated() (local
 * BSP patch), and board_p4_present() swaps on the last flush. No sw rotation
 * anywhere. (The lvgl9_demo variant of this file uses the simpler full-frame
 * board_p4_present_rotated() path — correct but redraws everything per frame.)
 */

#include "lvgl_glue.h"
#include "board_p4.h"

#include "lvgl.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "lvgl_glue9";

/* Panel geometry. */
#define PANEL_NATIVE_W      BOARD_P4_LCD_H_RES   /* 480 */
#define PANEL_NATIVE_H      BOARD_P4_LCD_V_RES   /* 800 */
#define LV_HOR              PANEL_NATIVE_H        /* logical landscape width  = 800 */
#define LV_VER              PANEL_NATIVE_W        /* logical landscape height = 480 */
#define LVGL_TICK_PERIOD_MS 2
/* 32 KB, not the 8 KB this was ported with: thorvg's rasterizer (vector
 * graphics, LV_USE_VECTOR_GRAPHIC) puts very large frames on the stack of
 * whichever task calls lv_timer_handler(). At 8 KB the first vector draw ran
 * the stack pointer ~13 KB PAST the end of the stack — a "Stack protection
 * fault" panic in task "lvgl" the instant a vector screen was shown. Watch
 * lvgl_glue_stack_high_water() before trimming this back. */
#define LVGL_TASK_STACK     32768
#define LVGL_TASK_PRIO      2
#define LVGL_TASK_CORE      0

static lv_display_t      *s_disp  = NULL;
static lv_indev_t        *s_indev = NULL;
static uint16_t          *s_buf1  = NULL;
static esp_timer_handle_t s_tick_timer = NULL;

static SemaphoreHandle_t  s_lvgl_mutex = NULL;
static TaskHandle_t       s_lvgl_task  = NULL;
static bool               s_started    = false;

/* ===================================================================== */
/* LVGL callbacks                                                        */
/* ===================================================================== */

/* Stall diagnostics. Both stages of the flush can block forever: the PPA rotate
 * runs in PPA_TRANS_MODE_BLOCKING (semaphore, no timeout) and the DPI present
 * gates internally. s_stall_watch_task runs on the other core and takes no LVGL
 * lock, so it still reports when the LVGL task and loop() are both wedged. */
#define FLUSH_PHASE_IDLE     0
#define FLUSH_PHASE_ROTATE   1
#define FLUSH_PHASE_PRESENT  2
static volatile uint32_t s_flush_seq   = 0;
static volatile int      s_flush_phase = FLUSH_PHASE_IDLE;
static volatile int      s_flush_rect[4];

static void stall_watch_task(void *arg)
{
    (void)arg;
    uint32_t last_seq = 0;
    int quiet_s = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint32_t seq = s_flush_seq;
        if (seq != last_seq) { last_seq = seq; quiet_s = 0; continue; }
        /* No flush completed for a while. Idle UIs legitimately stop flushing,
         * so only shout if we're parked mid-flush. */
        if (++quiet_s >= 3 && s_flush_phase != FLUSH_PHASE_IDLE) {
            ESP_LOGE(TAG, "FLUSH STALLED in %s, seq=%u, last rect %d,%d %dx%d",
                     s_flush_phase == FLUSH_PHASE_ROTATE ? "PPA rotate" : "DPI present",
                     (unsigned)seq, s_flush_rect[0], s_flush_rect[1],
                     s_flush_rect[2], s_flush_rect[3]);
            quiet_s = 0;
        }
    }
}

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    /* RENDER_MODE_PARTIAL: LVGL hands us only the invalidated areas, un-rotated
     * RGB565 in logical 800x480 coords. Each area is PPA-rotated into its mapped
     * spot in the back native FB; on the refresh's last area the completed back
     * FB is presented (buffer swap + dirty-band coherency in the BSP). Far less
     * work than the old RENDER_MODE_FULL path, which redrew and rotated the
     * whole 800x480 frame even for a one-widget change. */
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;

    s_flush_rect[0] = area->x1; s_flush_rect[1] = area->y1;
    s_flush_rect[2] = w;        s_flush_rect[3] = h;

    s_flush_phase = FLUSH_PHASE_ROTATE;
    board_p4_flush_region_rotated(area->x1, area->y1, w, h, (const uint16_t *)px_map);
    if (lv_display_flush_is_last(disp)) {
        s_flush_phase = FLUSH_PHASE_PRESENT;
        board_p4_present();
    }
    s_flush_phase = FLUSH_PHASE_IDLE;
    s_flush_seq++;

    lv_display_flush_ready(disp);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int  px = 0, py = 0;          /* RAW native-panel coords: px:0..480, py:0..800 */
    bool pressed = false;
    board_p4_touch_read(&px, &py, &pressed);

    if (pressed) {
        /* Map raw NATIVE portrait coords into LOGICAL landscape with the same
         * 270 CCW rotation the PPA applies to the image (see BSP README):
         *     lx = py ; ly = (PANEL_NATIVE_W - 1) - px */
        if (px < 0) px = 0; else if (px >= PANEL_NATIVE_W) px = PANEL_NATIVE_W - 1;
        if (py < 0) py = 0; else if (py >= PANEL_NATIVE_H) py = PANEL_NATIVE_H - 1;

        int lx = py;
        int ly = (PANEL_NATIVE_W - 1) - px;

        if (lx < 0) lx = 0; else if (lx >= LV_HOR) lx = LV_HOR - 1;
        if (ly < 0) ly = 0; else if (ly >= LV_VER) ly = LV_VER - 1;
        data->point.x = lx;
        data->point.y = ly;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/* ===================================================================== */
/* LVGL service task: run lv_timer_handler() under the lock.             */
/* ===================================================================== */
static void lvgl_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t delay_ms = 5;
        if (lvgl_glue_lock(0)) {
            delay_ms = lv_timer_handler();
            lvgl_glue_unlock();
        }
        if (delay_ms < 2)   delay_ms = 2;
        if (delay_ms > 100) delay_ms = 100;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/* ===================================================================== */
/* Public API                                                            */
/* ===================================================================== */

bool lvgl_glue_lock(unsigned int timeout_ms)
{
    if (!s_lvgl_mutex) return false;
    const TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY
                                               : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mutex, ticks) == pdTRUE;
}

void lvgl_glue_unlock(void)
{
    if (s_lvgl_mutex) xSemaphoreGiveRecursive(s_lvgl_mutex);
}

unsigned int lvgl_glue_stack_high_water(void)
{
    if (!s_lvgl_task) return 0;
    return (unsigned int)uxTaskGetStackHighWaterMark(s_lvgl_task);
}

unsigned int lvgl_glue_handler(void)
{
    unsigned int delay_ms = 5;
    if (lvgl_glue_lock(0)) {
        delay_ms = lv_timer_handler();
        lvgl_glue_unlock();
    }
    return delay_ms;
}

bool lvgl_glue_start(bool run_service_task)
{
    if (s_started) return true;

    /* Touch (panel is brought up by the caller via board_p4_display_init). */
    if (board_p4_touch_init() != ESP_OK) {
        ESP_LOGW(TAG, "board_p4_touch_init failed — UI shows but touch is dead");
    }

    /* 1. LVGL core. With LV_USE_OS == LV_OS_NONE, lv_draw_sw_init() calls
     * tvg_engine_init(TVG_ENGINE_SW, 0), so thorvg rasterizes synchronously on
     * whichever task calls lv_timer_handler() — this one — and spawns no worker
     * threads of its own. (If LV_USE_OS is ever turned back on, those workers
     * are std::thread/pthread and take ESP-IDF's ~3KB default stack, which
     * thorvg overruns instantly; esp_pthread_set_cfg() before lv_init() is the
     * fix.) */
    lv_init();

    /* 2. ONE partial draw buffer, a 40-row logical stripe (800*40*2B = 62.5KB),
     *    in INTERNAL RAM: LVGL's software renderer is memory-bandwidth-bound,
     *    and internal SRAM is far faster to render into than PSRAM. The PPA
     *    reads it via DMA (internal RAM is DMA-capable). A second buffer buys
     *    nothing here — the flush rotates synchronously, so render and flush
     *    never overlap.
     *
     *    Down from 120 rows, and this size is now load-bearing: it caps the
     *    ARGB8888 scratch lv_draw_sw_vector() allocates per vector draw, since
     *    a render chunk can never exceed the draw buffer —
     *    (buf_bytes / 2 px) * 4 B = buf_bytes * 2, so 125KB here. LV_MEM_SIZE
     *    must stay comfortably above that; resize the two together.
     *
     *    Only byte capacity matters for chunking, not row count: a
     *    needle-sized dirty region still lands in a single chunk, while a
     *    full-screen redraw just takes more passes, and those are rare. */
    const size_t buf_bytes = (size_t)LV_HOR * 40 * sizeof(uint16_t);
    s_buf1 = (uint16_t *)heap_caps_malloc(buf_bytes,
                                          MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (!s_buf1) {
        ESP_LOGW(TAG, "internal draw buffer alloc failed — falling back to PSRAM");
        s_buf1 = (uint16_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
    }
    if (!s_buf1) {
        ESP_LOGE(TAG, "draw buffer alloc failed");
        return false;
    }

    /* 3. Display — logical landscape 800x480, partial (dirty-area) flushes. */
    s_disp = lv_display_create(LV_HOR, LV_VER);
    if (!s_disp) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return false;
    }
    lv_display_set_flush_cb(s_disp, disp_flush_cb);
    lv_display_set_buffers(s_disp, s_buf1, NULL, buf_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    /* Force the refresh period rather than trusting LV_DEF_REFR_PERIOD: the
     * lv_conf is pulled in via LV_CONF_PATH, which PlatformIO's dependency
     * scanner can't see through — a conf edit does NOT rebuild the compiled
     * LVGL objects, so the conf value can silently be stale. */
    lv_timer_set_period(lv_display_get_refr_timer(s_disp), 16);

    /* 4. Touch input device. */
    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);

    /* 5. Tick source: esp_timer periodic. */
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    esp_timer_create(&tick_args, &s_tick_timer);
    esp_timer_start_periodic(s_tick_timer, LVGL_TICK_PERIOD_MS * 1000);

    /* 6. Recursive mutex (+ optional service task). */
    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (!s_lvgl_mutex) {
        ESP_LOGE(TAG, "LVGL mutex create failed");
        return false;
    }
    if (run_service_task) {
        /* Checked: internal RAM is tight once the render threads are up, and a
         * silent failure here leaves nothing driving lv_timer_handler() — a
         * frozen display with no clue why. */
        if (xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK, NULL,
                                    LVGL_TASK_PRIO, &s_lvgl_task, LVGL_TASK_CORE) != pdPASS) {
            s_lvgl_task = NULL;
            ESP_LOGE(TAG, "LVGL service task create FAILED (%d B stack, %u B internal free) "
                          "— nothing will render",
                     LVGL_TASK_STACK,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            return false;
        }
    }

    /* Deliberately on the OTHER core to the LVGL task, and it never takes the
     * LVGL lock — so it survives a wedged render pipeline and can name it. */
    xTaskCreatePinnedToCore(stall_watch_task, "lvgl_stall", 3072, NULL,
                            LVGL_TASK_PRIO + 1, NULL, LVGL_TASK_CORE ? 0 : 1);

    /* 7. Backlight on. */
    board_p4_backlight(true);

    s_started = true;
    ESP_LOGI(TAG, "lvgl_glue_start OK (LVGL %d.%d, logical %dx%d landscape, PPA HW rotate)",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LV_HOR, LV_VER);
    /* Internal RAM is the scarce pool here: draw buffer + the LVGL service task
     * + LV_DRAW_SW_DRAW_UNIT_CNT render threads all come out of it, and pit mode
     * still needs room for WiFi. Report the margin rather than assuming it. */
    /* printf, not ESP_LOGI/W: this build runs with CORE_DEBUG_LEVEL at Error,
     * so anything below ESP_LOGE is compiled out. */
    printf("[LVGL] internal RAM free: %u B (largest block %u B), draw units: %d\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
           (int)LV_DRAW_SW_DRAW_UNIT_CNT);
    return true;
}
