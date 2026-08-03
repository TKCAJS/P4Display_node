/*
 * lvgl_glue.c — LVGL 9 glue for guition-jc1060p470-bsp (P4Display_node, 7").
 *
 * Sibling of src/t89_ui/lvgl_glue.c. Same public API, same task/mutex/tick
 * layout — but MUCH simpler, because this panel needs no rotation:
 *
 *   - NATIVE glass (landscape):  1024 x 600  (BOARD_P4_LCD_H_RES / _V_RES)
 *   - LOGICAL (what LVGL sees):  1024 x 600  — identical
 *
 * The 4.3" board's glass is wired portrait, so its glue renders a logical
 * 800x480 frame and PPA-rotates every dirty area into the native 480x800
 * framebuffer. Three things existed purely to serve that rotation and are gone
 * here:
 *
 *   1. invalidate_area_cb() snapping dirty areas to a 32px grid. That was to
 *      keep every PPA destination block on a 64-byte cache line; with a plain
 *      memcpy flush there is no such constraint, and NOT snapping means we
 *      redraw materially less per refresh.
 *   2. board_p4_flush_region_rotated() — the flush is the plain
 *      board_p4_flush_region().
 *   3. The touch coordinate remap. Raw GT911 coords are already in LVGL's
 *      space, so touch_read_cb just clamps and passes them through.
 */

#include "lvgl_glue.h"
#include "board_p4_1060.h"

#include "lvgl.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "lvgl_glue9_1060";

/* Panel geometry — logical == native, no swap. */
#define LV_HOR              BOARD_P4_LCD_H_RES   /* 1024 */
#define LV_VER              BOARD_P4_LCD_V_RES   /* 600  */
#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_STACK     8192
#define LVGL_TASK_PRIO      2
#define LVGL_TASK_CORE      0

/* Draw-buffer height in rows. 1024*100*2B = 200KB, the same internal-RAM
 * budget the 4.3" glue spends on its 800x128 stripe. Unlike that one this
 * number has no alignment constraint — nothing downstream cares where a chunk
 * boundary falls — so it is purely a memory-vs-flush-count trade. */
#define DRAW_BUF_ROWS       100

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

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    /* RENDER_MODE_PARTIAL: LVGL hands us only the invalidated areas, RGB565 in
     * the panel's own coordinate space. Copy each straight into the back
     * framebuffer; on the refresh's last area the completed back FB is
     * presented (buffer swap + dirty-band coherency in the BSP). */
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;
    board_p4_flush_region(area->x1, area->y1, w, h, (const uint16_t *)px_map);
    if (lv_display_flush_is_last(disp)) {
        board_p4_present();
    }
    lv_display_flush_ready(disp);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int  px = 0, py = 0;          /* already landscape 1024x600 — no remap */
    bool pressed = false;
    board_p4_touch_read(&px, &py, &pressed);

    if (pressed) {
        if (px < 0) px = 0; else if (px >= LV_HOR) px = LV_HOR - 1;
        if (py < 0) py = 0; else if (py >= LV_VER) py = LV_VER - 1;
        data->point.x = px;
        data->point.y = py;
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

    /* 1. LVGL core. */
    lv_init();

    /* 2. ONE partial draw buffer, preferably in INTERNAL RAM: LVGL's software
     *    renderer is memory-bandwidth-bound, and internal SRAM is far faster to
     *    render into than PSRAM. A second buffer buys nothing — the flush is a
     *    synchronous memcpy, so render and flush never overlap. */
    const size_t buf_bytes = (size_t)LV_HOR * DRAW_BUF_ROWS * sizeof(uint16_t);
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

    /* 3. Display — 1024x600, partial (dirty-area) flushes. */
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
        xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK, NULL,
                                LVGL_TASK_PRIO, &s_lvgl_task, LVGL_TASK_CORE);
    }

    /* 7. Backlight on. */
    board_p4_backlight(true);

    s_started = true;
    ESP_LOGI(TAG, "lvgl_glue_start OK (LVGL %d.%d, %dx%d landscape, no rotation)",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LV_HOR, LV_VER);
    return true;
}
