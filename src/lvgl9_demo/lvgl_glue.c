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
 * LVGL renders UN-rotated at 800x480; board_p4_present_rotated() PPA-rotates
 * each full frame 270deg into the native FB. No sw rotation anywhere.
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
#define LVGL_TASK_STACK     8192
#define LVGL_TASK_PRIO      2
#define LVGL_TASK_CORE      0

static lv_display_t      *s_disp  = NULL;
static lv_indev_t        *s_indev = NULL;
static uint16_t          *s_buf1  = NULL;
static uint16_t          *s_buf2  = NULL;
static esp_timer_handle_t s_tick_timer = NULL;

static SemaphoreHandle_t  s_lvgl_mutex = NULL;
static TaskHandle_t       s_lvgl_task  = NULL;
static bool               s_started    = false;

/* ===================================================================== */
/* LVGL callbacks                                                        */
/* ===================================================================== */

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    /* RENDER_MODE_FULL: one flush per frame covering the whole logical 800x480
     * buffer, un-rotated RGB565 (LV_COLOR_DEPTH=16). The PPA rotates the frame
     * in hardware into the off-screen native FB and swaps it in, so nothing
     * renders into the scanned FB (no DPI-fetch underrun / white flash). */
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;
    board_p4_present_rotated((const uint16_t *)px_map, w, h);
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

    /* 2. TWO FULL-SCREEN logical (800x480) RGB565 draw buffers in PSRAM:
     *    RENDER_MODE_FULL needs full-frame buffers; double-buffering keeps
     *    render and PPA-rotate pipelined. 2 x 800*480*2B = 1.5MB. */
    const size_t buf_bytes = (size_t)LV_HOR * LV_VER * sizeof(uint16_t);
    s_buf1 = (uint16_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
    s_buf2 = (uint16_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
    if (!s_buf1 || !s_buf2) {
        ESP_LOGE(TAG, "draw buffer alloc failed");
        return false;
    }

    /* 3. Display — logical landscape 800x480, full-frame flushes. */
    s_disp = lv_display_create(LV_HOR, LV_VER);
    if (!s_disp) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return false;
    }
    lv_display_set_flush_cb(s_disp, disp_flush_cb);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, buf_bytes,
                           LV_DISPLAY_RENDER_MODE_FULL);

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
    ESP_LOGI(TAG, "lvgl_glue_start OK (LVGL %d.%d, logical %dx%d landscape, PPA HW rotate)",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LV_HOR, LV_VER);
    return true;
}
