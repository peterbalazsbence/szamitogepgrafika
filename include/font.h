#ifndef DOOM3D_FONT_H
#define DOOM3D_FONT_H

#include "common.h"

extern const unsigned long long FONT_GLYPHS[128];

void draw_char(float px, float py, float size, char ch);
void draw_text(float x, float y, float size, const char *text);

#endif
