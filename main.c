/*
 * DOOM-LIKE 3D FPS GAME
 * Built with C, SDL2, and OpenGL
 *
 * Controls:
 *   W/S         - Move forward/backward
 *   A/D         - Strafe left/right
 *   Mouse       - Look around (yaw + pitch)
 *   Left Click  - Shoot
 *   Space       - Jump
 *   +/-         - Increase/decrease light brightness
 *   L           - Cycle light color (white/red/blue)
 *   F           - Toggle flashlight (player-attached spotlight)
 *   Arrow Keys  - Move the scene light
 *   F1          - Toggle help screen
 *   R           - Restart (when dead or won)
 *   ESC         - Quit
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

#define MAX_DECORATIONS 32

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

/* ──────────────────────── OBJ Model Loader ─────────────────────────────── */

#define MAX_OBJ_VERTS   4096
#define MAX_OBJ_NORMS   4096
#define MAX_OBJ_UVS     4096
#define MAX_OBJ_FACES   4096

typedef struct {
    int vi[4], ti[4], ni[4];
    int count; /* 3 = triangle, 4 = quad */
} ObjFace;

typedef struct {
    float verts[MAX_OBJ_VERTS][3];
    float norms[MAX_OBJ_NORMS][3];
    float uvs[MAX_OBJ_UVS][2];
    ObjFace faces[MAX_OBJ_FACES];
    int vert_count, norm_count, uv_count, face_count;
    GLuint display_list;
} ObjModel;

static int obj_load(ObjModel *m, const char *path) {
    memset(m, 0, sizeof(*m));
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Cannot open OBJ: %s\n", path);
        return 0;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0]=='v' && line[1]==' ') {
            if (m->vert_count < MAX_OBJ_VERTS) {
                sscanf(line+2, "%f %f %f",
                       &m->verts[m->vert_count][0],
                       &m->verts[m->vert_count][1],
                       &m->verts[m->vert_count][2]);
                m->vert_count++;
            }
        } else if (line[0]=='v' && line[1]=='n') {
            if (m->norm_count < MAX_OBJ_NORMS) {
                sscanf(line+3, "%f %f %f",
                       &m->norms[m->norm_count][0],
                       &m->norms[m->norm_count][1],
                       &m->norms[m->norm_count][2]);
                m->norm_count++;
            }
        } else if (line[0]=='v' && line[1]=='t') {
            if (m->uv_count < MAX_OBJ_UVS) {
                sscanf(line+3, "%f %f",
                       &m->uvs[m->uv_count][0],
                       &m->uvs[m->uv_count][1]);
                m->uv_count++;
            }
        } else if (line[0]=='f' && line[1]==' ') {
            if (m->face_count < MAX_OBJ_FACES) {
                ObjFace *face = &m->faces[m->face_count];
                memset(face, 0, sizeof(*face));
                char *p = line + 2;
                int idx = 0;
                while (*p && idx < 4) {
                    int vi=0, ti=0, ni=0;
                    if (sscanf(p, "%d/%d/%d", &vi, &ti, &ni) == 3) {
                        /* v/vt/vn */
                    } else if (sscanf(p, "%d//%d", &vi, &ni) == 2) {
                        ti = 0;
                    } else if (sscanf(p, "%d/%d", &vi, &ti) == 2) {
                        ni = 0;
                    } else {
                        sscanf(p, "%d", &vi);
                    }
                    face->vi[idx] = vi - 1;
                    face->ti[idx] = ti - 1;
                    face->ni[idx] = ni - 1;
                    idx++;
                    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
                    while (*p == ' ' || *p == '\t') p++;
                }
                face->count = idx;
                m->face_count++;
            }
        }
    }
    fclose(f);
    printf("Loaded OBJ: %s (%d verts, %d faces)\n", path, m->vert_count, m->face_count);
    return 1;
}

static void obj_build_display_list(ObjModel *m) {
    m->display_list = glGenLists(1);
    glNewList(m->display_list, GL_COMPILE);
    for (int i = 0; i < m->face_count; i++) {
        ObjFace *face = &m->faces[i];
        if (face->count == 3) glBegin(GL_TRIANGLES);
        else                  glBegin(GL_QUADS);
        for (int j = 0; j < face->count; j++) {
            if (face->ni[j] >= 0 && face->ni[j] < m->norm_count)
                glNormal3fv(m->norms[face->ni[j]]);
            if (face->ti[j] >= 0 && face->ti[j] < m->uv_count)
                glTexCoord2fv(m->uvs[face->ti[j]]);
            if (face->vi[j] >= 0 && face->vi[j] < m->vert_count)
                glVertex3fv(m->verts[face->vi[j]]);
        }
        glEnd();
    }
    glEndList();
}

static void obj_draw(const ObjModel *m) {
    glCallList(m->display_list);
}

/* ─────────────────── Global Models (loaded from files) ─────────────────── */

static ObjModel model_cube;
static ObjModel model_enemy;
static ObjModel model_barrel;

static int load_all_models(const char *asset_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/cube.obj", asset_dir);
    if (!obj_load(&model_cube, path)) return 0;
    snprintf(path, sizeof(path), "%s/sphere.obj", asset_dir);
    if (!obj_load(&model_enemy, path)) return 0;
    snprintf(path, sizeof(path), "%s/barrel.obj", asset_dir);
    if (!obj_load(&model_barrel, path)) return 0;
    return 1;
}

static void build_all_display_lists(void) {
    obj_build_display_list(&model_cube);
    obj_build_display_list(&model_enemy);
    obj_build_display_list(&model_barrel);
}

/* ──────────────────────────────── Level ────────────────────────────────── */

/*  Map key:
 *  '#' = wall      '.' = floor (empty)
 *  'E' = enemy     'P' = player spawn
 *  'B' = barrel    'C' = crate
 */
static const char *MAP_ROWS[] = {
    "####################",
    "#P.................#",
    "#..................#",
    "#...B####.....B....#",
    "#....#..#..........#",
    "#....#..#....C.....#",
    "#....####..........#",
    "#.......E..........#",
    "#..B...............#",
    "#.....#####........#",
    "#.....#...#....B...#",
    "#.....#.E.#........#",
    "#.....#...#........#",
    "#.....#####........#",
    "#..C...............#",
    "#...E.........C....#",
    "#..................#",
    "#..######....B.....#",
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

static GLuint tex_wall, tex_floor, tex_ceil, tex_enemy, tex_crate, tex_barrel;

static void make_texture(GLuint *out, int w, int h,
                         void (*fill)(unsigned char*,int,int,int,int))
{
    unsigned char *px = (unsigned char*)malloc(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) fill(px,x,y,w,h);
    glGenTextures(1,out);
    glBindTexture(GL_TEXTURE_2D,*out);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,px);
    free(px);
}

static void fill_wall(unsigned char *px, int x, int y, int w, int h) {
    int bw=w/4, bh=h/4;
    int row=y/bh;
    int ox=(row%2)*(bw/2);
    int mx=(x+ox)%bw, my=y%bh;
    int mortar = (mx<2)||(my<2);
    int idx = 3*(y*w+x);
    if(mortar){ px[idx]=160; px[idx+1]=150; px[idx+2]=140; }
    else       { int v=180+(rand()%20-10);
                 px[idx]=(unsigned char)v;
                 px[idx+1]=(unsigned char)(v-30);
                 px[idx+2]=(unsigned char)(v-50); }
    (void)h;
}

static void fill_floor(unsigned char *px, int x, int y, int w, int h) {
    int tile=((x/(w/8))+(y/(h/8)))%2;
    int v = tile ? 90 : 70;
    int idx = 3*(y*w+x);
    px[idx]=(unsigned char)v; px[idx+1]=(unsigned char)(v+5); px[idx+2]=(unsigned char)v;
    (void)h;
}

static void fill_ceil(unsigned char *px, int x, int y, int w, int h) {
    int v=50+((x*3+y*5)%20);
    int idx = 3*(y*w+x);
    px[idx]=(unsigned char)v; px[idx+1]=(unsigned char)v; px[idx+2]=(unsigned char)(v+10);
    (void)h;
}

static void fill_enemy_tex(unsigned char *px, int x, int y, int w, int h) {
    float cx=x/(float)w, cy=y/(float)h;
    int idx = 3*(y*w+x);
    px[idx]=160; px[idx+1]=30; px[idx+2]=30;
    if(((cx>0.2f&&cx<0.35f)||(cx>0.65f&&cx<0.8f))&&cy>0.3f&&cy<0.5f){
        px[idx]=255; px[idx+1]=220; px[idx+2]=0;
    }
    if(cx>0.2f&&cx<0.8f&&cy>0.65f&&cy<0.75f){
        px[idx]=20; px[idx+1]=20; px[idx+2]=20;
    }
    if(((cx>0.1f&&cx<0.25f)||(cx>0.75f&&cx<0.9f))&&cy>0.05f&&cy<0.25f){
        px[idx]=100; px[idx+1]=20; px[idx+2]=20;
    }
    (void)h;
}

static void fill_crate(unsigned char *px, int x, int y, int w, int h) {
    int bdr = (x<3||x>=w-3||y<3||y>=h-3);
    int cross = (abs(x-w/2)<3)||(abs(y-h/2)<3);
    int idx = 3*(y*w+x);
    if(bdr||cross) { px[idx]=100; px[idx+1]=70; px[idx+2]=30; }
    else           { int v=160+(rand()%20-10);
                     px[idx]=(unsigned char)v;
                     px[idx+1]=(unsigned char)(v-20);
                     px[idx+2]=(unsigned char)(v-60); }
    (void)h;
}

static void fill_barrel_tex(unsigned char *px, int x, int y, int w, int h) {
    int stripe = ((y/(h/6))%2==0);
    int idx = 3*(y*w+x);
    if(stripe) { px[idx]=80; px[idx+1]=100; px[idx+2]=80; }
    else       { px[idx]=60; px[idx+1]=80;  px[idx+2]=60; }
    int band = (y%(h/6) < 2);
    if(band) { px[idx]=120; px[idx+1]=120; px[idx+2]=130; }
    (void)w;
}

static void init_textures(void) {
    srand(42);
    make_texture(&tex_wall,   64,64,fill_wall);
    make_texture(&tex_floor,  64,64,fill_floor);
    make_texture(&tex_ceil,   64,64,fill_ceil);
    make_texture(&tex_enemy,  64,64,fill_enemy_tex);
    make_texture(&tex_crate,  64,64,fill_crate);
    make_texture(&tex_barrel, 64,64,fill_barrel_tex);
}

/* ──────────────────────────────── Enemy ────────────────────────────────── */

typedef struct {
    Vec3  pos;
    int   health;
    float pain_timer;
    int   alive;
    float bob_phase;
} Enemy;

static Enemy enemies[MAX_ENEMIES];
static int   enemy_count = 0;

static void enemy_spawn(float x, float z) {
    if(enemy_count>=MAX_ENEMIES) return;
    enemies[enemy_count].pos       = v3(x, 0.8f, z);
    enemies[enemy_count].health    = ENEMY_HEALTH;
    enemies[enemy_count].alive     = 1;
    enemies[enemy_count].pain_timer= 0;
    enemies[enemy_count].bob_phase = (float)(rand()%100)/100.0f * 2*PI;
    enemy_count++;
}

/* ──────────────────────────── Decorations ──────────────────────────────── */

typedef enum { DECO_BARREL, DECO_CRATE } DecoType;

typedef struct {
    Vec3     pos;
    DecoType type;
    float    rotation;
} Decoration;

static Decoration decorations[MAX_DECORATIONS];
static int deco_count = 0;

static void deco_spawn(float x, float z, DecoType type) {
    if(deco_count>=MAX_DECORATIONS) return;
    decorations[deco_count].pos = v3(x, FLOOR_Y, z);
    decorations[deco_count].type = type;
    decorations[deco_count].rotation = (float)(rand()%360);
    deco_count++;
}

/* ──────────────────────────────── Player ───────────────────────────────── */

typedef struct {
    Vec3  pos;
    float yaw;
    float pitch;
    float vy;
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

/* ──────────────────────── Lighting State ───────────────────────────────── */

static float light_brightness = 1.0f;
#define LIGHT_MIN 0.1f
#define LIGHT_MAX 2.0f
#define LIGHT_STEP 0.1f

static int light_color_mode = 0;   /* 0=white, 1=red, 2=blue */
static int flashlight_on = 1;
static float ambient_light_pos[4] = {10.0f, 2.5f, 10.0f, 1.0f};

/* Help screen toggle */
static int show_help = 0;

/* ─────────────────────────── Collision helper ──────────────────────────── */

static int collides_map(float x, float z) {
    int col=(int)floorf(x), row=(int)floorf(z);
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++)
        if(map_is_wall(col+dc, row+dr)) {
            float wx=(float)(col+dc), wz=(float)(row+dr);
            if(x>wx-PLAYER_RADIUS && x<wx+1+PLAYER_RADIUS &&
               z>wz-PLAYER_RADIUS && z<wz+1+PLAYER_RADIUS)
                return 1;
        }
    return 0;
}

/* ──────────────────────────── Init from map ────────────────────────────── */

static void init_level(void) {
    enemy_count=0;
    deco_count=0;
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
            } else if(ch=='B') {
                deco_spawn(c+0.5f, r+0.5f, DECO_BARREL);
            } else if(ch=='C') {
                deco_spawn(c+0.5f, r+0.5f, DECO_CRATE);
            }
        }
    }
}

/* ──────────────────────────── Game update ──────────────────────────────── */

static int game_over = 0;
static int game_win  = 0;
static float game_time = 0;

static void shoot(void) {
    if(player.shoot_cd>0||player.ammo<=0) return;
    player.ammo--;
    player.shoot_cd   = SHOOT_COOLDOWN;
    player.muzzle_flash = MUZZLE_TIME;

    Vec3 fwd = player_forward();
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 d = v3sub(enemies[i].pos, player.pos);
        float along = v3dot(d, fwd);
        if(along<0.5f) continue;
        Vec3 closest = v3sub(d, v3scale(fwd, along));
        if(v3len(closest)<0.55f) {
            enemies[i].health--;
            enemies[i].pain_timer = 0.2f;
            if(enemies[i].health<=0) {
                enemies[i].alive=0;
                int alive=0;
                for(int j=0;j<enemy_count;j++) alive+=enemies[j].alive;
                if(!alive) game_win=1;
            }
        }
    }
}

static void update(float dt) {
    if(game_over||game_win) return;
    game_time += dt;

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

    if(!player.on_ground) player.vy-=GRAVITY*dt;
    player.pos.y+=player.vy*dt;
    if(player.pos.y<=PLAYER_HEIGHT) {
        player.pos.y=PLAYER_HEIGHT;
        player.vy=0;
        player.on_ground=1;
    }

    if(keys[SDL_SCANCODE_SPACE]&&player.on_ground){
        player.vy=JUMP_VEL;
        player.on_ground=0;
    }

    if(player.shoot_cd>0) player.shoot_cd-=dt;
    if(player.muzzle_flash>0) player.muzzle_flash-=dt;

    /* Move ambient light with arrow keys */
    float lms = 5.0f * dt;
    if(keys[SDL_SCANCODE_UP])    ambient_light_pos[2] += lms;
    if(keys[SDL_SCANCODE_DOWN])  ambient_light_pos[2] -= lms;
    if(keys[SDL_SCANCODE_LEFT])  ambient_light_pos[0] -= lms;
    if(keys[SDL_SCANCODE_RIGHT]) ambient_light_pos[0] += lms;

    /* Enemies */
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        if(enemies[i].pain_timer>0) enemies[i].pain_timer-=dt;
        enemies[i].bob_phase += dt * 3.0f;

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

        if(dist<0.8f) {
            player.health-=(int)(20*dt);
            if(player.health<=0) { player.health=0; game_over=1; }
        }
    }

    /* Animate decorations */
    for(int i=0;i<deco_count;i++) {
        decorations[i].rotation += 15.0f * dt;
        if(decorations[i].rotation > 360.0f) decorations[i].rotation -= 360.0f;
    }
}

/* ─────────────────────────── Lighting Setup ────────────────────────────── */

static void setup_lighting(void) {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    float r = 1.0f, g = 1.0f, b = 1.0f;
    switch(light_color_mode) {
        case 1: r=1.0f; g=0.3f; b=0.2f; break;
        case 2: r=0.3f; g=0.4f; b=1.0f; break;
        default: break;
    }
    r *= light_brightness;
    g *= light_brightness;
    b *= light_brightness;

    /* GL_LIGHT0: main scene light (positional, movable with arrows) */
    glEnable(GL_LIGHT0);
    float diff0[] = {r*0.8f, g*0.8f, b*0.8f, 1.0f};
    float amb0[]  = {r*0.15f, g*0.15f, b*0.15f, 1.0f};
    float spec0[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, ambient_light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.3f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

    /* GL_LIGHT1: flashlight (spotlight attached to player camera) */
    if(flashlight_on) {
        glEnable(GL_LIGHT1);
        Vec3 fwd = player_forward();
        float flash_pos[] = {player.pos.x, player.pos.y, player.pos.z, 1.0f};
        float flash_dir[] = {fwd.x, fwd.y, fwd.z};
        float flash_diff[] = {0.9f*light_brightness, 0.85f*light_brightness, 0.7f*light_brightness, 1.0f};
        float flash_amb[]  = {0.0f, 0.0f, 0.0f, 1.0f};
        glLightfv(GL_LIGHT1, GL_POSITION,       flash_pos);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, flash_dir);
        glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,    25.0f);
        glLightf (GL_LIGHT1, GL_SPOT_EXPONENT,  30.0f);
        glLightfv(GL_LIGHT1, GL_DIFFUSE,        flash_diff);
        glLightfv(GL_LIGHT1, GL_AMBIENT,        flash_amb);
        glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  0.5f);
        glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.05f);
        glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);
    } else {
        glDisable(GL_LIGHT1);
    }

    float global_amb[] = {0.08f, 0.08f, 0.10f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_amb);
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

static void quad_n(float x0,float y0,float z0,
                   float x1,float y1,float z1,
                   float x2,float y2,float z2,
                   float x3,float y3,float z3,
                   float nx,float ny,float nz,
                   float us, float vs)
{
    glNormal3f(nx,ny,nz);
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
                glBindTexture(GL_TEXTURE_2D, tex_floor);
                glColor3f(0.9f,0.9f,0.9f);
                quad_n(x0,FLOOR_Y,z0, x1,FLOOR_Y,z0,
                       x1,FLOOR_Y,z1, x0,FLOOR_Y,z1,
                       0,1,0, 1,1);

                glBindTexture(GL_TEXTURE_2D, tex_ceil);
                glColor3f(0.6f,0.6f,0.7f);
                quad_n(x0,CEIL_Y,z1, x1,CEIL_Y,z1,
                       x1,CEIL_Y,z0, x0,CEIL_Y,z0,
                       0,-1,0, 1,1);
            }

            if(map_is_wall(c,r)) {
                glBindTexture(GL_TEXTURE_2D, tex_wall);
                glColor3f(1,1,1);

                if(!map_is_wall(c,r-1))
                    quad_n(x0,CEIL_Y,z0, x1,CEIL_Y,z0,
                           x1,FLOOR_Y,z0, x0,FLOOR_Y,z0,
                           0,0,-1, 1,1);
                if(!map_is_wall(c,r+1))
                    quad_n(x1,CEIL_Y,z1, x0,CEIL_Y,z1,
                           x0,FLOOR_Y,z1, x1,FLOOR_Y,z1,
                           0,0,1, 1,1);
                if(!map_is_wall(c-1,r))
                    quad_n(x0,CEIL_Y,z1, x0,CEIL_Y,z0,
                           x0,FLOOR_Y,z0, x0,FLOOR_Y,z1,
                           -1,0,0, 1,1);
                if(!map_is_wall(c+1,r))
                    quad_n(x1,CEIL_Y,z0, x1,CEIL_Y,z1,
                           x1,FLOOR_Y,z1, x1,FLOOR_Y,z0,
                           1,0,0, 1,1);
            }
        }
    }
    glDisable(GL_TEXTURE_2D);
}

static void draw_enemies(void) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_enemy);

    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 p=enemies[i].pos;
        float bob = sinf(enemies[i].bob_phase) * 0.1f;

        if(enemies[i].pain_timer>0) glColor3f(1,0.3f,0.3f);
        else                         glColor3f(1,1,1);

        glPushMatrix();
        glTranslatef(p.x, p.y + bob, p.z);
        float dx = player.pos.x - p.x;
        float dz = player.pos.z - p.z;
        float angle = atan2f(dx, dz) * 180.0f / PI;
        glRotatef(angle, 0, 1, 0);
        glScalef(0.6f, 0.8f, 0.6f);
        obj_draw(&model_enemy);
        glPopMatrix();
    }
    glColor3f(1,1,1);
    glDisable(GL_TEXTURE_2D);
}

static void draw_decorations(void) {
    glEnable(GL_TEXTURE_2D);
    for(int i=0;i<deco_count;i++) {
        Vec3 p = decorations[i].pos;
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glRotatef(decorations[i].rotation, 0, 1, 0);

        if(decorations[i].type == DECO_BARREL) {
            glBindTexture(GL_TEXTURE_2D, tex_barrel);
            glColor3f(0.8f, 0.9f, 0.8f);
            glScalef(0.5f, 0.8f, 0.5f);
            obj_draw(&model_barrel);
        } else {
            glBindTexture(GL_TEXTURE_2D, tex_crate);
            glColor3f(0.9f, 0.8f, 0.6f);
            glScalef(0.7f, 0.7f, 0.7f);
            obj_draw(&model_cube);
        }
        glPopMatrix();
    }
    glColor3f(1,1,1);
    glDisable(GL_TEXTURE_2D);
}

/* Floating diamond showing the movable light position */
static void draw_light_marker(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    float pulse = 0.7f + 0.3f * sinf(game_time * 4.0f);
    switch(light_color_mode) {
        case 1:  glColor3f(1.0f*pulse, 0.3f*pulse, 0.1f*pulse); break;
        case 2:  glColor3f(0.2f*pulse, 0.3f*pulse, 1.0f*pulse); break;
        default: glColor3f(1.0f*pulse, 1.0f*pulse, 0.8f*pulse); break;
    }

    glPushMatrix();
    glTranslatef(ambient_light_pos[0], ambient_light_pos[1], ambient_light_pos[2]);
    float s = 0.15f;
    glBegin(GL_TRIANGLES);
    glVertex3f(0, s*2, 0); glVertex3f(-s,0,-s); glVertex3f( s,0,-s);
    glVertex3f(0, s*2, 0); glVertex3f( s,0,-s); glVertex3f( s,0, s);
    glVertex3f(0, s*2, 0); glVertex3f( s,0, s); glVertex3f(-s,0, s);
    glVertex3f(0, s*2, 0); glVertex3f(-s,0, s); glVertex3f(-s,0,-s);
    glVertex3f(0,-s*2, 0); glVertex3f( s,0,-s); glVertex3f(-s,0,-s);
    glVertex3f(0,-s*2, 0); glVertex3f( s,0, s); glVertex3f( s,0,-s);
    glVertex3f(0,-s*2, 0); glVertex3f(-s,0, s); glVertex3f( s,0, s);
    glVertex3f(0,-s*2, 0); glVertex3f(-s,0,-s); glVertex3f(-s,0, s);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* ────────────────────────────────── HUD ────────────────────────────────── */

static void ortho_begin(void) {
    glDisable(GL_LIGHTING);
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
    glEnable(GL_LIGHTING);
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

/* ─────────── Bitmap font: 8x8 pixel glyphs drawn as tiny quads ──────── */

static const unsigned long long FONT_GLYPHS[128] = {
    [' ']  = 0x0000000000000000ULL,
    ['!']  = 0x1818181818001800ULL,
    ['+']  = 0x0018187E18180000ULL,
    ['-']  = 0x000000FE00000000ULL,
    ['.']  = 0x0000000000181800ULL,
    ['/']  = 0x060C183060C08000ULL,
    ['%']  = 0xC6CC183066C60000ULL,
    ['0']  = 0x7CC6CEDEF6E67C00ULL,
    ['1']  = 0x1838181818187E00ULL,
    ['2']  = 0x7CC6060C3060FE00ULL,
    ['3']  = 0x7CC6061C06C67C00ULL,
    ['4']  = 0x0C1C3C6CFE0C0C00ULL,
    ['5']  = 0xFEC0FC0606C67C00ULL,
    ['6']  = 0x3C60C0FCC6C67C00ULL,
    ['7']  = 0xFE06060C18181800ULL,
    ['8']  = 0x7CC6C67CC6C67C00ULL,
    ['9']  = 0x7CC6C67E06067C00ULL,
    [':']  = 0x0018180018180000ULL,
    ['(']  = 0x0C18303030180C00ULL,
    [')']  = 0x30180C0C0C183000ULL,
    ['A']  = 0x386CC6FEC6C6C600ULL,
    ['B']  = 0xFC66667C6666FC00ULL,
    ['C']  = 0x3C66C0C0C0663C00ULL,
    ['D']  = 0xF86C6666666CF800ULL,
    ['E']  = 0xFE6268786862FE00ULL,
    ['F']  = 0xFE6268786860F000ULL,
    ['G']  = 0x3C66C0C0CE663E00ULL,
    ['H']  = 0xC6C6C6FEC6C6C600ULL,
    ['I']  = 0x3C18181818183C00ULL,
    ['J']  = 0x1E0C0C0CCCCC7800ULL,
    ['K']  = 0xC6CCD8F0D8CCC600ULL,
    ['L']  = 0xF06060606266FE00ULL,
    ['M']  = 0xC6EEFEFED6C6C600ULL,
    ['N']  = 0xC6E6F6DECEC6C600ULL,
    ['O']  = 0x7CC6C6C6C6C67C00ULL,
    ['P']  = 0xFC66667C6060F000ULL,
    ['Q']  = 0x7CC6C6C6D6DE7C06ULL,
    ['R']  = 0xFC66667C6C66F200ULL,
    ['S']  = 0x7CC6C07C06C67C00ULL,
    ['T']  = 0x7E5A181818183C00ULL,
    ['U']  = 0xC6C6C6C6C6C67C00ULL,
    ['V']  = 0xC6C6C6C66C381000ULL,
    ['W']  = 0xC6C6D6FEEEC6C600ULL,
    ['X']  = 0xC66C38386CC6C600ULL,
    ['Y']  = 0x6666663C18183C00ULL,
    ['Z']  = 0xFE860C183062FE00ULL,
    ['a']  = 0x0000780C7CCC7600ULL,
    ['b']  = 0xE060607C6666DC00ULL,
    ['c']  = 0x00007CC6C0C67C00ULL,
    ['d']  = 0x1C0C0C7CCCCC7600ULL,
    ['e']  = 0x00007CC6FEC07C00ULL,
    ['f']  = 0x1C3630FC30303000ULL,
    ['g']  = 0x000076CCCC7C0CF8ULL,
    ['h']  = 0xE0606C766666E600ULL,
    ['i']  = 0x1800381818183C00ULL,
    ['j']  = 0x0600060606C67C00ULL,
    ['k']  = 0xE060666C786CE600ULL,
    ['l']  = 0x3818181818183C00ULL,
    ['m']  = 0x0000ECFED6D6C600ULL,
    ['n']  = 0x0000DC6666666600ULL,
    ['o']  = 0x00007CC6C6C67C00ULL,
    ['p']  = 0x0000DC667C60F000ULL,
    ['q']  = 0x000076CC7C0C1E00ULL,
    ['r']  = 0x0000DC7660606000ULL,
    ['s']  = 0x00007CC07C06FC00ULL,
    ['t']  = 0x1030FC3030361C00ULL,
    ['u']  = 0x0000CCCCCCCC7600ULL,
    ['v']  = 0x0000C6C66C381000ULL,
    ['w']  = 0x0000C6D6FEEE4400ULL,
    ['x']  = 0x0000C66C386CC600ULL,
    ['y']  = 0x0000C6C67E060CFCULL,
    ['z']  = 0x0000FC983064FC00ULL,
    [',']  = 0x0000000000181830ULL,
    ['?']  = 0x7CC60C1818001800ULL,
};

static void draw_char(float px, float py, float size, char ch) {
    unsigned char uch = (unsigned char)ch;
    if(uch >= 128) return;
    unsigned long long g = FONT_GLYPHS[uch];
    if(!g && ch!=' ') return;
    for(int row=0; row<8; row++) {
        unsigned char bits = (unsigned char)((g >> (56 - row*8)) & 0xFF);
        for(int col=0; col<8; col++) {
            if(bits & (0x80 >> col)) {
                float x = px + col*size;
                float y = py + (7-row)*size;
                glBegin(GL_QUADS);
                glVertex2f(x,     y);
                glVertex2f(x+size,y);
                glVertex2f(x+size,y+size);
                glVertex2f(x,     y+size);
                glEnd();
            }
        }
    }
}

static void draw_text(float x, float y, float size, const char *text) {
    for(int i=0; text[i]; i++) {
        draw_char(x + i*size*9, y, size, text[i]);
    }
}

static void draw_hud(void) {
    ortho_begin();

    /* Crosshair */
    int cx=WINDOW_W/2, cy=WINDOW_H/2;
    fill_rect(cx-12,cy-2,  10,4,  1,1,1,0.8f);
    fill_rect(cx+2, cy-2,  10,4,  1,1,1,0.8f);
    fill_rect(cx-2, cy-12, 4, 10, 1,1,1,0.8f);
    fill_rect(cx-2, cy+2,  4, 10, 1,1,1,0.8f);

    /* Muzzle flash overlay */
    if(player.muzzle_flash>0) {
        float a=player.muzzle_flash/MUZZLE_TIME;
        fill_rect(0,0,WINDOW_W,WINDOW_H, 1,0.8f,0,0.15f*a);
    }

    /* Health bar */
    fill_rect(20, 20, 204, 24, 0.2f,0.2f,0.2f,0.8f);
    float hp_ratio = player.health/100.0f;
    if(hp_ratio<0) hp_ratio=0;
    float hcr = hp_ratio<0.4f ? 1.0f : 0.2f;
    float hcg = hp_ratio>0.6f ? 0.8f : (hp_ratio*1.5f);
    fill_rect(22, 22, 200*hp_ratio, 20, hcr, hcg, 0.1f, 1.0f);

    /* Ammo bar */
    fill_rect(20, 50, 154, 18, 0.2f,0.2f,0.2f,0.8f);
    fill_rect(22, 52, (player.ammo/50.0f)*150, 14, 0.9f,0.7f,0.1f,1.0f);

    /* Labels */
    glColor4f(1,1,1,0.9f);
    draw_text(22, 26, 2.0f, "HP");
    draw_text(22, 54, 1.5f, "AMMO");

    /* Enemy count */
    int alive=0;
    for(int i=0;i<enemy_count;i++) alive+=enemies[i].alive;
    char buf[64];
    glColor4f(1,0.4f,0.4f,0.9f);
    snprintf(buf, sizeof(buf), "ENEMIES: %d", alive);
    draw_text(20, 80, 2.0f, buf);

    /* Light info */
    glColor4f(1,1,0.6f,0.7f);
    const char *cnames[] = {"WHITE", "RED", "BLUE"};
    snprintf(buf, sizeof(buf), "LIGHT: %.0f%%  %s  %s",
             light_brightness*100, cnames[light_color_mode],
             flashlight_on ? "FLASH:ON" : "FLASH:OFF");
    draw_text(20, WINDOW_H-30, 1.5f, buf);

    /* F1 hint */
    glColor4f(0.7f,0.7f,0.7f,0.5f);
    draw_text(WINDOW_W-200, WINDOW_H-25, 1.5f, "F1: HELP");

    /* Game over / win overlays */
    if(game_over) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-50, 400, 100, 0.8f,0.05f,0.05f,0.85f);
        glColor4f(1,1,1,1);
        draw_text(WINDOW_W/2-100, WINDOW_H/2-10, 3.0f, "YOU DIED");
        glColor4f(0.8f,0.8f,0.8f,0.8f);
        draw_text(WINDOW_W/2-120, WINDOW_H/2-35, 1.5f, "PRESS R TO RESTART");
    }
    if(game_win) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-50, 400, 100, 0.05f,0.6f,0.05f,0.85f);
        glColor4f(1,1,1,1);
        draw_text(WINDOW_W/2-100, WINDOW_H/2-10, 3.0f, "YOU WIN!");
        glColor4f(0.8f,0.8f,0.8f,0.8f);
        draw_text(WINDOW_W/2-120, WINDOW_H/2-35, 1.5f, "PRESS R TO RESTART");
    }

    ortho_end();
}

/* ──────────────────────────── Help Screen ──────────────────────────────── */

static void draw_help_screen(void) {
    ortho_begin();

    fill_rect(0, 0, WINDOW_W, WINDOW_H, 0, 0, 0, 0.75f);

    float px = 200, py = 80, pw = WINDOW_W-400, ph = WINDOW_H-160;
    fill_rect(px, py, pw, ph, 0.1f, 0.1f, 0.15f, 0.95f);
    fill_rect(px+2, py+2, pw-4, ph-4, 0.15f, 0.15f, 0.2f, 0.95f);

    float tx = px + 30;
    float ty = py + ph - 50;
    float lh = 28;
    float sz = 2.0f;

    glColor4f(1.0f, 0.4f, 0.3f, 1.0f);
    draw_text(tx, ty, 3.0f, "DOOM3D - HELP");
    ty -= lh * 1.5f;

    glColor4f(1.0f, 0.9f, 0.5f, 1.0f);
    draw_text(tx, ty, sz, "DESCRIPTION:");
    ty -= lh;
    glColor4f(0.9f, 0.9f, 0.9f, 0.9f);
    draw_text(tx, ty, sz, "A DOOM STYLE 3D FPS. KILL ALL ENEMIES");
    ty -= lh;
    draw_text(tx, ty, sz, "TO WIN. BUILT WITH SDL2 AND OPENGL.");
    ty -= lh;
    draw_text(tx, ty, sz, "3D MODELS LOADED FROM OBJ FILES.");
    ty -= lh * 1.5f;

    glColor4f(1.0f, 0.9f, 0.5f, 1.0f);
    draw_text(tx, ty, sz, "MOVEMENT:");
    ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "W S A D   - MOVE AND STRAFE");
    ty -= lh;
    draw_text(tx, ty, sz, "MOUSE     - LOOK AROUND");
    ty -= lh;
    draw_text(tx, ty, sz, "SPACE     - JUMP");
    ty -= lh;
    draw_text(tx, ty, sz, "L. CLICK  - SHOOT");
    ty -= lh * 1.5f;

    glColor4f(1.0f, 0.9f, 0.5f, 1.0f);
    draw_text(tx, ty, sz, "LIGHTING:");
    ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "+/-       - BRIGHTNESS UP/DOWN");
    ty -= lh;
    draw_text(tx, ty, sz, "L         - CYCLE LIGHT COLOR");
    ty -= lh;
    draw_text(tx, ty, sz, "F         - TOGGLE FLASHLIGHT");
    ty -= lh;
    draw_text(tx, ty, sz, "ARROWS    - MOVE SCENE LIGHT");
    ty -= lh * 1.5f;

    glColor4f(1.0f, 0.9f, 0.5f, 1.0f);
    draw_text(tx, ty, sz, "OTHER:");
    ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "F1        - TOGGLE THIS HELP");
    ty -= lh;
    draw_text(tx, ty, sz, "R         - RESTART (WHEN DEAD)");
    ty -= lh;
    draw_text(tx, ty, sz, "ESC       - QUIT");

    ortho_end();
}

/* ───────────────────────────────── Main ────────────────────────────────── */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    char asset_dir[512] = "assets";

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

    SDL_GL_SetSwapInterval(1);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.02f,0.02f,0.05f,1);
    glShadeModel(GL_SMOOTH);

    if(!load_all_models(asset_dir)) {
        fprintf(stderr,"Failed to load models from '%s/'\n", asset_dir);
        fprintf(stderr,"Make sure cube.obj, sphere.obj, barrel.obj are in the assets/ folder.\n");
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    init_textures();
    build_all_display_lists();
    init_level();
    set_projection();

    Uint32 prev = SDL_GetTicks();
    int running=1;

    while(running) {
        Uint32 now=SDL_GetTicks();
        float dt=(now-prev)/1000.0f;
        if(dt>0.05f) dt=0.05f;
        prev=now;

        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT) running=0;
            if(ev.type==SDL_KEYDOWN) {
                switch(ev.key.keysym.sym) {
                    case SDLK_ESCAPE: running=0; break;
                    case SDLK_F1:     show_help = !show_help; break;
                    case SDLK_f:      flashlight_on = !flashlight_on; break;
                    case SDLK_l:
                        light_color_mode = (light_color_mode + 1) % 3;
                        break;
                    case SDLK_EQUALS:
                    case SDLK_PLUS:
                    case SDLK_KP_PLUS:
                        light_brightness += LIGHT_STEP;
                        if(light_brightness > LIGHT_MAX) light_brightness = LIGHT_MAX;
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        light_brightness -= LIGHT_STEP;
                        if(light_brightness < LIGHT_MIN) light_brightness = LIGHT_MIN;
                        break;
                    case SDLK_r:
                        if(game_over||game_win) {
                            game_over=0; game_win=0;
                            init_level();
                        }
                        break;
                    default: break;
                }
            }
            if(ev.type==SDL_MOUSEMOTION && !game_over && !game_win && !show_help) {
                player.yaw   -= ev.motion.xrel * TURN_SPEED;
                player.pitch -= ev.motion.yrel * TURN_SPEED;
                if(player.pitch> 1.4f) player.pitch= 1.4f;
                if(player.pitch<-1.4f) player.pitch=-1.4f;
            }
            if(ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT && !show_help)
                shoot();
        }

        if(!show_help)
            update(dt);

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        set_camera();
        setup_lighting();
        draw_level();
        draw_decorations();
        draw_enemies();
        draw_light_marker();
        draw_hud();

        if(show_help)
            draw_help_screen();

        SDL_GL_SwapWindow(win);
    }

    glDeleteLists(model_cube.display_list, 1);
    glDeleteLists(model_enemy.display_list, 1);
    glDeleteLists(model_barrel.display_list, 1);
    glDeleteTextures(1,&tex_wall);
    glDeleteTextures(1,&tex_floor);
    glDeleteTextures(1,&tex_ceil);
    glDeleteTextures(1,&tex_enemy);
    glDeleteTextures(1,&tex_crate);
    glDeleteTextures(1,&tex_barrel);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}