/*
 * lvgl_glue.h — LVGL 9 glue for guition-jc4880p4-bsp (P4Display_node).
 *
 * Same public API as the BSP's LVGL 8.3 example glue, ported to the LVGL 9
 * display/indev API. See lvgl_glue.c for the geometry/rotation contract.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bring up LVGL on the BSP display: draw buffers, display + touch drivers,
 * tick source, recursive lock, and (optionally) a pinned service task that
 * drives lv_timer_handler(). Call board_p4_display_init() first. */
bool lvgl_glue_start(bool run_service_task);

/* Serialize any LVGL access from outside the service task. timeout_ms == 0
 * blocks forever. Recursive — nested lock() from the same task is safe. */
bool lvgl_glue_lock(unsigned int timeout_ms);
void lvgl_glue_unlock(void);

/* Manual alternative to the service task: run one lv_timer_handler() pass
 * under the lock; returns the delay LVGL asks for until the next call (ms). */
unsigned int lvgl_glue_handler(void);

#ifdef __cplusplus
}
#endif
