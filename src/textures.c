#include "textures.h"

GLuint tex_wall, tex_floor, tex_ceil, tex_enemy, tex_crate, tex_barrel;
GLuint tex_pickup_health, tex_pickup_ammo, tex_pickup_armor;
GLuint tex_window;
GLuint tex_shadow;

/* Generic helper: allocate RGB pixel buffer, run fill fn per pixel, upload to GL */
static void make_texture(GLuint *out, int w, int h,
                         void (*fill)(unsigned char*, int, int, int, int))
{
    unsigned char *px = (unsigned char*)malloc(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) fill(px, x, y, w, h);
    glGenTextures(1, out);
    glBindTexture(GL_TEXTURE_2D, *out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    free(px);
}

/* Same but for RGBA (used for transparent window and soft shadow) */
static void make_texture_rgba(GLuint *out, int w, int h,
                              void (*fill)(unsigned char*, int, int, int, int))
{
    unsigned char *px = (unsigned char*)malloc(w*h*4);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) fill(px, x, y, w, h);
    glGenTextures(1, out);
    glBindTexture(GL_TEXTURE_2D, *out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    free(px);
}

/* ─── Texture fill functions (one per texture) ─── */

static void fill_wall(unsigned char *px, int x, int y, int w, int h) {
    int bw=w/4, bh=h/4, row=y/bh, ox=(row%2)*(bw/2);
    int mx=(x+ox)%bw, my=y%bh;
    int mortar = (mx<2)||(my<2);
    int idx = 3*(y*w+x);
    if(mortar) { px[idx]=160; px[idx+1]=150; px[idx+2]=140; }
    else {
        int v = 180 + (rand()%20 - 10);
        px[idx]=(unsigned char)v;
        px[idx+1]=(unsigned char)(v-30);
        px[idx+2]=(unsigned char)(v-50);
    }
    (void)h;
}

static void fill_floor(unsigned char *px, int x, int y, int w, int h) {
    int tile = ((x/(w/8))+(y/(h/8))) % 2;
    int v = tile ? 195 : 175;
    int noise = rand()%8 - 4;
    int idx = 3*(y*w+x);
    px[idx]   = (unsigned char)(v + noise);
    px[idx+1] = (unsigned char)(v + noise - 5);
    px[idx+2] = (unsigned char)(v + noise - 10);
    (void)h;
}

static void fill_ceil(unsigned char *px, int x, int y, int w, int h) {
    int v = 50 + ((x*3 + y*5) % 20);
    int idx = 3*(y*w+x);
    px[idx]=(unsigned char)v;
    px[idx+1]=(unsigned char)v;
    px[idx+2]=(unsigned char)(v+10);
    (void)h;
}

static void fill_enemy_tex(unsigned char *px, int x, int y, int w, int h) {
    float cx = x/(float)w, cy = y/(float)h;
    int idx = 3*(y*w+x);
    px[idx]=160; px[idx+1]=30; px[idx+2]=30; /* dark red body */
    /* Yellow eyes */
    if(((cx>0.2f&&cx<0.35f)||(cx>0.65f&&cx<0.8f)) && cy>0.3f && cy<0.5f)
        { px[idx]=255; px[idx+1]=220; px[idx+2]=0; }
    /* Dark mouth */
    if(cx>0.2f && cx<0.8f && cy>0.65f && cy<0.75f)
        { px[idx]=20; px[idx+1]=20; px[idx+2]=20; }
    /* Horn caps */
    if(((cx>0.1f&&cx<0.25f)||(cx>0.75f&&cx<0.9f)) && cy>0.05f && cy<0.25f)
        { px[idx]=100; px[idx+1]=20; px[idx+2]=20; }
    (void)h;
}

static void fill_crate(unsigned char *px, int x, int y, int w, int h) {
    int bdr   = (x<3||x>=w-3||y<3||y>=h-3);
    int cross = (abs(x-w/2)<3)||(abs(y-h/2)<3);
    int idx = 3*(y*w+x);
    if(bdr||cross) { px[idx]=100; px[idx+1]=70; px[idx+2]=30; }
    else {
        int v = 160 + (rand()%20 - 10);
        px[idx]=(unsigned char)v;
        px[idx+1]=(unsigned char)(v-20);
        px[idx+2]=(unsigned char)(v-60);
    }
    (void)h;
}

static void fill_barrel_tex(unsigned char *px, int x, int y, int w, int h) {
    int stripe = ((y/(h/6))%2 == 0);
    int idx = 3*(y*w+x);
    if(stripe) { px[idx]=80;  px[idx+1]=100; px[idx+2]=80; }
    else       { px[idx]=60;  px[idx+1]=80;  px[idx+2]=60; }
    if(y%(h/6) < 2) { px[idx]=120; px[idx+1]=120; px[idx+2]=130; } /* metal band */
    (void)w;
}

static void fill_pickup_health(unsigned char *px, int x, int y, int w, int h) {
    int idx = 3*(y*w+x);
    float cx = x/(float)w - 0.5f, cy = y/(float)h - 0.5f;
    int cross = (fabsf(cx)<0.15f && fabsf(cy)<0.4f) ||
                (fabsf(cy)<0.15f && fabsf(cx)<0.4f);
    if(cross) { px[idx]=30;  px[idx+1]=220; px[idx+2]=30; }
    else      { px[idx]=200; px[idx+1]=200; px[idx+2]=200; }
    (void)h;
}

static void fill_pickup_ammo(unsigned char *px, int x, int y, int w, int h) {
    int idx = 3*(y*w+x);
    float cx = x/(float)w, cy = y/(float)h;
    int bullet = (cy>0.3f && cy<0.8f) &&
                 ((cx>0.15f&&cx<0.3f) || (cx>0.4f&&cx<0.55f) || (cx>0.65f&&cx<0.8f));
    int tip = (cy>0.1f && cy<0.3f) &&
              ((cx>0.18f&&cx<0.27f) || (cx>0.43f&&cx<0.52f) || (cx>0.68f&&cx<0.77f));
    if(bullet)   { px[idx]=200; px[idx+1]=170; px[idx+2]=50; }
    else if(tip) { px[idx]=220; px[idx+1]=140; px[idx+2]=40; }
    else         { px[idx]=60;  px[idx+1]=60;  px[idx+2]=60; }
    (void)h;
}

static void fill_pickup_armor(unsigned char *px, int x, int y, int w, int h) {
    int idx = 3*(y*w+x);
    float cx = x/(float)w - 0.5f, cy = y/(float)h - 0.5f;
    float d = cx*cx + cy*cy;
    if(d < 0.18f && cy < 0.1f) { px[idx]=50;  px[idx+1]=80;  px[idx+2]=220; }
    else                       { px[idx]=100; px[idx+1]=100; px[idx+2]=100; }
    (void)h;
}

/* RGBA window - frame is opaque, glass is semi-transparent */
static void fill_window(unsigned char *px, int x, int y, int w, int h) {
    int idx = 4*(y*w+x);
    int frame = (x<3 || x>=w-3 || y<3 || y>=h-3);
    int bars  = (abs(x-w/2)<2) || (abs(y-h/2)<2);
    if(frame || bars) {
        /* Dark metal frame - fully opaque */
        px[idx]=80; px[idx+1]=85; px[idx+2]=90; px[idx+3]=255;
    } else {
        /* Glass pane - blue tint, partially transparent */
        int shimmer = ((x+y)%8 < 4) ? 10 : 0;
        px[idx]  =(unsigned char)(120+shimmer);
        px[idx+1]=(unsigned char)(140+shimmer);
        px[idx+2]=(unsigned char)(180+shimmer);
        px[idx+3]=100;
    }
    (void)h;
}

/* RGBA soft shadow: dark center, transparent edges (radial alpha falloff) */
static void fill_shadow(unsigned char *px, int x, int y, int w, int h) {
    int idx = 4*(y*w+x);
    float cx = x/(float)w - 0.5f, cy = y/(float)h - 0.5f;
    float d = sqrtf(cx*cx + cy*cy) * 2.0f; /* 0 at center, 1 at corner */
    if(d > 1.0f) d = 1.0f;
    float alpha = (1.0f - d*d) * 0.6f;
    px[idx]=0; px[idx+1]=0; px[idx+2]=0;
    px[idx+3]=(unsigned char)(alpha*255);
    (void)h;
}

void init_textures(void) {
    srand(42); /* fixed seed for reproducible textures */
    make_texture(&tex_wall,          64, 64, fill_wall);
    make_texture(&tex_floor,         64, 64, fill_floor);
    make_texture(&tex_ceil,          64, 64, fill_ceil);
    make_texture(&tex_enemy,         64, 64, fill_enemy_tex);
    make_texture(&tex_crate,         64, 64, fill_crate);
    make_texture(&tex_barrel,        64, 64, fill_barrel_tex);
    make_texture(&tex_pickup_health, 32, 32, fill_pickup_health);
    make_texture(&tex_pickup_ammo,   32, 32, fill_pickup_ammo);
    make_texture(&tex_pickup_armor,  32, 32, fill_pickup_armor);
    make_texture_rgba(&tex_window,   64, 64, fill_window);
    make_texture_rgba(&tex_shadow,   32, 32, fill_shadow);
}

void free_textures(void) {
    glDeleteTextures(1, &tex_wall);
    glDeleteTextures(1, &tex_floor);
    glDeleteTextures(1, &tex_ceil);
    glDeleteTextures(1, &tex_enemy);
    glDeleteTextures(1, &tex_crate);
    glDeleteTextures(1, &tex_barrel);
    glDeleteTextures(1, &tex_pickup_health);
    glDeleteTextures(1, &tex_pickup_ammo);
    glDeleteTextures(1, &tex_pickup_armor);
    glDeleteTextures(1, &tex_window);
    glDeleteTextures(1, &tex_shadow);
}
