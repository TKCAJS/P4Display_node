# Vendored from https://github.com/ultramcu/guition-jc4880p4-bsp
# commit 324970bade0d1f4e52880fe8016580368bc1e06e (MIT), cloned 2026-07-23.
# CONTAINS LOCAL PATCHES — do not blindly update from upstream:
#   * board_p4_flush_region_rotated() in src/board_p4.{c,h}: per-region PPA
#     rotate for LVGL RENDER_MODE_PARTIAL (see P4Display_node lvgl_glue.c).
