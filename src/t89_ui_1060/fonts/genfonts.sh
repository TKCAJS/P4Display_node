#!/bin/sh
# Regenerate the t89_ui fonts at 1.25x for the 1024x600 (7") variant.
# Symbol names are kept IDENTICAL to the 4.3" set so the screen code needs no
# font-name changes: "ui_font_DSEG20" just means "the DSEG20 slot", rendered
# 25px here instead of 20px.
set -e

SCRATCH="$(cd "$(dirname "$0")" && pwd)"
CONV="$SCRATCH/node_modules/.bin/lv_font_conv"
OUT="/c/Users/Adrian/Documents/PlatformIO/Projects/P4Display_node/src/t89_ui_1060/fonts"

DSEG_CLASSIC="C:/Users/Adrian/SquareLine/assets/fonts/DSEG7-Classic/DSEG7Classic-Italic.ttf"
DSEG_MODERN="C:/Users/Adrian/SquareLine/assets/fonts/DSEG7-Modern/DSEG7Modern-BoldItalic.ttf"
EXO2="C:/Users/Adrian/Documents/assets/Exo2-ExtraBoldItalic.ttf"
INKFREE="C:/Users/Adrian/SquareLine/assets/Inkfree.ttf"

mkdir -p "$OUT"

# name                        bpp size ttf
gen() {
  name="$1"; bpp="$2"; size="$3"; ttf="$4"
  printf '  %-28s bpp=%s size=%s\n' "$name" "$bpp" "$size"
  "$CONV" --bpp "$bpp" --size "$size" --font "$ttf" \
          -o "$OUT/$name.c" --format lvgl -r 0x20-0x7f \
          --no-compress --no-prefilter --lv-include lvgl.h
}

#   symbol                     bpp  1.25x size   source ttf        (was)
gen ui_font_DSEG20              8   25            "$DSEG_CLASSIC"   # 20
gen ui_font_DSEG48              8   60            "$DSEG_CLASSIC"   # 48
gen ui_font_DSEG60              8   75            "$DSEG_MODERN"    # 60
gen ui_font_DSEG72              8   90            "$DSEG_MODERN"    # 72
gen ui_font_DSEG86              8   60            "$DSEG_MODERN"    # 48 (name is a misnomer upstream)
gen ui_font_DSEG7classicitalic  1   20            "$DSEG_CLASSIC"   # 16
gen ui_font_EXO2                4   90            "$EXO2"           # 72
gen ui_font_EXO2_128            4   160           "$EXO2"           # 128
gen ui_font_Font1               1   20            "$INKFREE"        # 16
gen ui_font_inkfree40           1   50            "$INKFREE"        # 40
gen ui_font_inkfree402          4   50            "$INKFREE"        # 40

echo "done"
