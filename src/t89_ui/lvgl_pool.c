/*
 * lvgl_pool.c — where LVGL's heap physically lives, and why it matters here.
 *
 * This pool backs the ARGB8888 scratch buffer that lv_draw_sw_vector()
 * allocates, memzero-clears, rasterizes into and then alpha-blends down to
 * RGB565 for EVERY vector draw — thorvg cannot render straight into a 16-bit
 * framebuffer. That is hundreds of KB of traffic per frame on the rev counter
 * screen.
 *
 * With the pool in PSRAM that traffic pinned both cores at 100% for ~15 FPS,
 * and adding a second render thread bought nothing — the classic signature of
 * being memory-bandwidth-bound rather than compute-bound. It also competes with
 * the MIPI-DSI scan-out for bus time. Hence: internal SRAM first.
 *
 * The PSRAM fallback exists so a tight-RAM build still boots (slowly) instead
 * of dying inside lv_init().
 */
#include "lvgl_pool.h"

#include "esp_heap_caps.h"
#include <stdio.h>

void *t89_lvgl_pool_alloc(size_t size)
{
    void *p = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (p) {
        printf("[LVGL] pool: %u B in internal SRAM (%u B internal left)\n",
               (unsigned)size,
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        return p;
    }

    p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    printf("[LVGL] pool: %u B internal alloc FAILED — %s\n", (unsigned)size,
           p ? "using PSRAM instead, vector screens will be slow"
             : "PSRAM FAILED TOO, lv_init() is about to die");
    return p;
}
