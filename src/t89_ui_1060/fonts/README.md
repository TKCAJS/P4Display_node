# t89_ui_1060 fonts — the 4.3" set, recut at 1.25x

Every file here is the same typeface, same character range (0x20–0x7f) and same
bpp as its namesake in `../../t89_ui/fonts/`, generated one size class up so the
text matches the 1.25x layout scale that `../ui_scale.h` applies to everything
else.

**The symbol names are deliberately identical to the 4.3" set.** `ui_font_DSEG20`
here simply *is* 25 px. That is what lets the screen files be byte-for-byte
copies of the 4.3" ones — no font references to rewrite when porting a change
across.

| symbol | bpp | 4.3" size | here | source ttf |
|---|---|---|---|---|
| `ui_font_DSEG20` | 8 | 20 | **25** | DSEG7Classic-Italic |
| `ui_font_DSEG48` | 8 | 48 | **60** | DSEG7Classic-Italic |
| `ui_font_DSEG60` | 8 | 60 | **75** | DSEG7Modern-BoldItalic |
| `ui_font_DSEG72` | 8 | 72 | **90** | DSEG7Modern-BoldItalic |
| `ui_font_DSEG86` | 8 | 48 | **60** | DSEG7Modern-BoldItalic |
| `ui_font_DSEG7classicitalic` | 1 | 16 | **20** | DSEG7Classic-Italic |
| `ui_font_EXO2` | 4 | 72 | **90** | Exo2-ExtraBoldItalic |
| `ui_font_EXO2_128` | 4 | 128 | **160** | Exo2-ExtraBoldItalic |
| `ui_font_Font1` | 1 | 16 | **20** | Inkfree |
| `ui_font_inkfree40` | 1 | 40 | **50** | Inkfree |
| `ui_font_inkfree402` | 4 | 40 | **50** | Inkfree |

`ui_font_DSEG86` is 48 px upstream despite the name — that misnomer is carried
over rather than fixed, so the two trees keep matching symbol semantics.

`lv_font_montserrat_*` is not here: it ships with LVGL. `../ui_scale.h` remaps
those names to the next enabled cut, and 18/24/36 were switched on in
`lv_conf_v9.h` to provide them.

## Regenerating

`genfonts.sh` rebuilds the whole set. It needs `lv_font_conv` (node) and the
original TTFs, which on the authoring machine live at:

- `C:/Users/Adrian/SquareLine/assets/fonts/DSEG7-Classic/DSEG7Classic-Italic.ttf`
- `C:/Users/Adrian/SquareLine/assets/fonts/DSEG7-Modern/DSEG7Modern-BoldItalic.ttf`
- `C:/Users/Adrian/Documents/assets/Exo2-ExtraBoldItalic.ttf`
- `C:/Users/Adrian/SquareLine/assets/Inkfree.ttf`

```sh
npm install lv_font_conv        # into whatever dir the script points CONV at
sh genfonts.sh
```

The generated struct layout is byte-identical in shape to what SquareLine emits
(same `lv_font_fmt_txt_dsc_t` / `lv_font_t` initialisers, same
`LVGL_VERSION_MAJOR` guards), which is why these drop straight into the LVGL 9.5
build alongside the SquareLine-generated 4.3" set.
