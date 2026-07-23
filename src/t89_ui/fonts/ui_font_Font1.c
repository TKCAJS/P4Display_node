/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font C:/Users/Adrian/SquareLine/assets/Inkfree.ttf -o C:/Users/Adrian/SquareLine/assets\ui_font_Font1.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_FONT1
#define UI_FONT_FONT1 1
#endif

#if UI_FONT_FONT1

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xaa, 0xaa, 0x4,

    /* U+0022 "\"" */
    0xb6, 0xd0,

    /* U+0023 "#" */
    0xa, 0x2, 0x80, 0xa1, 0xff, 0x12, 0x4, 0x81,
    0x3b, 0xf8, 0x22, 0x9, 0x2, 0x40, 0x80,

    /* U+0024 "$" */
    0x10, 0x10, 0x16, 0x18, 0x70, 0x50, 0x90, 0xd0,
    0x38, 0x17, 0x11, 0x16, 0x78, 0x10, 0x10,

    /* U+0025 "%" */
    0x23, 0x81, 0x72, 0x4, 0x88, 0x24, 0x20, 0xa1,
    0x3, 0x4, 0xe0, 0x12, 0x40, 0x91, 0x2, 0x44,
    0x1, 0x20, 0x3, 0x80,

    /* U+0026 "&" */
    0x6, 0x1, 0x40, 0x50, 0xc, 0x1, 0x0, 0xe4,
    0x62, 0x94, 0x22, 0x8b, 0x8e, 0x0,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x12, 0x24, 0x48, 0x88, 0x88, 0x88, 0x84, 0x30,

    /* U+0029 ")" */
    0x20, 0x84, 0x21, 0x4, 0x21, 0x8, 0x42, 0x11,
    0x19, 0x0,

    /* U+002A "*" */
    0xa3, 0xd9, 0x72, 0x10,

    /* U+002B "+" */
    0x10, 0x20, 0x40, 0xbf, 0x2, 0x4, 0x8,

    /* U+002C "," */
    0x5a,

    /* U+002D "-" */
    0xf,

    /* U+002E "." */
    0x90,

    /* U+002F "/" */
    0x0, 0x84, 0x21, 0x10, 0x84, 0x42, 0x11, 0x8,
    0x42, 0x0,

    /* U+0030 "0" */
    0x7c, 0x46, 0x43, 0x41, 0x81, 0x81, 0x81, 0x82,
    0x86, 0xcc, 0x78,

    /* U+0031 "1" */
    0x0, 0x63, 0x92, 0x8, 0x20, 0x82, 0x8, 0x30,
    0xc1,

    /* U+0032 "2" */
    0x3c, 0x18, 0x88, 0x20, 0x8, 0x4, 0x1, 0x0,
    0x80, 0x40, 0x30, 0x8, 0x4, 0x7d, 0xe0,

    /* U+0033 "3" */
    0x3c, 0x21, 0x20, 0x80, 0x80, 0x80, 0xf9, 0x86,
    0x1, 0x0, 0x80, 0x80, 0xc3, 0x80, 0x0,

    /* U+0034 "4" */
    0x1, 0x0, 0x60, 0x14, 0x2, 0x80, 0x90, 0x22,
    0x8, 0x4a, 0x7e, 0xf1, 0x0, 0x20, 0x4, 0x0,
    0x80,

    /* U+0035 "5" */
    0x0, 0x3e, 0x40, 0x40, 0x40, 0x5f, 0xe1, 0x1,
    0x1, 0x2, 0x4, 0x18, 0x0,

    /* U+0036 "6" */
    0x8, 0x10, 0x20, 0x40, 0x80, 0x87, 0x89, 0x91,
    0x92, 0xd4, 0x78, 0x10, 0x0,

    /* U+0037 "7" */
    0x0, 0xfe, 0x2, 0x4, 0x4, 0x7, 0x18, 0x8,
    0x8, 0x8, 0x8, 0x8,

    /* U+0038 "8" */
    0x3c, 0x10, 0x84, 0x22, 0x10, 0x87, 0x26, 0x36,
    0x5, 0x1, 0x40, 0x50, 0x26, 0x10, 0xf8,

    /* U+0039 "9" */
    0xe, 0x8, 0x88, 0x48, 0x2c, 0x34, 0x2a, 0x15,
    0x12, 0x71, 0x0, 0x80, 0x40, 0x20, 0x0,

    /* U+003A ":" */
    0xa0, 0x9,

    /* U+003B ";" */
    0x48, 0x0, 0x12, 0x40,

    /* U+003C "<" */
    0x6, 0x30, 0x86, 0xc, 0x7, 0x1, 0x0,

    /* U+003D "=" */
    0xfc, 0x0, 0x7, 0xe0,

    /* U+003E ">" */
    0x81, 0x81, 0x3, 0x1b, 0x80,

    /* U+003F "?" */
    0x1e, 0xc7, 0x8, 0x20, 0x82, 0x4, 0x8, 0x0,
    0x10,

    /* U+0040 "@" */
    0xf8, 0x0, 0x1c, 0x0, 0xc, 0x0, 0x8, 0xc,
    0x10, 0x50, 0x22, 0x40, 0xd2, 0x1, 0x8c, 0x6,
    0x50, 0x1a, 0x40, 0xf0, 0x86, 0x1, 0xf0,

    /* U+0041 "A" */
    0x0, 0x1, 0x80, 0xa0, 0x24, 0x11, 0x4, 0x41,
    0xfb, 0xc2, 0x20, 0x88, 0x22, 0x9, 0x1, 0x40,
    0x0,

    /* U+0042 "B" */
    0xf, 0x8e, 0x12, 0x42, 0x8, 0x81, 0x3e, 0x2c,
    0x46, 0x8, 0x82, 0x10, 0x82, 0x20, 0x58, 0x0,
    0x0,

    /* U+0043 "C" */
    0x18, 0x28, 0x40, 0x40, 0x80, 0x80, 0x80, 0x80,
    0x81, 0xc6, 0x38,

    /* U+0044 "D" */
    0xff, 0x0, 0x18, 0x40, 0x88, 0x11, 0x2, 0x20,
    0x84, 0x20, 0x88, 0x12, 0x3, 0x80, 0x40, 0x0,

    /* U+0045 "E" */
    0x1, 0xbf, 0x8, 0x4, 0x2, 0x33, 0xe0, 0x80,
    0x40, 0x20, 0x90, 0x47, 0xc0,

    /* U+0046 "F" */
    0x17, 0xfc, 0x4, 0x2, 0x1, 0x18, 0xf0, 0xc0,
    0x20, 0x10, 0x8, 0x4, 0x2, 0x0,

    /* U+0047 "G" */
    0x7, 0x3, 0x1, 0x0, 0x80, 0x20, 0x10, 0x34,
    0xfe, 0x3, 0x81, 0x60, 0x58, 0x26, 0x11, 0xc4,
    0x5e, 0x10,

    /* U+0048 "H" */
    0x0, 0x0, 0x8, 0x1, 0x8, 0x21, 0x4, 0x20,
    0x84, 0x20, 0x84, 0x10, 0x82, 0x1f, 0xfe, 0x8,
    0x41, 0x8, 0x21, 0x4, 0x20, 0x80,

    /* U+0049 "I" */
    0xff, 0x82, 0x0, 0x80, 0x20, 0x8, 0x2, 0x0,
    0x80, 0x20, 0x8, 0x2, 0xf, 0xf8,

    /* U+004A "J" */
    0x2, 0xf, 0xf0, 0x20, 0x8, 0x1, 0x0, 0x40,
    0x10, 0x4, 0x41, 0x20, 0x46, 0x10, 0x78,

    /* U+004B "K" */
    0x3, 0xa, 0x14, 0x49, 0x14, 0x30, 0x40, 0xe1,
    0x3a, 0xc, 0x0,

    /* U+004C "L" */
    0xc0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x7f,

    /* U+004D "M" */
    0x0, 0xd0, 0x1a, 0x65, 0x54, 0xaa, 0x95, 0x54,
    0xaa, 0x96, 0x52, 0xc4, 0x48, 0x5, 0x0, 0x80,

    /* U+004E "N" */
    0x3, 0x7, 0xe, 0x1a, 0x32, 0x62, 0xc5, 0x87,
    0x4,

    /* U+004F "O" */
    0x0, 0x7, 0xc2, 0x9, 0x1, 0x40, 0x60, 0x18,
    0x6, 0x2, 0x81, 0x30, 0x83, 0xc0,

    /* U+0050 "P" */
    0x3e, 0xd1, 0x11, 0x12, 0x1c, 0x10, 0x10, 0x10,
    0x10, 0x10,

    /* U+0051 "Q" */
    0x6, 0x6, 0x62, 0xd, 0x1, 0x40, 0x60, 0x18,
    0x26, 0xa, 0x82, 0xa0, 0xc4, 0x60, 0xe8, 0x1,
    0x80, 0x20,

    /* U+0052 "R" */
    0x1f, 0xc3, 0x21, 0x81, 0x4, 0x8, 0x20, 0x43,
    0x2, 0x30, 0x12, 0x0, 0xe0, 0x7, 0x0, 0x27,
    0x1, 0x6, 0x8, 0x0, 0x40, 0x0,

    /* U+0053 "S" */
    0xe, 0x70, 0xc0, 0x80, 0x40, 0x38, 0x6, 0x1,
    0x1, 0x2, 0x7c,

    /* U+0054 "T" */
    0x3, 0xe7, 0xe0, 0x2, 0x0, 0x20, 0x2, 0x0,
    0x20, 0x2, 0x0, 0x20, 0x2, 0x0, 0x20, 0x2,
    0x0,

    /* U+0055 "U" */
    0x0, 0x40, 0xa0, 0x50, 0x28, 0x14, 0x1a, 0xd,
    0xa, 0x8d, 0x44, 0x94, 0x2c, 0x0,

    /* U+0056 "V" */
    0x0, 0x0, 0xa0, 0x48, 0x44, 0x21, 0x10, 0x88,
    0x48, 0x14, 0xa, 0x3, 0x1, 0x0,

    /* U+0057 "W" */
    0x0, 0x20, 0x18, 0x6, 0x11, 0x4c, 0x52, 0x94,
    0xa5, 0x49, 0x31, 0x8c, 0x62, 0x8,

    /* U+0058 "X" */
    0x80, 0x89, 0x21, 0x43, 0x2, 0x4, 0x18, 0x28,
    0x99, 0x12, 0x0,

    /* U+0059 "Y" */
    0x3, 0x5, 0x12, 0x22, 0x43, 0x82, 0x4, 0x8,
    0x10, 0x20,

    /* U+005A "Z" */
    0x3c, 0x44, 0x8, 0xc, 0x70, 0x10, 0x20, 0x20,
    0x40, 0x41, 0x3f,

    /* U+005B "[" */
    0x3c, 0x88, 0x88, 0x88, 0x88, 0x88, 0xb4,

    /* U+005C "\\" */
    0x4, 0x20, 0x84, 0x21, 0x4, 0x21, 0x4, 0x21,
    0x8, 0x20,

    /* U+005D "]" */
    0xf, 0x11, 0x11, 0x11, 0x11, 0x11, 0x3c,

    /* U+005E "^" */
    0x18, 0x30, 0xa2, 0x24, 0x50, 0xa0, 0x0,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0x88, 0x80,

    /* U+0061 "a" */
    0x6, 0x34, 0x8a, 0x34, 0xb1, 0x64, 0xf1,

    /* U+0062 "b" */
    0x81, 0x2, 0x4, 0x8, 0xf2, 0x68, 0xe2, 0xc5,
    0x92, 0x43, 0x0,

    /* U+0063 "c" */
    0x31, 0x10, 0x88, 0xc6, 0x5c,

    /* U+0064 "d" */
    0x4, 0x10, 0x41, 0x1c, 0x94, 0x53, 0x96, 0x5a,
    0x71,

    /* U+0065 "e" */
    0x1c, 0x94, 0xbc, 0x82, 0x18, 0x9c,

    /* U+0066 "f" */
    0xc, 0x24, 0x89, 0x2, 0x4, 0x3f, 0x90, 0x20,
    0x40, 0x81, 0x2, 0x4, 0x0,

    /* U+0067 "g" */
    0x18, 0x49, 0x12, 0x69, 0x32, 0x68, 0xe1, 0x2,
    0x4, 0x10, 0x61, 0x84, 0x0,

    /* U+0068 "h" */
    0x81, 0x2, 0x4, 0x29, 0xb4, 0x68, 0xe1, 0xc3,
    0x6, 0x8,

    /* U+0069 "i" */
    0x81, 0x55, 0x54,

    /* U+006A "j" */
    0x40, 0x0, 0x22, 0x22, 0x11, 0x11, 0x19, 0xe0,

    /* U+006B "k" */
    0x81, 0x2, 0x4, 0x8, 0x73, 0x2c, 0x60, 0x81,
    0x82, 0xc4, 0x70,

    /* U+006C "l" */
    0xff, 0xf0,

    /* U+006D "m" */
    0x4c, 0xca, 0xa9, 0x54, 0xb3, 0x16, 0x62, 0xcc,
    0x51, 0xa, 0x21,

    /* U+006E "n" */
    0x8e, 0x59, 0x59, 0x65, 0x14, 0x41,

    /* U+006F "o" */
    0x1, 0x85, 0xa1, 0x86, 0x18, 0x9c,

    /* U+0070 "p" */
    0xc0, 0xf1, 0x12, 0x14, 0x29, 0xbe, 0x20, 0x40,
    0x81, 0x0,

    /* U+0071 "q" */
    0x0, 0x11, 0xc9, 0x4d, 0x59, 0x69, 0xc4, 0x10,
    0x41,

    /* U+0072 "r" */
    0x9d, 0x31, 0x88, 0x42, 0x10,

    /* U+0073 "s" */
    0x33, 0x8, 0x10, 0x3c, 0x10, 0x9c,

    /* U+0074 "t" */
    0x0, 0x10, 0x10, 0x10, 0x1e, 0xf0, 0x10, 0x10,
    0x10, 0x10, 0x10,

    /* U+0075 "u" */
    0x44, 0x8a, 0x34, 0xa9, 0x54, 0xb1, 0x61,

    /* U+0076 "v" */
    0x0, 0xc0, 0x90, 0x84, 0x42, 0x41, 0x20, 0x50,
    0x28, 0x8, 0x0, 0x0,

    /* U+0077 "w" */
    0x82, 0x10, 0x94, 0x65, 0x29, 0x2a, 0x4c, 0xa3,
    0x18, 0xc4, 0x0, 0x0,

    /* U+0078 "x" */
    0x84, 0x64, 0xc6, 0x31, 0x93, 0x88,

    /* U+0079 "y" */
    0x86, 0x18, 0x63, 0x59, 0xa0, 0x82, 0x8, 0x20,
    0x80,

    /* U+007A "z" */
    0x18, 0x70, 0x10, 0x20, 0x60, 0x40, 0x41, 0x7e,

    /* U+007B "{" */
    0x12, 0x44, 0x42, 0x46, 0x24, 0x88, 0x8c, 0x70,

    /* U+007C "|" */
    0x7f, 0xff, 0x80,

    /* U+007D "}" */
    0xe1, 0x11, 0x26, 0x12, 0x22, 0x11, 0x24, 0x0,

    /* U+007E "~" */
    0x30, 0x24, 0xa3, 0x50, 0xc0,

    /* U+007F "" */
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 73, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 51, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 87, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 6, .adv_w = 156, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 141, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 36, .adv_w = 252, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 164, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 54, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 71, .adv_w = 104, .box_w = 4, .box_h = 15, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 79, .adv_w = 104, .box_w = 5, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 89, .adv_w = 92, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 93, .adv_w = 121, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 54, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 101, .adv_w = 87, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 102, .adv_w = 54, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 93, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 113, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 124, .adv_w = 113, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 133, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 148, .adv_w = 178, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 163, .adv_w = 179, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 180, .adv_w = 142, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 193, .adv_w = 149, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 206, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 218, .adv_w = 177, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 233, .adv_w = 160, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 248, .adv_w = 54, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 254, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 126, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 265, .adv_w = 124, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 256, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 302, .adv_w = 159, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 319, .adv_w = 177, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 336, .adv_w = 140, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 167, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 152, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 151, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 390, .adv_w = 186, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 408, .adv_w = 175, .box_w = 11, .box_h = 16, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 430, .adv_w = 170, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 149, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 459, .adv_w = 147, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 147, .box_w = 8, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 480, .adv_w = 207, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 177, .box_w = 7, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 183, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 519, .adv_w = 142, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 529, .adv_w = 180, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 547, .adv_w = 160, .box_w = 13, .box_h = 13, .ofs_x = -3, .ofs_y = -2},
    {.bitmap_index = 569, .adv_w = 141, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 580, .adv_w = 180, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 166, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 611, .adv_w = 140, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 625, .adv_w = 182, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 99, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 650, .adv_w = 124, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 153, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 671, .adv_w = 104, .box_w = 4, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 678, .adv_w = 93, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 688, .adv_w = 104, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 695, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 702, .adv_w = 121, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 703, .adv_w = 118, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 10},
    {.bitmap_index = 705, .adv_w = 135, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 737, .adv_w = 119, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 108, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 756, .adv_w = 139, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -6},
    {.bitmap_index = 769, .adv_w = 143, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 56, .box_w = 2, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 782, .adv_w = 56, .box_w = 4, .box_h = 15, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 790, .adv_w = 124, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 801, .adv_w = 51, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 803, .adv_w = 192, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 110, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 826, .adv_w = 133, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 836, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 845, .adv_w = 106, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 850, .adv_w = 114, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 126, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 874, .adv_w = 137, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 886, .adv_w = 183, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 898, .adv_w = 99, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 904, .adv_w = 109, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 913, .adv_w = 135, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 104, .box_w = 4, .box_h = 15, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 929, .adv_w = 93, .box_w = 1, .box_h = 17, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 932, .adv_w = 104, .box_w = 4, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 940, .adv_w = 147, .box_w = 9, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 945, .adv_w = 0, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 96, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_Font1 = {
#else
lv_font_t ui_font_Font1 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 6,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_FONT1*/

