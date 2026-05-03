#include "font.h"

unsigned long long FONT_GLYPHS[128];

/* Maps a CSV "char" cell to its actual ASCII code.
 * Special tokens "SPACE" and "COMMA" are used because those characters
 * conflict with the CSV separator or are awkward in plain text. */
static int parse_char_token(const char *tok) {
    if(strcmp(tok, "SPACE") == 0) return ' ';
    if(strcmp(tok, "COMMA") == 0) return ',';
    if(strlen(tok) == 1) return (unsigned char)tok[0];
    return -1;
}

int load_font_from_csv(const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        fprintf(stderr, "Cannot open font file: %s\n", path);
        return 0;
    }
    char line[128];
    /* Skip header line */
    if(!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    int loaded = 0;
    while(fgets(line, sizeof(line), f)) {
        /* Strip newline */
        size_t len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = 0;
        }
        if(len == 0) continue;

        char tok[16];
        char hex[32];
        if(sscanf(line, "%15[^,],%31s", tok, hex) != 2) continue;

        int ch = parse_char_token(tok);
        if(ch < 0 || ch >= 128) continue;

        unsigned long long bits = 0;
        sscanf(hex, "%llx", &bits);
        FONT_GLYPHS[ch] = bits;
        loaded++;
    }
    fclose(f);
    printf("Loaded font: %s (%d glyphs)\n", path, loaded);
    return 1;
}

void draw_char(float px, float py, float size, char ch) {
    unsigned char uch = (unsigned char)ch;
    if(uch >= 128) return;
    unsigned long long g = FONT_GLYPHS[uch];
    if(!g && ch != ' ') return;

    for(int row=0;row<8;row++) {
        unsigned char bits = (unsigned char)((g >> (56 - row*8)) & 0xFF);
        for(int col=0;col<8;col++) {
            if(bits & (0x80 >> col)) {
                float x = px + col*size;
                float y = py + (7-row)*size;
                glBegin(GL_QUADS);
                glVertex2f(x,      y);
                glVertex2f(x+size, y);
                glVertex2f(x+size, y+size);
                glVertex2f(x,      y+size);
                glEnd();
            }
        }
    }
}

void draw_text(float x, float y, float size, const char *text) {
    for(int i=0;text[i];i++)
        draw_char(x + i*size*9, y, size, text[i]);
}
