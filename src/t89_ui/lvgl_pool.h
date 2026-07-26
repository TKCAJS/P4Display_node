/*
 * lvgl_pool.h — backing allocator for LVGL's builtin heap (LV_MEM_POOL_ALLOC).
 *
 * Kept deliberately tiny: lv_conf_v9.h pulls this in, so it is included by
 * effectively every LVGL translation unit.
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate LVGL's pool, preferring internal SRAM and falling back to PSRAM.
 * Called once, from lv_init(). */
void *t89_lvgl_pool_alloc(size_t size);

#ifdef __cplusplus
}
#endif
