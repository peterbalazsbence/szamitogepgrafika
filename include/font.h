#ifndef DOOM3D_FONT_H
#define DOOM3D_FONT_H

#include "common.h"

/* 8x8 bitmap font glyphs. Each entry encodes 8 rows in a uint64
 * (high byte = top row, bit 7 = leftmost pixel).
 * Loaded at startup from assets/font.csv. */
extern unsigned long long FONT_GLYPHS[128];

/* Loads glyph definitions from a CSV file with two columns:
 *   character (or "SPACE", "COMMA"), 14-hex-digit bitmap.
 * Returns 1 on success, 0 if the file is missing or malformed. */
int load_font_from_csv(const char *path);

/* Renders a single character as small quads. */
void draw_char(float px, float py, float size, char ch);

/* Renders a string. Characters are spaced 9*size pixels apart. */
void draw_text(float x, float y, float size, const char *text);

#endif
