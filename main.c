/*
 * DOOM-LIKE 3D FPS GAME
 * Built with C, SDL2, and OpenGL
 *
 * Controls:
 *   W/S        - Move forward/backward
 *   A/D        - Strafe left/right
 *   Mouse      - Look around (yaw + pitch)
 *   Left Click - Shoot
 *   ESC        - Quit
 */

#ifdef _WIN32
  #include <windows.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ─────────────────────────────── Constants ─────────────────────────────── */

#define WINDOW_W      1280
#define WINDOW_H      720
#define WINDOW_TITLE  "DOOM3D"
#define FOV           70.0f
#define NEAR_CLIP     0.1f
#define FAR_CLIP      100.0f
#define MOVE_SPEED    5.0f
#define TURN_SPEED    0.002f
#define GRAVITY       18.0f
#define JUMP_VEL      7.0f
#define PLAYER_HEIGHT 1.7f
#define PLAYER_RADIUS 0.3f
#define FLOOR_Y       0.0f
#define CEIL_Y        3.0f

#define MAX_ENEMIES   16
#define ENEMY_SPEED   2.0f
#define ENEMY_HEALTH  3
#define SHOOT_COOLDOWN 0.25f
#define MUZZLE_TIME   0.08f

#define PI  3.14159265358979f
#define DEG2RAD(x) ((x)*PI/180.0f)

/* ───────────────────────────────── Math ────────────────────────────────── */

typedef struct { float x, y, z; } Vec3;

static inline Vec3 v3(float x, float y, float z) { Vec3 v={x,y,z}; return v; }
static inline Vec3 v3add(Vec3 a, Vec3 b) { return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline Vec3 v3sub(Vec3 a, Vec3 b) { return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline Vec3 v3scale(Vec3 a, float s) { return v3(a.x*s, a.y*s, a.z*s); }
static inline float v3dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float v3len(Vec3 a) { return sqrtf(v3dot(a,a)); }
static inline Vec3 v3norm(Vec3 a) { float l=v3len(a); if(l<1e-6f) return v3(0,0,0); return v3scale(a,1.0f/l); }
static inline float v3dist2d(Vec3 a, Vec3 b) { float dx=a.x-b.x,dz=a.z-b.z; return sqrtf(dx*dx+dz*dz); }

/* ──────────────────────────────── Level ────────────────────────────────── */

/*  Map key:
 *  '#' = wall
 *  '.' = floor (empty)
 *  'E' = enemy spawn
 *  'P' = player spawn
 */
static const char *MAP_ROWS[] = {
    "####################",
    "#P.................#",
    "#..................#",
    "#....####..........#",
    "#....#..#..........#",
    "#....#..#..........#",
    "#....####..........#",
    "#.......E..........#",
    "#..................#",
    "#.....#####........#",
    "#.....#...#........#",
    "#.....#.E.#........#",
    "#.....#...#........#",
    "#.....#####........#",
    "#..................#",
    "#...E..............#",
    "#..................#",
    "#..######..........#",
    "#..................#",
    "####################",
};
#define MAP_ROWS_N  20
#define MAP_COLS    20

static int map_is_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    return MAP_ROWS[row][col]=='#';
}

/* ──────────────────────────── Texture gen ──────────────────────────────── */

/* Procedurally create simple brick / floor textures */
static GLuint tex_wall, tex_floor, tex_ceil, tex_enemy;

static void make_texture(GLuint *out, int w, int h,
                         void (*fill)(unsigned char*,int,int,int,int))
{
    unsigned char *px = malloc(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) fill(px,x,y,w,h);
    glGenTextures(1,out);
    glBindTexture(GL_TEXTURE_2D,*out);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,px);
    free(px);
}

static void fill_wall(unsigned char *px, int x, int y, int w, int h) {
    int bw=w/4, bh=h/4;
    int row=y/bh, col=x/bw;
    int mx=x%bw, my=y%bh;
    int mortar = (mx<2)||(my<2);
    int ox=(row%2)*(bw/2);
    col=(x+ox)/(bw); mx=(x+ox)%bw;
    mortar = (mx<2)||(my<2);
    if(mortar){ px[3*(y*w+x)+0]=160; px[3*(y*w+x)+1]=150; px[3*(y*w+x)+2]=140; }
    else       { int v=180+(rand()%20-10);
                 px[3*(y*w+x)+0]=v; px[3*(y*w+x)+1]=v-30; px[3*(y*w+x)+2]=v-50; }
    (void)col;(void)h;
}

static void fill_floor(unsigned char *px, int x, int y, int w, int h) {
    int tile=((x/(w/8))+(y/(h/8)))%2;
    int v = tile ? 90 : 70;
    px[3*(y*w+x)+0]=v; px[3*(y*w+x)+1]=v+5; px[3*(y*w+x)+2]=v;
    (void)h;
}

static void fill_ceil(unsigned char *px, int x, int y, int w, int h) {
    int v=50+((x*3+y*5)%20);
    px[3*(y*w+x)+0]=v; px[3*(y*w+x)+1]=v; px[3*(y*w+x)+2]=v+10;
    (void)h;
}

static void fill_enemy(unsigned char *px, int x, int y, int w, int h) {
    /* Simple pixel-art demon face */
    float cx=x/(float)w, cy=y/(float)h;
    /* background = dark red */
    px[3*(y*w+x)+0]=160; px[3*(y*w+x)+1]=30; px[3*(y*w+x)+2]=30;
    /* eyes */
    if((cx>0.2f&&cx<0.35f||cx>0.65f&&cx<0.8f)&&cy>0.3f&&cy<0.5f){
        px[3*(y*w+x)+0]=255; px[3*(y*w+x)+1]=220; px[3*(y*w+x)+2]=0;
    }
    /* mouth */
    if(cx>0.2f&&cx<0.8f&&cy>0.65f&&cy<0.75f){
        px[3*(y*w+x)+0]=20; px[3*(y*w+x)+1]=20; px[3*(y*w+x)+2]=20;
    }
    (void)h;
}

static void init_textures(void) {
    srand(42);
    make_texture(&tex_wall,  64,64,fill_wall);
    make_texture(&tex_floor, 64,64,fill_floor);
    make_texture(&tex_ceil,  64,64,fill_ceil);
    make_texture(&tex_enemy, 32,32,fill_enemy);
}

/* ──────────────────────────────── Enemy ────────────────────────────────── */

typedef struct {
    Vec3  pos;
    int   health;
    float pain_timer;   /* flash red when hit */
    int   alive;
} Enemy;

static Enemy enemies[MAX_ENEMIES];
static int   enemy_count = 0;

static void enemy_spawn(float x, float z) {
    if(enemy_count>=MAX_ENEMIES) return;
    enemies[enemy_count].pos    = v3(x, PLAYER_HEIGHT*0.5f, z);
    enemies[enemy_count].health = ENEMY_HEALTH;
    enemies[enemy_count].alive  = 1;
    enemies[enemy_count].pain_timer = 0;
    enemy_count++;
}

/* ──────────────────────────────── Player ───────────────────────────────── */

typedef struct {
    Vec3  pos;
    float yaw;      /* left-right, radians */
    float pitch;    /* up-down,   radians  */
    float vy;       /* vertical velocity   */
    int   on_ground;
    int   health;
    int   ammo;
    float shoot_cd;
    float muzzle_flash;
} Player;

static Player player;

static Vec3 player_forward(void) {
    return v3(sinf(player.yaw)*cosf(player.pitch),
              sinf(player.pitch),
              cosf(player.yaw)*cosf(player.pitch));
}
static Vec3 player_right(void) {
    return v3(cosf(player.yaw), 0, -sinf(player.yaw));
}

/* ─────────────────────────── Collision helper ──────────────────────────── */

static int collides_map(float x, float z) {
    int col=(int)floorf(x), row=(int)floorf(z);
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++)
        if(map_is_wall(col+dc, row+dr)) {
            float wx=col+dc, wz=row+dr;
            if(x>wx-PLAYER_RADIUS && x<wx+1+PLAYER_RADIUS &&
               z>wz-PLAYER_RADIUS && z<wz+1+PLAYER_RADIUS)
                return 1;
        }
    return 0;
}

/* ──────────────────────────── Init from map ────────────────────────────── */

static void init_level(void) {
    enemy_count=0;
    for(int r=0;r<MAP_ROWS_N;r++) {
        for(int c=0;c<MAP_COLS;c++) {
            char ch=MAP_ROWS[r][c];
            if(ch=='P') {
                player.pos    = v3(c+0.5f, PLAYER_HEIGHT, r+0.5f);
                player.yaw    = 0;
                player.pitch  = 0;
                player.vy     = 0;
                player.on_ground = 1;
                player.health = 100;
                player.ammo   = 50;
                player.shoot_cd = 0;
                player.muzzle_flash = 0;
            } else if(ch=='E') {
                enemy_spawn(c+0.5f, r+0.5f);
            }
        }
    }
}

/* ──────────────────────────── Game update ──────────────────────────────── */

static int game_over = 0;
static int game_win  = 0;

static void shoot(void) {
    if(player.shoot_cd>0||player.ammo<=0) return;
    player.ammo--;
    player.shoot_cd   = SHOOT_COOLDOWN;
    player.muzzle_flash = MUZZLE_TIME;

    /* Ray cast toward crosshair */
    Vec3 fwd = player_forward();
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 d = v3sub(enemies[i].pos, player.pos);
        /* Project onto forward */
        float along = v3dot(d, fwd);
        if(along<0.5f) continue;
        Vec3 closest = v3sub(d, v3scale(fwd, along));
        if(v3len(closest)<0.55f) {           /* hit radius */
            enemies[i].health--;
            enemies[i].pain_timer = 0.2f;
            if(enemies[i].health<=0) {
                enemies[i].alive=0;
                /* check win */
                int alive=0;
                for(int j=0;j<enemy_count;j++) alive+=enemies[j].alive;
                if(!alive) game_win=1;
            }
        }
    }
}

static void update(float dt) {
    if(game_over||game_win) return;

    /* Keyboard */
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    Vec3 fwd_xz = v3(sinf(player.yaw),0,cosf(player.yaw));
    Vec3 rgt_xz = v3(cosf(player.yaw),0,-sinf(player.yaw));
    Vec3 move   = v3(0,0,0);
    if(keys[SDL_SCANCODE_W]) move=v3add(move,fwd_xz);
    if(keys[SDL_SCANCODE_S]) move=v3sub(move,fwd_xz);
    if(keys[SDL_SCANCODE_A]) move=v3add(move,rgt_xz);
    if(keys[SDL_SCANCODE_D]) move=v3sub(move,rgt_xz);

    float ml=v3len(move);
    if(ml>0) {
        move=v3scale(move,MOVE_SPEED*dt/ml);
        float nx=player.pos.x+move.x;
        float nz=player.pos.z+move.z;
        if(!collides_map(nx, player.pos.z)) player.pos.x=nx;
        if(!collides_map(player.pos.x, nz)) player.pos.z=nz;
    }

    /* Gravity */
    if(!player.on_ground) player.vy-=GRAVITY*dt;
    player.pos.y+=player.vy*dt;
    if(player.pos.y<=PLAYER_HEIGHT) {
        player.pos.y=PLAYER_HEIGHT;
        player.vy=0;
        player.on_ground=1;
    }

    /* Jump */
    if(keys[SDL_SCANCODE_SPACE]&&player.on_ground){
        player.vy=JUMP_VEL;
        player.on_ground=0;
    }

    /* Shoot cooldown */
    if(player.shoot_cd>0) player.shoot_cd-=dt;
    if(player.muzzle_flash>0) player.muzzle_flash-=dt;

    /* Enemies */
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        if(enemies[i].pain_timer>0) enemies[i].pain_timer-=dt;

        /* Simple chase AI */
        Vec3 d=v3sub(player.pos, enemies[i].pos);
        d.y=0;
        float dist=v3len(d);
        if(dist>0.01f&&dist<15.0f) {
            Vec3 dir=v3scale(d,1.0f/dist);
            float spd=ENEMY_SPEED*dt;
            float nx=enemies[i].pos.x+dir.x*spd;
            float nz=enemies[i].pos.z+dir.z*spd;
            if(!collides_map(nx,enemies[i].pos.z)) enemies[i].pos.x=nx;
            if(!collides_map(enemies[i].pos.x,nz)) enemies[i].pos.z=nz;
        }

        /* Damage player on contact */
        if(dist<0.8f) {
            player.health-=(int)(20*dt);
            if(player.health<=0) { player.health=0; game_over=1; }
        }
    }
}

/* ──────────────────────────────── Render ───────────────────────────────── */

static void set_projection(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (float)WINDOW_W/WINDOW_H, NEAR_CLIP, FAR_CLIP);
    glMatrixMode(GL_MODELVIEW);
}

static void set_camera(void) {
    glLoadIdentity();
    Vec3 eye=player.pos;
    Vec3 fwd=player_forward();
    Vec3 center=v3add(eye,fwd);
    gluLookAt(eye.x,eye.y,eye.z, center.x,center.y,center.z, 0,1,0);
}

/* Draw a textured quad given 4 vertices (CCW) */
static void quad(float x0,float y0,float z0,
                 float x1,float y1,float z1,
                 float x2,float y2,float z2,
                 float x3,float y3,float z3,
                 float us, float vs)   /* texture scale */
{
    glBegin(GL_QUADS);
    glTexCoord2f(0,   0  ); glVertex3f(x0,y0,z0);
    glTexCoord2f(us,  0  ); glVertex3f(x1,y1,z1);
    glTexCoord2f(us,  vs ); glVertex3f(x2,y2,z2);
    glTexCoord2f(0,   vs ); glVertex3f(x3,y3,z3);
    glEnd();
}

static void draw_level(void) {
    glEnable(GL_TEXTURE_2D);

    for(int r=0;r<MAP_ROWS_N;r++) {
        for(int c=0;c<MAP_COLS;c++) {

            float x0=(float)c,   z0=(float)r;
            float x1=(float)c+1, z1=(float)r+1;

            if(!map_is_wall(c,r)) {
                /* Floor */
                glBindTexture(GL_TEXTURE_2D, tex_floor);
                glColor3f(0.9f,0.9f,0.9f);
                quad(x0,FLOOR_Y,z0, x1,FLOOR_Y,z0,
                     x1,FLOOR_Y,z1, x0,FLOOR_Y,z1, 1,1);

                /* Ceiling */
                glBindTexture(GL_TEXTURE_2D, tex_ceil);
                glColor3f(0.6f,0.6f,0.7f);
                quad(x0,CEIL_Y,z1, x1,CEIL_Y,z1,
                     x1,CEIL_Y,z0, x0,CEIL_Y,z0, 1,1);
            }

            if(map_is_wall(c,r)) {
                glBindTexture(GL_TEXTURE_2D, tex_wall);
                glColor3f(1,1,1);

                /* Check each neighbouring cell - draw face if neighbour is open */
                /* North face (z-1) */
                if(!map_is_wall(c,r-1))
                    quad(x0,CEIL_Y,z0, x1,CEIL_Y,z0,
                         x1,FLOOR_Y,z0, x0,FLOOR_Y,z0, 1,1);
                /* South face (z+1) */
                if(!map_is_wall(c,r+1))
                    quad(x1,CEIL_Y,z1, x0,CEIL_Y,z1,
                         x0,FLOOR_Y,z1, x1,FLOOR_Y,z1, 1,1);
                /* West face (c-1) */
                if(!map_is_wall(c-1,r))
                    quad(x0,CEIL_Y,z1, x0,CEIL_Y,z0,
                         x0,FLOOR_Y,z0, x0,FLOOR_Y,z1, 1,1);
                /* East face (c+1) */
                if(!map_is_wall(c+1,r))
                    quad(x1,CEIL_Y,z0, x1,CEIL_Y,z1,
                         x1,FLOOR_Y,z1, x1,FLOOR_Y,z0, 1,1);
            }
        }
    }
    glDisable(GL_TEXTURE_2D);
}

/* Billboard sprite for enemy */
static void draw_enemies(void) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_enemy);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Get camera right & up from modelview */
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float rx=mv[0], ry=mv[4], rz=mv[8]; /* right */
    /* up is world Y */

    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;

        Vec3 p=enemies[i].pos;
        float w=0.8f, h=1.6f;

        if(enemies[i].pain_timer>0) glColor3f(1,0.3f,0.3f);
        else                         glColor3f(1,1,1);

        glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex3f(p.x-rx*w, p.y-h*0.5f, p.z-rz*w);
        glTexCoord2f(1,1); glVertex3f(p.x+rx*w, p.y-h*0.5f, p.z+rz*w);
        glTexCoord2f(1,0); glVertex3f(p.x+rx*w, p.y+h*0.5f, p.z+rz*w);
        glTexCoord2f(0,0); glVertex3f(p.x-rx*w, p.y+h*0.5f, p.z-rz*w);
        glEnd();

        (void)ry;
    }
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

/* ────────────────────────────────── HUD ────────────────────────────────── */

static void ortho_begin(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0,WINDOW_W,0,WINDOW_H,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}
static void ortho_end(void) {
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

static void fill_rect(float x,float y,float w,float h,float r,float g,float b,float a){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
    glDisable(GL_BLEND);
}

static void draw_hud(void) {
    ortho_begin();

    /* Crosshair */
    int cx=WINDOW_W/2, cy=WINDOW_H/2;
    fill_rect(cx-12,cy-2,  10,4,  1,1,1,0.8f);
    fill_rect(cx+2, cy-2,  10,4,  1,1,1,0.8f);
    fill_rect(cx-2, cy-12, 4, 10, 1,1,1,0.8f);
    fill_rect(cx-2, cy+2,  4, 10, 1,1,1,0.8f);

    /* Muzzle flash */
    if(player.muzzle_flash>0) {
        float a=player.muzzle_flash/MUZZLE_TIME;
        fill_rect(0,0,WINDOW_W,WINDOW_H, 1,0.8f,0,0.15f*a);
    }

    /* Health bar background */
    fill_rect(20, 20, 204, 24, 0.2f,0.2f,0.2f,0.8f);
    float hp_ratio = player.health/100.0f;
    float hcr = hp_ratio<0.4f ? 1.0f : 0.2f;
    float hcg = hp_ratio>0.6f ? 0.8f : (hp_ratio*1.5f);
    fill_rect(22, 22, 200*hp_ratio, 20, hcr, hcg, 0.1f, 1.0f);

    /* Ammo bar */
    fill_rect(20, 50, 154, 18, 0.2f,0.2f,0.2f,0.8f);
    fill_rect(22, 52, (player.ammo/50.0f)*150, 14, 0.9f,0.7f,0.1f,1.0f);

    /* Enemy count (alive) */
    int alive=0;
    for(int i=0;i<enemy_count;i++) alive+=enemies[i].alive;

    /* Overlay messages */
    if(game_over) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-40, 400, 80, 0.8f,0.05f,0.05f,0.85f);
    }
    if(game_win) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-40, 400, 80, 0.05f,0.6f,0.05f,0.85f);
    }

    ortho_end();
}

/* ───────────────────────────────── Main ────────────────────────────────── */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* SDL init */
    if(SDL_Init(SDL_INIT_VIDEO)<0){
        fprintf(stderr,"SDL_Init: %s\n",SDL_GetError()); return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);

    SDL_Window *win = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    if(!win){ fprintf(stderr,"SDL_CreateWindow: %s\n",SDL_GetError()); return 1; }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if(!ctx){ fprintf(stderr,"SDL_GL_CreateContext: %s\n",SDL_GetError()); return 1; }

    SDL_GL_SetSwapInterval(1);          /* vsync */
    SDL_SetRelativeMouseMode(SDL_TRUE); /* capture mouse */

    /* OpenGL state */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.05f,0.05f,0.1f,1);

    /* Simple directional lighting via glColor (no fixed-function light needed) */

    init_textures();
    init_level();
    set_projection();

    Uint32 prev = SDL_GetTicks();
    int running=1;

    while(running) {
        Uint32 now=SDL_GetTicks();
        float dt=(now-prev)/1000.0f;
        if(dt>0.05f) dt=0.05f;   /* cap dt */
        prev=now;

        /* Events */
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT) running=0;
            if(ev.type==SDL_KEYDOWN&&ev.key.keysym.sym==SDLK_ESCAPE) running=0;
            if(ev.type==SDL_MOUSEMOTION && !game_over && !game_win) {
                player.yaw   -= ev.motion.xrel * TURN_SPEED;
                player.pitch -= ev.motion.yrel * TURN_SPEED;
                if(player.pitch> 1.4f) player.pitch= 1.4f;
                if(player.pitch<-1.4f) player.pitch=-1.4f;
            }
            if(ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT)
                shoot();
            /* Restart */
            if((game_over||game_win)&&ev.type==SDL_KEYDOWN&&
               ev.key.keysym.sym==SDLK_r) {
                game_over=0; game_win=0;
                init_level();
            }
        }

        update(dt);

        /* Draw */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        set_camera();
        draw_level();
        draw_enemies();
        draw_hud();
        SDL_GL_SwapWindow(win);
    }

    /* Cleanup */
    glDeleteTextures(1,&tex_wall);
    glDeleteTextures(1,&tex_floor);
    glDeleteTextures(1,&tex_ceil);
    glDeleteTextures(1,&tex_enemy);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}