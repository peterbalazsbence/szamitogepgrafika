/*
 * DOOM-LIKE 3D FPS GAME
 * Built with C, SDL2, and OpenGL
 *
 * Controls:
 *   W/S         - Move forward/backward
 *   A/D         - Strafe left/right
 *   Mouse       - Look around (yaw + pitch)
 *   Left Click  - Shoot
 *   1/2/3       - Switch weapon (Fist / Pistol / Shotgun)
 *   Space       - Jump
 *   +/-         - Increase/decrease light brightness
 *   L           - Cycle light color (white/red/blue)
 *   F           - Toggle flashlight (player-attached spotlight)
 *   [ / ]       - Decrease/increase fog density
 *   G           - Toggle fog on/off
 *   Arrow Keys  - Move the scene light
 *   F1          - Toggle help screen
 *   R           - Restart (when dead or won)
 *   ESC         - Quit
 *
 * Additional features:
 *   - Fog: Distance-based OpenGL fog, adjustable density
 *   - Particle system: Fire/smoke on barrels, blood splatter on enemy hits
 *   - Transparency: Glass windows in walls ('W' tile)
 *   - Shadows: Projected shadow blobs under enemies and decorations
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

#define MAX_ENEMIES      32
#define ENEMY_SPEED      2.5f
#define ENEMY_HEALTH     3
#define ENEMY_SIGHT_RANGE 18.0f
#define ENEMY_ATTACK_RANGE 1.0f
#define ENEMY_PATROL_RANGE 8.0f

#define MAX_DECORATIONS  64
#define MAX_PICKUPS      64

#define SHOOT_COOLDOWN_FIST   0.4f
#define SHOOT_COOLDOWN_PISTOL 0.3f
#define SHOOT_COOLDOWN_SHOTGUN 0.7f
#define MUZZLE_TIME          0.08f

#define FIST_DAMAGE     2
#define PISTOL_DAMAGE   1
#define SHOTGUN_DAMAGE  3
#define FIST_RANGE      2.0f
#define PISTOL_RANGE    50.0f
#define SHOTGUN_RANGE   15.0f
#define SHOTGUN_SPREAD  0.8f

/* Particle system */
#define MAX_PARTICLES    512

/* Fog */
#define FOG_DENSITY_MIN  0.0f
#define FOG_DENSITY_MAX  0.30f
#define FOG_DENSITY_STEP 0.01f

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

static float randf(void) { return (float)rand()/(float)RAND_MAX; }
static float randf_range(float lo, float hi) { return lo + randf()*(hi-lo); }

/* ──────────────────────── OBJ Model Loader ─────────────────────────────── */

#define MAX_OBJ_VERTS   4096
#define MAX_OBJ_NORMS   4096
#define MAX_OBJ_UVS     4096
#define MAX_OBJ_FACES   4096

typedef struct { int vi[4], ti[4], ni[4]; int count; } ObjFace;

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
    if (!f) { fprintf(stderr, "Cannot open OBJ: %s\n", path); return 0; }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0]=='v' && line[1]==' ') {
            if (m->vert_count < MAX_OBJ_VERTS) {
                sscanf(line+2, "%f %f %f", &m->verts[m->vert_count][0],
                       &m->verts[m->vert_count][1], &m->verts[m->vert_count][2]);
                m->vert_count++;
            }
        } else if (line[0]=='v' && line[1]=='n') {
            if (m->norm_count < MAX_OBJ_NORMS) {
                sscanf(line+3, "%f %f %f", &m->norms[m->norm_count][0],
                       &m->norms[m->norm_count][1], &m->norms[m->norm_count][2]);
                m->norm_count++;
            }
        } else if (line[0]=='v' && line[1]=='t') {
            if (m->uv_count < MAX_OBJ_UVS) {
                sscanf(line+3, "%f %f", &m->uvs[m->uv_count][0], &m->uvs[m->uv_count][1]);
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
                    if (sscanf(p, "%d/%d/%d", &vi, &ti, &ni) == 3) { }
                    else if (sscanf(p, "%d//%d", &vi, &ni) == 2) { ti = 0; }
                    else if (sscanf(p, "%d/%d", &vi, &ti) == 2) { ni = 0; }
                    else { sscanf(p, "%d", &vi); }
                    face->vi[idx]=vi-1; face->ti[idx]=ti-1; face->ni[idx]=ni-1;
                    idx++;
                    while (*p && *p!=' ' && *p!='\t' && *p!='\n' && *p!='\r') p++;
                    while (*p==' ' || *p=='\t') p++;
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
        glBegin(face->count == 3 ? GL_TRIANGLES : GL_QUADS);
        for (int j = 0; j < face->count; j++) {
            if (face->ni[j]>=0 && face->ni[j]<m->norm_count) glNormal3fv(m->norms[face->ni[j]]);
            if (face->ti[j]>=0 && face->ti[j]<m->uv_count)   glTexCoord2fv(m->uvs[face->ti[j]]);
            if (face->vi[j]>=0 && face->vi[j]<m->vert_count)  glVertex3fv(m->verts[face->vi[j]]);
        }
        glEnd();
    }
    glEndList();
}

static void obj_draw(const ObjModel *m) { glCallList(m->display_list); }

/* ──────────────────── Global Models ────────────────────────────────────── */

static ObjModel model_cube, model_enemy, model_barrel;

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

/* 40x40 map:
 *  '#' = wall      '.' = floor       'E' = enemy spawn
 *  'P' = player    'B' = barrel      'C' = crate
 *  'W' = glass window (transparent wall, blocks movement)
 */
static const char *MAP_ROWS[] = {
    "########################################",
    "#P.........#.........#.................#",
    "#..........#.........#.................#",
    "#..........#....E....W......####.......#",
    "#...B......#.........W......#..#.......#",
    "#..........###..######......#..#.......#",
    "#..........#.........#......####.......#",
    "#..........W.........#.........E.......#",
    "#..E.......W.........#.................#",
    "####.###...#....B....#..........####...#",
    "#..........#.........#..........#..#...#",
    "#.....................B.........#..#...#",
    "#..........#.........#..........####...#",
    "#..B.......#..E......#.................#",
    "#..........###.#######.................#",
    "#..........W.................####......#",
    "####..######.................#..#......#",
    "#..........#.........E.......#..#......#",
    "#..........#.................####......#",
    "#..E.......#...........................#",
    "#..........#####W####..................#",
    "#..........#.........#.....E...........#",
    "#..........#....B....#.................#",
    "#..........W.........#.........####....#",
    "####.###...#.........W.........#..#....#",
    "#..........#.........W.........#..#....#",
    "#..........###..######.........####....#",
    "#.....C....#.........#.................#",
    "#..........#.........#.................#",
    "#..E.......#....E....W...E.............#",
    "#..........#.........#.................#",
    "####.###...#.........####.######.......#",
    "#..........#.........................B.#",
    "#..........#..C......#.................#",
    "#..B.......#.........W.......E.........#",
    "#..........###########.................#",
    "#..............C.......................#",
    "#...E..................................#",
    "#......................................#",
    "########################################",
};
#define MAP_ROWS_N  40
#define MAP_COLS    40

static int map_is_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    char ch = MAP_ROWS[row][col];
    return ch=='#' || ch=='W'; /* windows block movement too */
}

static int map_is_solid_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    return MAP_ROWS[row][col]=='#';
}

static int map_is_window(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 0;
    return MAP_ROWS[row][col]=='W';
}

/* ──────────────── Line-of-sight (windows don't block) ──────────────────── */

static int has_line_of_sight(float x1, float z1, float x2, float z2) {
    float dx=x2-x1, dz=z2-z1;
    float dist=sqrtf(dx*dx+dz*dz);
    if(dist<0.1f) return 1;
    float step=0.5f;
    int steps=(int)(dist/step)+1;
    for(int i=1;i<steps;i++) {
        float t=(float)i/steps;
        float cx=x1+dx*t, cz=z1+dz*t;
        int col=(int)floorf(cx), row=(int)floorf(cz);
        if(map_is_solid_wall(col,row)) return 0;
        /* Windows are transparent - LOS passes through */
    }
    return 1;
}

/* ──────────────────────────── Texture gen ──────────────────────────────── */

static GLuint tex_wall, tex_floor, tex_ceil, tex_enemy, tex_crate, tex_barrel;
static GLuint tex_pickup_health, tex_pickup_ammo, tex_pickup_armor;
static GLuint tex_window; /* semi-transparent glass */
static GLuint tex_shadow; /* soft shadow blob */

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

/* RGBA texture (for transparency) */
static void make_texture_rgba(GLuint *out, int w, int h,
                              void (*fill)(unsigned char*,int,int,int,int))
{
    unsigned char *px = (unsigned char*)malloc(w*h*4);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) fill(px,x,y,w,h);
    glGenTextures(1,out);
    glBindTexture(GL_TEXTURE_2D,*out);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,px);
    free(px);
}

static void fill_wall(unsigned char *px, int x, int y, int w, int h) {
    int bw=w/4, bh=h/4, row=y/bh, ox=(row%2)*(bw/2);
    int mx=(x+ox)%bw, my=y%bh, mortar=(mx<2)||(my<2);
    int idx=3*(y*w+x);
    if(mortar){px[idx]=160;px[idx+1]=150;px[idx+2]=140;}
    else{int v=180+(rand()%20-10);px[idx]=(unsigned char)v;px[idx+1]=(unsigned char)(v-30);px[idx+2]=(unsigned char)(v-50);}
    (void)h;
}

static void fill_floor(unsigned char *px, int x, int y, int w, int h) {
    int tile=((x/(w/8))+(y/(h/8)))%2, v=tile?90:70, idx=3*(y*w+x);
    px[idx]=(unsigned char)v;px[idx+1]=(unsigned char)(v+5);px[idx+2]=(unsigned char)v;
    (void)h;
}

static void fill_ceil(unsigned char *px, int x, int y, int w, int h) {
    int v=50+((x*3+y*5)%20), idx=3*(y*w+x);
    px[idx]=(unsigned char)v;px[idx+1]=(unsigned char)v;px[idx+2]=(unsigned char)(v+10);
    (void)h;
}

static void fill_enemy_tex(unsigned char *px, int x, int y, int w, int h) {
    float cx=x/(float)w, cy=y/(float)h; int idx=3*(y*w+x);
    px[idx]=160;px[idx+1]=30;px[idx+2]=30;
    if(((cx>0.2f&&cx<0.35f)||(cx>0.65f&&cx<0.8f))&&cy>0.3f&&cy<0.5f){px[idx]=255;px[idx+1]=220;px[idx+2]=0;}
    if(cx>0.2f&&cx<0.8f&&cy>0.65f&&cy<0.75f){px[idx]=20;px[idx+1]=20;px[idx+2]=20;}
    if(((cx>0.1f&&cx<0.25f)||(cx>0.75f&&cx<0.9f))&&cy>0.05f&&cy<0.25f){px[idx]=100;px[idx+1]=20;px[idx+2]=20;}
    (void)h;
}

static void fill_crate(unsigned char *px, int x, int y, int w, int h) {
    int bdr=(x<3||x>=w-3||y<3||y>=h-3),cross=(abs(x-w/2)<3)||(abs(y-h/2)<3),idx=3*(y*w+x);
    if(bdr||cross){px[idx]=100;px[idx+1]=70;px[idx+2]=30;}
    else{int v=160+(rand()%20-10);px[idx]=(unsigned char)v;px[idx+1]=(unsigned char)(v-20);px[idx+2]=(unsigned char)(v-60);}
    (void)h;
}

static void fill_barrel_tex(unsigned char *px, int x, int y, int w, int h) {
    int stripe=((y/(h/6))%2==0),idx=3*(y*w+x);
    if(stripe){px[idx]=80;px[idx+1]=100;px[idx+2]=80;}else{px[idx]=60;px[idx+1]=80;px[idx+2]=60;}
    if(y%(h/6)<2){px[idx]=120;px[idx+1]=120;px[idx+2]=130;}
    (void)w;
}

static void fill_pickup_health(unsigned char *px, int x, int y, int w, int h) {
    int idx=3*(y*w+x); float cx=x/(float)w-0.5f,cy=y/(float)h-0.5f;
    int cross=(fabsf(cx)<0.15f&&fabsf(cy)<0.4f)||(fabsf(cy)<0.15f&&fabsf(cx)<0.4f);
    if(cross){px[idx]=30;px[idx+1]=220;px[idx+2]=30;}else{px[idx]=200;px[idx+1]=200;px[idx+2]=200;}
    (void)h;
}

static void fill_pickup_ammo(unsigned char *px, int x, int y, int w, int h) {
    int idx=3*(y*w+x); float cx=x/(float)w,cy=y/(float)h;
    int bullet=(cy>0.3f&&cy<0.8f)&&((cx>0.15f&&cx<0.3f)||(cx>0.4f&&cx<0.55f)||(cx>0.65f&&cx<0.8f));
    int tip=(cy>0.1f&&cy<0.3f)&&((cx>0.18f&&cx<0.27f)||(cx>0.43f&&cx<0.52f)||(cx>0.68f&&cx<0.77f));
    if(bullet){px[idx]=200;px[idx+1]=170;px[idx+2]=50;}
    else if(tip){px[idx]=220;px[idx+1]=140;px[idx+2]=40;}
    else{px[idx]=60;px[idx+1]=60;px[idx+2]=60;}
    (void)h;
}

static void fill_pickup_armor(unsigned char *px, int x, int y, int w, int h) {
    int idx=3*(y*w+x); float cx=x/(float)w-0.5f,cy=y/(float)h-0.5f;
    float d=cx*cx+cy*cy;
    if(d<0.18f&&cy<0.1f){px[idx]=50;px[idx+1]=80;px[idx+2]=220;}
    else{px[idx]=100;px[idx+1]=100;px[idx+2]=100;}
    (void)h;
}

/* Glass window texture (RGBA - semi-transparent blue-gray) */
static void fill_window(unsigned char *px, int x, int y, int w, int h) {
    int idx=4*(y*w+x);
    /* Frame (opaque dark metal) */
    int frame = (x<3||x>=w-3||y<3||y>=h-3);
    /* Cross bars */
    int bars = (abs(x-w/2)<2)||(abs(y-h/2)<2);
    if(frame||bars) {
        px[idx]=80; px[idx+1]=85; px[idx+2]=90; px[idx+3]=255; /* opaque */
    } else {
        /* Glass pane: semi-transparent blue tint */
        int shimmer = ((x+y)%8<4) ? 10 : 0;
        px[idx]=(unsigned char)(120+shimmer);
        px[idx+1]=(unsigned char)(140+shimmer);
        px[idx+2]=(unsigned char)(180+shimmer);
        px[idx+3]=100; /* semi-transparent */
    }
    (void)h;
}

/* Shadow blob texture (RGBA - soft circular shadow) */
static void fill_shadow(unsigned char *px, int x, int y, int w, int h) {
    int idx=4*(y*w+x);
    float cx=x/(float)w-0.5f, cy=y/(float)h-0.5f;
    float d=sqrtf(cx*cx+cy*cy)*2.0f; /* 0 at center, 1 at edge */
    if(d>1.0f) d=1.0f;
    float alpha = (1.0f - d*d) * 0.6f; /* soft falloff */
    px[idx]=0; px[idx+1]=0; px[idx+2]=0;
    px[idx+3]=(unsigned char)(alpha*255);
    (void)h;
}

static void init_textures(void) {
    srand(42);
    make_texture(&tex_wall,64,64,fill_wall);
    make_texture(&tex_floor,64,64,fill_floor);
    make_texture(&tex_ceil,64,64,fill_ceil);
    make_texture(&tex_enemy,64,64,fill_enemy_tex);
    make_texture(&tex_crate,64,64,fill_crate);
    make_texture(&tex_barrel,64,64,fill_barrel_tex);
    make_texture(&tex_pickup_health,32,32,fill_pickup_health);
    make_texture(&tex_pickup_ammo,32,32,fill_pickup_ammo);
    make_texture(&tex_pickup_armor,32,32,fill_pickup_armor);
    make_texture_rgba(&tex_window,64,64,fill_window);
    make_texture_rgba(&tex_shadow,32,32,fill_shadow);
}

/* ──────────────────── Particle System ──────────────────────────────────── */

typedef enum { PART_FIRE, PART_SMOKE, PART_BLOOD } ParticleType;

typedef struct {
    Vec3 pos;
    Vec3 vel;
    float life;      /* remaining life in seconds */
    float max_life;
    float size;
    float r, g, b, a;
    ParticleType type;
    int active;
} Particle;

static Particle particles[MAX_PARTICLES];
static int particle_count = 0;

static void particle_emit(Vec3 pos, Vec3 vel, float life, float size,
                          float r, float g, float b, float a, ParticleType type)
{
    /* Find an inactive slot, or overwrite oldest */
    int slot = -1;
    for(int i=0;i<particle_count;i++) {
        if(!particles[i].active) { slot=i; break; }
    }
    if(slot<0) {
        if(particle_count<MAX_PARTICLES) { slot=particle_count++; }
        else {
            /* Overwrite the particle with least life remaining */
            float min_life=999; slot=0;
            for(int i=0;i<MAX_PARTICLES;i++) {
                if(particles[i].life<min_life) { min_life=particles[i].life; slot=i; }
            }
        }
    }
    Particle *p = &particles[slot];
    p->pos=pos; p->vel=vel; p->life=life; p->max_life=life;
    p->size=size; p->r=r; p->g=g; p->b=b; p->a=a;
    p->type=type; p->active=1;
}

static void particles_update(float dt) {
    for(int i=0;i<particle_count;i++) {
        Particle *p = &particles[i];
        if(!p->active) continue;
        p->life -= dt;
        if(p->life<=0) { p->active=0; continue; }

        /* Physics */
        p->pos = v3add(p->pos, v3scale(p->vel, dt));

        switch(p->type) {
            case PART_FIRE:
                p->vel.y += 1.5f*dt; /* rise */
                p->a = (p->life/p->max_life)*0.8f;
                /* Shift color from yellow to red as it ages */
                p->r = 1.0f;
                p->g = 0.3f + 0.7f*(p->life/p->max_life);
                p->b = 0.1f*(p->life/p->max_life);
                p->size *= (1.0f + 0.5f*dt); /* expand slightly */
                break;
            case PART_SMOKE:
                p->vel.y += 0.5f*dt;
                p->a = (p->life/p->max_life)*0.4f;
                p->size *= (1.0f + 1.0f*dt);
                break;
            case PART_BLOOD:
                p->vel.y -= 9.8f*dt; /* gravity */
                p->a = (p->life/p->max_life)*0.9f;
                /* Stop at floor */
                if(p->pos.y < FLOOR_Y+0.05f) {
                    p->pos.y = FLOOR_Y+0.05f;
                    p->vel = v3(0,0,0);
                    p->life -= dt*2; /* fade faster on ground */
                }
                break;
        }
    }
}

static void particles_draw(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); /* don't write to depth buffer */

    /* Get camera right/up for billboarding */
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    Vec3 cam_right = v3(mv[0], mv[4], mv[8]);
    Vec3 cam_up    = v3(mv[1], mv[5], mv[9]);

    for(int i=0;i<particle_count;i++) {
        Particle *p = &particles[i];
        if(!p->active) continue;

        float s = p->size * 0.5f;
        Vec3 pos = p->pos;

        /* Use additive blending for fire */
        if(p->type == PART_FIRE) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glColor4f(p->r, p->g, p->b, p->a);
        glBegin(GL_QUADS);
        Vec3 br = v3sub(v3sub(pos, v3scale(cam_right,s)), v3scale(cam_up,s));
        Vec3 bl = v3add(v3sub(pos, v3scale(cam_up,s)), v3scale(cam_right,s));
        Vec3 tl = v3add(v3add(pos, v3scale(cam_right,s)), v3scale(cam_up,s));
        Vec3 tr = v3sub(v3add(pos, v3scale(cam_up,s)), v3scale(cam_right,s));
        glVertex3f(br.x,br.y,br.z);
        glVertex3f(bl.x,bl.y,bl.z);
        glVertex3f(tl.x,tl.y,tl.z);
        glVertex3f(tr.x,tr.y,tr.z);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* Emit fire/smoke for a barrel at given position */
static void emit_barrel_fire(Vec3 pos, float dt) {
    /* Emit a few particles per frame */
    static float accum = 0;
    (void)dt;
    accum += dt;
    if(accum < 0.03f) return;
    accum = 0;

    /* Fire particle */
    Vec3 fire_pos = v3(pos.x + randf_range(-0.15f,0.15f),
                       pos.y + 0.7f,
                       pos.z + randf_range(-0.15f,0.15f));
    Vec3 fire_vel = v3(randf_range(-0.3f,0.3f), randf_range(0.8f,1.8f), randf_range(-0.3f,0.3f));
    particle_emit(fire_pos, fire_vel, randf_range(0.3f,0.7f), randf_range(0.08f,0.15f),
                  1.0f, 0.8f, 0.2f, 0.8f, PART_FIRE);

    /* Occasional smoke */
    if(randf()<0.3f) {
        Vec3 smoke_pos = v3(pos.x+randf_range(-0.1f,0.1f), pos.y+1.0f, pos.z+randf_range(-0.1f,0.1f));
        Vec3 smoke_vel = v3(randf_range(-0.2f,0.2f), randf_range(0.3f,0.8f), randf_range(-0.2f,0.2f));
        particle_emit(smoke_pos, smoke_vel, randf_range(0.8f,1.5f), randf_range(0.1f,0.2f),
                      0.4f, 0.4f, 0.4f, 0.4f, PART_SMOKE);
    }
}

/* Emit blood splatter at enemy position */
static void emit_blood(Vec3 pos) {
    int count = 8 + rand()%8;
    for(int i=0;i<count;i++) {
        Vec3 vel = v3(randf_range(-3.0f,3.0f), randf_range(1.0f,4.0f), randf_range(-3.0f,3.0f));
        float size = randf_range(0.04f,0.10f);
        particle_emit(pos, vel, randf_range(0.5f,1.2f), size,
                      0.8f, 0.05f, 0.05f, 0.9f, PART_BLOOD);
    }
}

/* ──────────────────────── Enemy with AI States ────────────────────────── */

typedef enum { AI_IDLE, AI_PATROL, AI_CHASE, AI_SEARCH, AI_ATTACK } AIState;

typedef struct {
    Vec3 pos, spawn_pos, last_known_player;
    int health; float pain_timer; int alive; float bob_phase;
    AIState state; float state_timer, patrol_angle, attack_cooldown;
} Enemy;

static Enemy enemies[MAX_ENEMIES];
static int enemy_count = 0;

static void enemy_spawn(float x, float z) {
    if(enemy_count>=MAX_ENEMIES) return;
    Enemy *e=&enemies[enemy_count];
    e->pos=v3(x,0.8f,z); e->spawn_pos=e->pos; e->last_known_player=e->pos;
    e->health=ENEMY_HEALTH; e->alive=1; e->pain_timer=0;
    e->bob_phase=randf()*2*PI;
    e->state=AI_IDLE; e->state_timer=0;
    e->patrol_angle=randf()*2*PI; e->attack_cooldown=0;
    enemy_count++;
}

/* ──────────────────────────── Decorations ──────────────────────────────── */

typedef enum { DECO_BARREL, DECO_CRATE } DecoType;
typedef struct { Vec3 pos; DecoType type; float rotation; } Decoration;
static Decoration decorations[MAX_DECORATIONS];
static int deco_count = 0;

static void deco_spawn(float x, float z, DecoType type) {
    if(deco_count>=MAX_DECORATIONS) return;
    decorations[deco_count].pos=v3(x,FLOOR_Y,z);
    decorations[deco_count].type=type;
    decorations[deco_count].rotation=(float)(rand()%360);
    deco_count++;
}

/* ──────────────────────────── Pickups ──────────────────────────────────── */

typedef enum { PICKUP_HEALTH, PICKUP_AMMO, PICKUP_ARMOR } PickupType;
typedef struct { Vec3 pos; PickupType type; int active; float bob_phase; } Pickup;
static Pickup pickups[MAX_PICKUPS];
static int pickup_count = 0;

static void pickup_spawn(float x, float z, PickupType type) {
    if(pickup_count>=MAX_PICKUPS) return;
    pickups[pickup_count].pos=v3(x,FLOOR_Y+0.3f,z);
    pickups[pickup_count].type=type;
    pickups[pickup_count].active=1;
    pickups[pickup_count].bob_phase=randf()*2*PI;
    pickup_count++;
}

/* ──────────────────────────────── Player ───────────────────────────────── */

typedef enum { WPN_FIST=0, WPN_PISTOL=1, WPN_SHOTGUN=2 } WeaponType;

typedef struct {
    Vec3 pos; float yaw, pitch, vy;
    int on_ground, health, armor, ammo_pistol, ammo_shotgun;
    WeaponType weapon; float shoot_cd, muzzle_flash;
} Player;

static Player player;

static Vec3 player_forward(void) {
    return v3(sinf(player.yaw)*cosf(player.pitch),
              sinf(player.pitch),
              cosf(player.yaw)*cosf(player.pitch));
}

/* ──────────────────────── State Variables ──────────────────────────────── */

static float light_brightness = 1.0f;
#define LIGHT_MIN 0.1f
#define LIGHT_MAX 2.0f
#define LIGHT_STEP 0.1f

static int light_color_mode = 0;
static int flashlight_on = 1;
static float ambient_light_pos[4] = {20.0f, 2.5f, 20.0f, 1.0f};
static int show_help = 0;

/* Fog state */
static int fog_enabled = 1;
static float fog_density = 0.12f;

static int game_over = 0, game_win = 0;
static float game_time = 0;

/* ─────────────────────────── Collision helper ──────────────────────────── */

static int collides_map(float x, float z) {
    int col=(int)floorf(x), row=(int)floorf(z);
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++)
        if(map_is_wall(col+dc,row+dr)) {
            float wx=(float)(col+dc), wz=(float)(row+dr);
            if(x>wx-PLAYER_RADIUS && x<wx+1+PLAYER_RADIUS &&
               z>wz-PLAYER_RADIUS && z<wz+1+PLAYER_RADIUS) return 1;
        }
    return 0;
}

/* ───────────── Scatter pickups randomly on open floor tiles ─────────── */

static void scatter_pickups(int ch, int ca, int car) {
    int open_tiles[MAP_ROWS_N*MAP_COLS][2]; int oc=0;
    for(int r=1;r<MAP_ROWS_N-1;r++) for(int c=1;c<MAP_COLS-1;c++)
        if(MAP_ROWS[r][c]=='.') { open_tiles[oc][0]=c; open_tiles[oc][1]=r; oc++; }
    for(int i=oc-1;i>0;i--) {
        int j=rand()%(i+1);
        int tc=open_tiles[i][0],tr=open_tiles[i][1];
        open_tiles[i][0]=open_tiles[j][0]; open_tiles[i][1]=open_tiles[j][1];
        open_tiles[j][0]=tc; open_tiles[j][1]=tr;
    }
    int idx=0;
    for(int i=0;i<ch&&idx<oc;i++,idx++) pickup_spawn(open_tiles[idx][0]+0.5f,open_tiles[idx][1]+0.5f,PICKUP_HEALTH);
    for(int i=0;i<ca&&idx<oc;i++,idx++) pickup_spawn(open_tiles[idx][0]+0.5f,open_tiles[idx][1]+0.5f,PICKUP_AMMO);
    for(int i=0;i<car&&idx<oc;i++,idx++) pickup_spawn(open_tiles[idx][0]+0.5f,open_tiles[idx][1]+0.5f,PICKUP_ARMOR);
}

/* ──────────────────────────── Init from map ────────────────────────────── */

static void init_level(void) {
    enemy_count=0; deco_count=0; pickup_count=0; particle_count=0;
    for(int i=0;i<MAX_PARTICLES;i++) particles[i].active=0;

    for(int r=0;r<MAP_ROWS_N;r++) for(int c=0;c<MAP_COLS;c++) {
        char ch=MAP_ROWS[r][c];
        if(ch=='P') {
            player.pos=v3(c+0.5f,PLAYER_HEIGHT,r+0.5f);
            player.yaw=0;player.pitch=0;player.vy=0;player.on_ground=1;
            player.health=100;player.armor=0;
            player.ammo_pistol=50;player.ammo_shotgun=10;
            player.weapon=WPN_PISTOL;player.shoot_cd=0;player.muzzle_flash=0;
        } else if(ch=='E') enemy_spawn(c+0.5f,r+0.5f);
          else if(ch=='B') deco_spawn(c+0.5f,r+0.5f,DECO_BARREL);
          else if(ch=='C') deco_spawn(c+0.5f,r+0.5f,DECO_CRATE);
    }
    scatter_pickups(12,15,8);
}

/* ──────────────────────────── Shooting ─────────────────────────────────── */

static void shoot(void) {
    float cooldown=0, range=0; int damage=0;
    switch(player.weapon) {
        case WPN_FIST: cooldown=SHOOT_COOLDOWN_FIST;range=FIST_RANGE;damage=FIST_DAMAGE;break;
        case WPN_PISTOL: if(player.ammo_pistol<=0)return; cooldown=SHOOT_COOLDOWN_PISTOL;range=PISTOL_RANGE;damage=PISTOL_DAMAGE;break;
        case WPN_SHOTGUN: if(player.ammo_shotgun<=0)return; cooldown=SHOOT_COOLDOWN_SHOTGUN;range=SHOTGUN_RANGE;damage=SHOTGUN_DAMAGE;break;
    }
    if(player.shoot_cd>0) return;
    if(player.weapon==WPN_PISTOL) player.ammo_pistol--;
    if(player.weapon==WPN_SHOTGUN) player.ammo_shotgun--;
    player.shoot_cd=cooldown; player.muzzle_flash=MUZZLE_TIME;

    Vec3 fwd=player_forward();
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 d=v3sub(enemies[i].pos,player.pos);
        float along=v3dot(d,fwd);
        if(along<0.5f||along>range) continue;
        Vec3 closest=v3sub(d,v3scale(fwd,along));
        float hit_r=(player.weapon==WPN_SHOTGUN)?SHOTGUN_SPREAD:0.55f;
        if(v3len(closest)<hit_r) {
            /* Check if a wall (or window) blocks the shot */
            float ex=enemies[i].pos.x, ez=enemies[i].pos.z;
            float px2=player.pos.x, pz2=player.pos.z;
            float ddx=ex-px2, ddz=ez-pz2;
            float dist2=sqrtf(ddx*ddx+ddz*ddz);
            int blocked=0;
            if(dist2>0.1f) {
                float step=0.4f;
                int steps=(int)(dist2/step)+1;
                for(int s=1;s<steps;s++) {
                    float t=(float)s/steps;
                    int cc=(int)floorf(px2+ddx*t);
                    int rr=(int)floorf(pz2+ddz*t);
                    if(map_is_wall(cc,rr)) { blocked=1; break; }
                }
            }
            if(blocked) continue;

            enemies[i].health-=damage;
            enemies[i].pain_timer=0.2f;
            enemies[i].state=AI_CHASE;
            enemies[i].last_known_player=player.pos;
            /* Blood splatter! */
            emit_blood(enemies[i].pos);
            if(enemies[i].health<=0) {
                enemies[i].alive=0;
                int alive=0;
                for(int j=0;j<enemy_count;j++) alive+=enemies[j].alive;
                if(!alive) game_win=1;
            }
        }
    }
}

/* ──────────────────────────── Game update ──────────────────────────────── */

static void update(float dt) {
    if(game_over||game_win) return;
    game_time+=dt;

    const Uint8 *keys=SDL_GetKeyboardState(NULL);
    Vec3 fwd_xz=v3(sinf(player.yaw),0,cosf(player.yaw));
    Vec3 rgt_xz=v3(cosf(player.yaw),0,-sinf(player.yaw));
    Vec3 move=v3(0,0,0);
    if(keys[SDL_SCANCODE_W]) move=v3add(move,fwd_xz);
    if(keys[SDL_SCANCODE_S]) move=v3sub(move,fwd_xz);
    if(keys[SDL_SCANCODE_A]) move=v3add(move,rgt_xz);
    if(keys[SDL_SCANCODE_D]) move=v3sub(move,rgt_xz);

    float ml=v3len(move);
    if(ml>0) {
        move=v3scale(move,MOVE_SPEED*dt/ml);
        float nx=player.pos.x+move.x, nz=player.pos.z+move.z;
        if(!collides_map(nx,player.pos.z)) player.pos.x=nx;
        if(!collides_map(player.pos.x,nz)) player.pos.z=nz;
    }

    if(!player.on_ground) player.vy-=GRAVITY*dt;
    player.pos.y+=player.vy*dt;
    if(player.pos.y<=PLAYER_HEIGHT){player.pos.y=PLAYER_HEIGHT;player.vy=0;player.on_ground=1;}
    if(keys[SDL_SCANCODE_SPACE]&&player.on_ground){player.vy=JUMP_VEL;player.on_ground=0;}
    if(player.shoot_cd>0) player.shoot_cd-=dt;
    if(player.muzzle_flash>0) player.muzzle_flash-=dt;

    float lms=5.0f*dt;
    if(keys[SDL_SCANCODE_UP])    ambient_light_pos[2]+=lms;
    if(keys[SDL_SCANCODE_DOWN])  ambient_light_pos[2]-=lms;
    if(keys[SDL_SCANCODE_LEFT])  ambient_light_pos[0]-=lms;
    if(keys[SDL_SCANCODE_RIGHT]) ambient_light_pos[0]+=lms;

    /* Enemy AI (same as before) */
    for(int i=0;i<enemy_count;i++) {
        Enemy *e=&enemies[i];
        if(!e->alive) continue;
        if(e->pain_timer>0) e->pain_timer-=dt;
        if(e->attack_cooldown>0) e->attack_cooldown-=dt;
        e->bob_phase+=dt*3.0f; e->state_timer+=dt;

        Vec3 to_player=v3sub(player.pos,e->pos); to_player.y=0;
        float dist=v3len(to_player);
        int can_see=(dist<ENEMY_SIGHT_RANGE)&&has_line_of_sight(e->pos.x,e->pos.z,player.pos.x,player.pos.z);

        switch(e->state) {
        case AI_IDLE:
            if(can_see){e->state=AI_CHASE;e->last_known_player=player.pos;e->state_timer=0;}
            else if(e->state_timer>2.0f+(rand()%30)/10.0f){e->state=AI_PATROL;e->patrol_angle+=(float)(rand()%314-157)/100.0f;e->state_timer=0;}
            break;
        case AI_PATROL: {
            if(can_see){e->state=AI_CHASE;e->last_known_player=player.pos;e->state_timer=0;break;}
            float px2=e->pos.x+sinf(e->patrol_angle)*ENEMY_SPEED*0.5f*dt;
            float pz2=e->pos.z+cosf(e->patrol_angle)*ENEMY_SPEED*0.5f*dt;
            float ds=px2-e->spawn_pos.x,dds=pz2-e->spawn_pos.z;
            if(ds*ds+dds*dds>ENEMY_PATROL_RANGE*ENEMY_PATROL_RANGE)
                e->patrol_angle=atan2f(e->spawn_pos.x-e->pos.x,e->spawn_pos.z-e->pos.z);
            if(!collides_map(px2,e->pos.z))e->pos.x=px2; else e->patrol_angle+=PI*0.5f;
            if(!collides_map(e->pos.x,pz2))e->pos.z=pz2; else e->patrol_angle+=PI*0.5f;
            if(e->state_timer>3.0f+(rand()%20)/10.0f){e->state=AI_IDLE;e->state_timer=0;}
            break;
        }
        case AI_CHASE:
            if(dist<ENEMY_ATTACK_RANGE){e->state=AI_ATTACK;e->state_timer=0;}
            else if(!can_see&&e->state_timer>0.5f){e->state=AI_SEARCH;e->state_timer=0;}
            else {
                if(can_see) e->last_known_player=player.pos;
                Vec3 dir=v3norm(to_player); float spd=ENEMY_SPEED*dt;
                float nx2=e->pos.x+dir.x*spd, nz2=e->pos.z+dir.z*spd;
                if(!collides_map(nx2,e->pos.z)) e->pos.x=nx2;
                else { float sx=e->pos.x+dir.x*spd*0.5f,sz=e->pos.z+dir.z*spd;
                       if(!collides_map(sx,sz)){e->pos.x=sx;e->pos.z=sz;}}
                if(!collides_map(e->pos.x,nz2)) e->pos.z=nz2;
                else { float sx=e->pos.x+dir.x*spd,sz=e->pos.z+dir.z*spd*0.5f;
                       if(!collides_map(sx,sz)){e->pos.x=sx;e->pos.z=sz;}}
            }
            break;
        case AI_SEARCH: {
            if(can_see){e->state=AI_CHASE;e->last_known_player=player.pos;e->state_timer=0;break;}
            Vec3 tl=v3sub(e->last_known_player,e->pos);tl.y=0;float dl=v3len(tl);
            if(dl>0.5f){Vec3 dir=v3scale(tl,1.0f/dl);float spd=ENEMY_SPEED*0.7f*dt;
                float nx2=e->pos.x+dir.x*spd,nz2=e->pos.z+dir.z*spd;
                if(!collides_map(nx2,e->pos.z))e->pos.x=nx2;
                if(!collides_map(e->pos.x,nz2))e->pos.z=nz2;
            } else {e->state=AI_IDLE;e->state_timer=0;}
            if(e->state_timer>5.0f){e->state=AI_IDLE;e->state_timer=0;}
            break;
        }
        case AI_ATTACK:
            if(dist>ENEMY_ATTACK_RANGE*1.5f){e->state=AI_CHASE;e->state_timer=0;}
            if(e->attack_cooldown<=0&&dist<ENEMY_ATTACK_RANGE*1.2f) {
                int dmg=10;
                if(player.armor>0){int ab=dmg/2;if(ab>player.armor)ab=player.armor;player.armor-=ab;dmg-=ab;}
                player.health-=dmg;
                if(player.health<=0){player.health=0;game_over=1;}
                e->attack_cooldown=0.8f;
            }
            break;
        }
        /* Alert nearby */
        if(can_see&&(e->state==AI_CHASE||e->state==AI_ATTACK)) {
            for(int j=0;j<enemy_count;j++) {
                if(j==i||!enemies[j].alive) continue;
                if(enemies[j].state==AI_IDLE||enemies[j].state==AI_PATROL) {
                    Vec3 dd=v3sub(enemies[j].pos,e->pos);dd.y=0;
                    if(v3len(dd)<8.0f){enemies[j].state=AI_SEARCH;enemies[j].last_known_player=player.pos;enemies[j].state_timer=0;}
                }
            }
        }
    }

    /* Pickup collection */
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        pickups[i].bob_phase+=dt*3.0f;
        float dx=player.pos.x-pickups[i].pos.x,dz=player.pos.z-pickups[i].pos.z;
        if(dx*dx+dz*dz<1.0f) {
            switch(pickups[i].type) {
                case PICKUP_HEALTH: if(player.health<100){player.health+=25;if(player.health>100)player.health=100;pickups[i].active=0;} break;
                case PICKUP_AMMO: player.ammo_pistol+=15;player.ammo_shotgun+=4;pickups[i].active=0; break;
                case PICKUP_ARMOR: if(player.armor<200){player.armor+=25;if(player.armor>200)player.armor=200;pickups[i].active=0;} break;
            }
        }
    }

    /* Animate decorations */
    for(int i=0;i<deco_count;i++) {
        decorations[i].rotation+=15.0f*dt;
        if(decorations[i].rotation>360.0f) decorations[i].rotation-=360.0f;
    }

    /* Emit fire/smoke particles from barrels */
    for(int i=0;i<deco_count;i++) {
        if(decorations[i].type==DECO_BARREL)
            emit_barrel_fire(decorations[i].pos, dt);
    }

    /* Update all particles */
    particles_update(dt);
}

/* ─────────────────────────── Fog Setup ─────────────────────────────────── */

static void setup_fog(void) {
    if(fog_enabled) {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_EXP2);
        float fog_color[] = {0.02f, 0.02f, 0.05f, 1.0f}; /* match clear color */
        glFogfv(GL_FOG_COLOR, fog_color);
        glFogf(GL_FOG_DENSITY, fog_density);
        glHint(GL_FOG_HINT, GL_NICEST);
    } else {
        glDisable(GL_FOG);
    }
}

/* ─────────────────────────── Lighting Setup ────────────────────────────── */

static void setup_lighting(void) {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float r=1,g=1,b=1;
    switch(light_color_mode){case 1:r=1;g=0.3f;b=0.2f;break;case 2:r=0.3f;g=0.4f;b=1;break;}
    r*=light_brightness;g*=light_brightness;b*=light_brightness;

    glEnable(GL_LIGHT0);
    float d0[]={r*0.8f,g*0.8f,b*0.8f,1},a0[]={r*0.15f,g*0.15f,b*0.15f,1},s0[]={0.5f,0.5f,0.5f,1};
    glLightfv(GL_LIGHT0,GL_POSITION,ambient_light_pos);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,d0);glLightfv(GL_LIGHT0,GL_AMBIENT,a0);glLightfv(GL_LIGHT0,GL_SPECULAR,s0);
    glLightf(GL_LIGHT0,GL_CONSTANT_ATTENUATION,0.3f);
    glLightf(GL_LIGHT0,GL_LINEAR_ATTENUATION,0.05f);
    glLightf(GL_LIGHT0,GL_QUADRATIC_ATTENUATION,0.01f);

    if(flashlight_on) {
        glEnable(GL_LIGHT1);
        Vec3 fwd=player_forward();
        float fp[]={player.pos.x,player.pos.y,player.pos.z,1};
        float fd[]={fwd.x,fwd.y,fwd.z};
        float fdif[]={0.9f*light_brightness,0.85f*light_brightness,0.7f*light_brightness,1};
        float fa[]={0,0,0,1};
        glLightfv(GL_LIGHT1,GL_POSITION,fp);glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION,fd);
        glLightf(GL_LIGHT1,GL_SPOT_CUTOFF,25);glLightf(GL_LIGHT1,GL_SPOT_EXPONENT,30);
        glLightfv(GL_LIGHT1,GL_DIFFUSE,fdif);glLightfv(GL_LIGHT1,GL_AMBIENT,fa);
        glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,0.5f);
        glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,0.05f);
        glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,0.02f);
    } else glDisable(GL_LIGHT1);

    float ga[]={0.08f,0.08f,0.10f,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ga);
}

/* ──────────────────────────────── Render ───────────────────────────────── */

static void set_projection(void) {
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluPerspective(FOV,(float)WINDOW_W/WINDOW_H,NEAR_CLIP,FAR_CLIP);
    glMatrixMode(GL_MODELVIEW);
}

static void set_camera(void) {
    glLoadIdentity();
    Vec3 e=player.pos,f=player_forward(),c=v3add(e,f);
    gluLookAt(e.x,e.y,e.z,c.x,c.y,c.z,0,1,0);
}

static void quad_n(float x0,float y0,float z0,float x1,float y1,float z1,
                   float x2,float y2,float z2,float x3,float y3,float z3,
                   float nx,float ny,float nz,float us,float vs) {
    glNormal3f(nx,ny,nz);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex3f(x0,y0,z0);
    glTexCoord2f(us,0);glVertex3f(x1,y1,z1);
    glTexCoord2f(us,vs);glVertex3f(x2,y2,z2);
    glTexCoord2f(0,vs);glVertex3f(x3,y3,z3);
    glEnd();
}

static void draw_level(void) {
    glEnable(GL_TEXTURE_2D);
    for(int r=0;r<MAP_ROWS_N;r++) for(int c=0;c<MAP_COLS;c++) {
        float x0=(float)c,z0=(float)r,x1=x0+1,z1=z0+1;
        char ch = MAP_ROWS[r][c];
        if(ch!='#' && ch!='W') {
            glBindTexture(GL_TEXTURE_2D,tex_floor);glColor3f(0.9f,0.9f,0.9f);
            quad_n(x0,FLOOR_Y,z0,x1,FLOOR_Y,z0,x1,FLOOR_Y,z1,x0,FLOOR_Y,z1,0,1,0,1,1);
            glBindTexture(GL_TEXTURE_2D,tex_ceil);glColor3f(0.6f,0.6f,0.7f);
            quad_n(x0,CEIL_Y,z1,x1,CEIL_Y,z1,x1,CEIL_Y,z0,x0,CEIL_Y,z0,0,-1,0,1,1);
        }
        /* Solid walls only - windows drawn in transparency pass */
        if(ch=='#') {
            glBindTexture(GL_TEXTURE_2D,tex_wall);glColor3f(1,1,1);
            if(!map_is_solid_wall(c,r-1)&&!map_is_window(c,r-1)) quad_n(x0,CEIL_Y,z0,x1,CEIL_Y,z0,x1,FLOOR_Y,z0,x0,FLOOR_Y,z0,0,0,-1,1,1);
            if(!map_is_solid_wall(c,r+1)&&!map_is_window(c,r+1)) quad_n(x1,CEIL_Y,z1,x0,CEIL_Y,z1,x0,FLOOR_Y,z1,x1,FLOOR_Y,z1,0,0,1,1,1);
            if(!map_is_solid_wall(c-1,r)&&!map_is_window(c-1,r)) quad_n(x0,CEIL_Y,z1,x0,CEIL_Y,z0,x0,FLOOR_Y,z0,x0,FLOOR_Y,z1,-1,0,0,1,1);
            if(!map_is_solid_wall(c+1,r)&&!map_is_window(c+1,r)) quad_n(x1,CEIL_Y,z0,x1,CEIL_Y,z1,x1,FLOOR_Y,z1,x1,FLOOR_Y,z0,1,0,0,1,1);
            /* Also draw face toward windows */
            if(map_is_window(c,r-1)) quad_n(x0,CEIL_Y,z0,x1,CEIL_Y,z0,x1,FLOOR_Y,z0,x0,FLOOR_Y,z0,0,0,-1,1,1);
            if(map_is_window(c,r+1)) quad_n(x1,CEIL_Y,z1,x0,CEIL_Y,z1,x0,FLOOR_Y,z1,x1,FLOOR_Y,z1,0,0,1,1,1);
            if(map_is_window(c-1,r)) quad_n(x0,CEIL_Y,z1,x0,CEIL_Y,z0,x0,FLOOR_Y,z0,x0,FLOOR_Y,z1,-1,0,0,1,1);
            if(map_is_window(c+1,r)) quad_n(x1,CEIL_Y,z0,x1,CEIL_Y,z1,x1,FLOOR_Y,z1,x1,FLOOR_Y,z0,1,0,0,1,1);
        }
    }
    glDisable(GL_TEXTURE_2D);
}

/* Draw transparent windows in a separate pass */
static void draw_windows(void) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, tex_window);
    glColor4f(1,1,1,1); /* texture has its own alpha */

    for(int r=0;r<MAP_ROWS_N;r++) for(int c=0;c<MAP_COLS;c++) {
        if(!map_is_window(c,r)) continue;
        float x0=(float)c,z0=(float)r,x1=x0+1,z1=z0+1;

        /* Draw faces toward non-wall neighbors */
        if(!map_is_solid_wall(c,r-1)&&!map_is_window(c,r-1))
            quad_n(x0,CEIL_Y,z0,x1,CEIL_Y,z0,x1,FLOOR_Y,z0,x0,FLOOR_Y,z0,0,0,-1,1,1);
        if(!map_is_solid_wall(c,r+1)&&!map_is_window(c,r+1))
            quad_n(x1,CEIL_Y,z1,x0,CEIL_Y,z1,x0,FLOOR_Y,z1,x1,FLOOR_Y,z1,0,0,1,1,1);
        if(!map_is_solid_wall(c-1,r)&&!map_is_window(c-1,r))
            quad_n(x0,CEIL_Y,z1,x0,CEIL_Y,z0,x0,FLOOR_Y,z0,x0,FLOOR_Y,z1,-1,0,0,1,1);
        if(!map_is_solid_wall(c+1,r)&&!map_is_window(c+1,r))
            quad_n(x1,CEIL_Y,z0,x1,CEIL_Y,z1,x1,FLOOR_Y,z1,x1,FLOOR_Y,z0,1,0,0,1,1);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

/* ──────────── Shadows: dark blobs projected onto floor ─────────────────── */

static void draw_shadow_blob(float x, float z, float radius) {
    glPushMatrix();
    glTranslatef(x, FLOOR_Y + 0.01f, z); /* slightly above floor */
    glRotatef(-90, 1, 0, 0); /* lay flat */
    glScalef(radius, radius, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(-1, -1, 0);
    glTexCoord2f(1,0); glVertex3f( 1, -1, 0);
    glTexCoord2f(1,1); glVertex3f( 1,  1, 0);
    glTexCoord2f(0,1); glVertex3f(-1,  1, 0);
    glEnd();
    glPopMatrix();
}

static void draw_shadows(void) {
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, tex_shadow);
    glColor4f(1,1,1,1);

    /* Enemy shadows */
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        draw_shadow_blob(enemies[i].pos.x, enemies[i].pos.z, 0.5f);
    }

    /* Decoration shadows */
    for(int i=0;i<deco_count;i++) {
        float r = (decorations[i].type==DECO_BARREL) ? 0.35f : 0.45f;
        draw_shadow_blob(decorations[i].pos.x, decorations[i].pos.z, r);
    }

    /* Pickup shadows (smaller) */
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        draw_shadow_blob(pickups[i].pos.x, pickups[i].pos.z, 0.2f);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

/* ────────────────── Draw enemies, decorations, pickups ─────────────────── */

static void draw_enemies(void) {
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,tex_enemy);
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 p=enemies[i].pos; float bob=sinf(enemies[i].bob_phase)*0.1f;
        if(enemies[i].pain_timer>0) glColor3f(1,0.3f,0.3f); else glColor3f(1,1,1);
        glPushMatrix();
        glTranslatef(p.x,p.y+bob,p.z);
        float dx=player.pos.x-p.x,dz=player.pos.z-p.z;
        glRotatef(atan2f(dx,dz)*180.0f/PI,0,1,0);
        glScalef(0.6f,0.8f,0.6f); obj_draw(&model_enemy);
        glPopMatrix();
    }
    glColor3f(1,1,1);glDisable(GL_TEXTURE_2D);
}

static void draw_decorations(void) {
    glEnable(GL_TEXTURE_2D);
    for(int i=0;i<deco_count;i++) {
        Vec3 p=decorations[i].pos;
        glPushMatrix();glTranslatef(p.x,p.y,p.z);glRotatef(decorations[i].rotation,0,1,0);
        if(decorations[i].type==DECO_BARREL){
            glBindTexture(GL_TEXTURE_2D,tex_barrel);glColor3f(0.8f,0.9f,0.8f);
            glScalef(0.5f,0.8f,0.5f);obj_draw(&model_barrel);
        } else {
            glBindTexture(GL_TEXTURE_2D,tex_crate);glColor3f(0.9f,0.8f,0.6f);
            glScalef(0.7f,0.7f,0.7f);obj_draw(&model_cube);
        }
        glPopMatrix();
    }
    glColor3f(1,1,1);glDisable(GL_TEXTURE_2D);
}

static void draw_pickups(void) {
    glEnable(GL_TEXTURE_2D);
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        Vec3 p=pickups[i].pos; float bob=sinf(pickups[i].bob_phase)*0.1f;
        glPushMatrix();glTranslatef(p.x,p.y+bob,p.z);
        glRotatef(game_time*90,0,1,0);glScalef(0.3f,0.3f,0.3f);
        switch(pickups[i].type){
            case PICKUP_HEALTH:glBindTexture(GL_TEXTURE_2D,tex_pickup_health);glColor3f(0.3f,1,0.3f);break;
            case PICKUP_AMMO:glBindTexture(GL_TEXTURE_2D,tex_pickup_ammo);glColor3f(1,0.9f,0.3f);break;
            case PICKUP_ARMOR:glBindTexture(GL_TEXTURE_2D,tex_pickup_armor);glColor3f(0.3f,0.5f,1);break;
        }
        obj_draw(&model_cube);glPopMatrix();
    }
    glColor3f(1,1,1);glDisable(GL_TEXTURE_2D);
}

static void draw_light_marker(void) {
    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);
    float pulse=0.7f+0.3f*sinf(game_time*4);
    switch(light_color_mode){case 1:glColor3f(pulse,0.3f*pulse,0.1f*pulse);break;case 2:glColor3f(0.2f*pulse,0.3f*pulse,pulse);break;default:glColor3f(pulse,pulse,0.8f*pulse);}
    glPushMatrix();glTranslatef(ambient_light_pos[0],ambient_light_pos[1],ambient_light_pos[2]);
    float s=0.15f;
    glBegin(GL_TRIANGLES);
    glVertex3f(0,s*2,0);glVertex3f(-s,0,-s);glVertex3f(s,0,-s);
    glVertex3f(0,s*2,0);glVertex3f(s,0,-s);glVertex3f(s,0,s);
    glVertex3f(0,s*2,0);glVertex3f(s,0,s);glVertex3f(-s,0,s);
    glVertex3f(0,s*2,0);glVertex3f(-s,0,s);glVertex3f(-s,0,-s);
    glVertex3f(0,-s*2,0);glVertex3f(s,0,-s);glVertex3f(-s,0,-s);
    glVertex3f(0,-s*2,0);glVertex3f(s,0,s);glVertex3f(s,0,-s);
    glVertex3f(0,-s*2,0);glVertex3f(-s,0,s);glVertex3f(s,0,s);
    glVertex3f(0,-s*2,0);glVertex3f(-s,0,-s);glVertex3f(-s,0,s);
    glEnd();glPopMatrix();glEnable(GL_LIGHTING);
}

/* ────────────────── Bitmap Font ───────────────────────────────────────── */

static const unsigned long long FONT_GLYPHS[128] = {
    [' ']=0,['+' ]=0x0018187E18180000ULL,['-']=0x000000FE00000000ULL,
    ['.']=0x0000000000181800ULL,['/']=0x060C183060C08000ULL,['%']=0xC6CC183066C60000ULL,
    ['0']=0x7CC6CEDEF6E67C00ULL,['1']=0x1838181818187E00ULL,['2']=0x7CC6060C3060FE00ULL,
    ['3']=0x7CC6061C06C67C00ULL,['4']=0x0C1C3C6CFE0C0C00ULL,['5']=0xFEC0FC0606C67C00ULL,
    ['6']=0x3C60C0FCC6C67C00ULL,['7']=0xFE06060C18181800ULL,['8']=0x7CC6C67CC6C67C00ULL,
    ['9']=0x7CC6C67E06067C00ULL,[':']=0x0018180018180000ULL,['(']=0x0C18303030180C00ULL,
    [')']=0x30180C0C0C183000ULL,['!']=0x1818181818001800ULL,['*']=0x0066663CFF3C6600ULL,
    ['[']=0x3C30303030303C00ULL,[']']=0x3C0C0C0C0C0C3C00ULL,
    ['A']=0x386CC6FEC6C6C600ULL,['B']=0xFC66667C6666FC00ULL,['C']=0x3C66C0C0C0663C00ULL,
    ['D']=0xF86C6666666CF800ULL,['E']=0xFE6268786862FE00ULL,['F']=0xFE6268786860F000ULL,
    ['G']=0x3C66C0C0CE663E00ULL,['H']=0xC6C6C6FEC6C6C600ULL,['I']=0x3C18181818183C00ULL,
    ['J']=0x1E0C0C0CCCCC7800ULL,['K']=0xC6CCD8F0D8CCC600ULL,['L']=0xF06060606266FE00ULL,
    ['M']=0xC6EEFEFED6C6C600ULL,['N']=0xC6E6F6DECEC6C600ULL,['O']=0x7CC6C6C6C6C67C00ULL,
    ['P']=0xFC66667C6060F000ULL,['Q']=0x7CC6C6C6D6DE7C06ULL,['R']=0xFC66667C6C66F200ULL,
    ['S']=0x7CC6C07C06C67C00ULL,['T']=0x7E5A181818183C00ULL,['U']=0xC6C6C6C6C6C67C00ULL,
    ['V']=0xC6C6C6C66C381000ULL,['W']=0xC6C6D6FEEEC6C600ULL,['X']=0xC66C38386CC6C600ULL,
    ['Y']=0x6666663C18183C00ULL,['Z']=0xFE860C183062FE00ULL,
    ['a']=0x0000780C7CCC7600ULL,['b']=0xE060607C6666DC00ULL,['c']=0x00007CC6C0C67C00ULL,
    ['d']=0x1C0C0C7CCCCC7600ULL,['e']=0x00007CC6FEC07C00ULL,['f']=0x1C3630FC30303000ULL,
    ['g']=0x000076CCCC7C0CF8ULL,['h']=0xE0606C766666E600ULL,['i']=0x1800381818183C00ULL,
    ['j']=0x0600060606C67C00ULL,['k']=0xE060666C786CE600ULL,['l']=0x3818181818183C00ULL,
    ['m']=0x0000ECFED6D6C600ULL,['n']=0x0000DC6666666600ULL,['o']=0x00007CC6C6C67C00ULL,
    ['p']=0x0000DC667C60F000ULL,['q']=0x000076CC7C0C1E00ULL,['r']=0x0000DC7660606000ULL,
    ['s']=0x00007CC07C06FC00ULL,['t']=0x1030FC3030361C00ULL,['u']=0x0000CCCCCCCC7600ULL,
    ['v']=0x0000C6C66C381000ULL,['w']=0x0000C6D6FEEE4400ULL,['x']=0x0000C66C386CC600ULL,
    ['y']=0x0000C6C67E060CFCULL,['z']=0x0000FC983064FC00ULL,
    [',']=0x0000000000181830ULL,['?']=0x7CC60C1818001800ULL,
};

static void draw_char(float px, float py, float size, char ch) {
    unsigned char uch=(unsigned char)ch; if(uch>=128)return;
    unsigned long long g=FONT_GLYPHS[uch]; if(!g&&ch!=' ')return;
    for(int row=0;row<8;row++){
        unsigned char bits=(unsigned char)((g>>(56-row*8))&0xFF);
        for(int col=0;col<8;col++) if(bits&(0x80>>col)){
            float x=px+col*size,y=py+(7-row)*size;
            glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+size,y);glVertex2f(x+size,y+size);glVertex2f(x,y+size);glEnd();
        }
    }
}

static void draw_text(float x, float y, float size, const char *text) {
    for(int i=0;text[i];i++) draw_char(x+i*size*9,y,size,text[i]);
}

/* ──────────────────── Doom-style Status Bar HUD ───────────────────────── */

static void ortho_begin(void) {
    glDisable(GL_LIGHTING);glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    glOrtho(0,WINDOW_W,0,WINDOW_H,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}

static void ortho_end(void) {
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glEnable(GL_LIGHTING);
    if(fog_enabled) glEnable(GL_FOG);
}

static void fill_rect(float x,float y,float w,float h,float r,float g,float b,float a) {
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
    glDisable(GL_BLEND);
}

static void draw_doom_hud(void) {
    ortho_begin();
    float bar_h=60, bar_y=0, sec_w=WINDOW_W/5.0f, inset=4;
    fill_rect(0,bar_y,WINDOW_W,bar_h, 0.22f,0.22f,0.22f,0.95f);
    fill_rect(0,bar_y+bar_h-2,WINDOW_W,2, 0.5f,0.5f,0.5f,1);
    for(int i=1;i<5;i++) fill_rect(sec_w*i-1,bar_y,2,bar_h,0.1f,0.1f,0.1f,1);

    /* Sec 1: AMMO */
    {float sx=0; fill_rect(sx+inset,bar_y+inset,sec_w-inset*2,bar_h-inset*2-4,0.12f,0.12f,0.12f,1);
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+sec_w/2-30,bar_y+bar_h-16,1.2f,"AMMO");
     int ca=0; switch(player.weapon){case WPN_FIST:ca=-1;break;case WPN_PISTOL:ca=player.ammo_pistol;break;case WPN_SHOTGUN:ca=player.ammo_shotgun;break;}
     char buf[16]; if(ca<0)snprintf(buf,sizeof(buf),"--");else snprintf(buf,sizeof(buf),"%d",ca);
     glColor4f(0.9f,0.2f,0.2f,1);draw_text(sx+sec_w/2-24,bar_y+12,3.0f,buf);}

    /* Sec 2: HEALTH */
    {float sx=sec_w; fill_rect(sx+inset,bar_y+inset,sec_w-inset*2,bar_h-inset*2-4,0.12f,0.12f,0.12f,1);
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+sec_w/2-38,bar_y+bar_h-16,1.2f,"HEALTH");
     char buf[16];snprintf(buf,sizeof(buf),"%d%%",player.health);
     glColor4f(0.9f,0.2f,0.2f,1);draw_text(sx+sec_w/2-36,bar_y+12,3.0f,buf);}

    /* Sec 3: ARMS */
    {float sx=sec_w*2; fill_rect(sx+inset,bar_y+inset,sec_w-inset*2,bar_h-inset*2-4,0.12f,0.12f,0.12f,1);
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+sec_w/2-24,bar_y+bar_h-16,1.2f,"ARMS");
     const char *wn[]={"1","2","3"};
     for(int w=0;w<3;w++){
         float bx=sx+inset+12+w*(sec_w-inset*2-20)/3.0f,by=bar_y+10,bw=(sec_w-inset*2-40)/3.0f,bhs=30;
         fill_rect(bx,by,bw,bhs, w==(int)player.weapon?0.35f:0.18f, w==(int)player.weapon?0.30f:0.18f, w==(int)player.weapon?0.20f:0.18f, 1);
         glColor4f(w==(int)player.weapon?1.0f:0.5f, w==(int)player.weapon?0.9f:0.45f, w==(int)player.weapon?0.2f:0.35f, 1);
         draw_text(bx+bw/2-8,by+8,2.0f,wn[w]);
     }}

    /* Sec 4: ARMOR */
    {float sx=sec_w*3; fill_rect(sx+inset,bar_y+inset,sec_w-inset*2,bar_h-inset*2-4,0.12f,0.12f,0.12f,1);
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+sec_w/2-34,bar_y+bar_h-16,1.2f,"ARMOR");
     char buf[16];snprintf(buf,sizeof(buf),"%d%%",player.armor);
     glColor4f(0.9f,0.2f,0.2f,1);draw_text(sx+sec_w/2-36,bar_y+12,3.0f,buf);}

    /* Sec 5: Ammo types */
    {float sx=sec_w*4; fill_rect(sx+inset,bar_y+inset,sec_w-inset*2,bar_h-inset*2-4,0.12f,0.12f,0.12f,1);
     char buf[32];
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+10,bar_y+34,1.2f,"PSTL");
     snprintf(buf,sizeof(buf),"%d",player.ammo_pistol);glColor4f(0.9f,0.8f,0.2f,1);draw_text(sx+70,bar_y+32,2.0f,buf);
     glColor4f(0.65f,0.55f,0.45f,1);draw_text(sx+10,bar_y+10,1.2f,"SHTG");
     snprintf(buf,sizeof(buf),"%d",player.ammo_shotgun);glColor4f(0.9f,0.8f,0.2f,1);draw_text(sx+70,bar_y+8,2.0f,buf);}

    /* Crosshair */
    int cx=WINDOW_W/2,cy=WINDOW_H/2;
    fill_rect(cx-12,cy-2,10,4,1,1,1,0.8f);fill_rect(cx+2,cy-2,10,4,1,1,1,0.8f);
    fill_rect(cx-2,cy-12,4,10,1,1,1,0.8f);fill_rect(cx-2,cy+2,4,10,1,1,1,0.8f);

    if(player.muzzle_flash>0){float a=player.muzzle_flash/MUZZLE_TIME;fill_rect(0,0,WINDOW_W,WINDOW_H,1,0.8f,0,0.15f*a);}

    /* Top info */
    glColor4f(1,1,0.6f,0.5f);
    char lbuf[80];
    snprintf(lbuf,sizeof(lbuf),"LIGHT:%.0f%% %s %s  FOG:%s %.0f%%",
             light_brightness*100,
             (const char*[]){"WHITE","RED","BLUE"}[light_color_mode],
             flashlight_on?"FLASH:ON":"FLASH:OFF",
             fog_enabled?"ON":"OFF", fog_density*1000);
    draw_text(10,WINDOW_H-20,1.3f,lbuf);

    glColor4f(0.6f,0.6f,0.6f,0.4f);draw_text(WINDOW_W-160,WINDOW_H-20,1.3f,"F1: HELP");

    int alive=0; for(int i=0;i<enemy_count;i++)alive+=enemies[i].alive;
    glColor4f(1,0.4f,0.4f,0.7f); char ebuf[32];snprintf(ebuf,sizeof(ebuf),"ENEMIES: %d",alive);
    draw_text(10,WINDOW_H-40,1.5f,ebuf);

    if(game_over){fill_rect(WINDOW_W/2-200,WINDOW_H/2-50,400,100,0.8f,0.05f,0.05f,0.85f);
        glColor4f(1,1,1,1);draw_text(WINDOW_W/2-100,WINDOW_H/2-10,3.0f,"YOU DIED");
        glColor4f(0.8f,0.8f,0.8f,0.8f);draw_text(WINDOW_W/2-120,WINDOW_H/2-35,1.5f,"PRESS R TO RESTART");}
    if(game_win){fill_rect(WINDOW_W/2-200,WINDOW_H/2-50,400,100,0.05f,0.6f,0.05f,0.85f);
        glColor4f(1,1,1,1);draw_text(WINDOW_W/2-100,WINDOW_H/2-10,3.0f,"YOU WIN!");
        glColor4f(0.8f,0.8f,0.8f,0.8f);draw_text(WINDOW_W/2-120,WINDOW_H/2-35,1.5f,"PRESS R TO RESTART");}

    ortho_end();
}

/* ──────────────────────────── Help Screen ──────────────────────────────── */

static void draw_help_screen(void) {
    ortho_begin();
    fill_rect(0,0,WINDOW_W,WINDOW_H,0,0,0,0.75f);
    float px=140,py=30,pw=WINDOW_W-280,ph=WINDOW_H-60;
    fill_rect(px,py,pw,ph,0.1f,0.1f,0.15f,0.95f);
    fill_rect(px+2,py+2,pw-4,ph-4,0.15f,0.15f,0.2f,0.95f);
    float tx=px+25,ty=py+ph-40,lh=22,sz=1.7f;

    glColor4f(1,0.4f,0.3f,1);draw_text(tx,ty,2.5f,"DOOM3D - HELP");ty-=lh*1.4f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"MOVEMENT:");ty-=lh;
    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"W S A D  - MOVE    MOUSE - LOOK");ty-=lh;
    draw_text(tx,ty,sz,"SPACE - JUMP   L.CLICK - SHOOT");ty-=lh*1.2f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"WEAPONS:");ty-=lh;
    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"1 - FIST   2 - PISTOL   3 - SHOTGUN");ty-=lh*1.2f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"LIGHTING:");ty-=lh;
    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"+/- BRIGHTNESS   L - COLOR   F - FLASH");ty-=lh;
    draw_text(tx,ty,sz,"ARROWS - MOVE SCENE LIGHT");ty-=lh*1.2f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"FOG:");ty-=lh;
    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"G - TOGGLE FOG   [ ] - FOG DENSITY");ty-=lh*1.2f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"PICKUPS:");ty-=lh;
    glColor4f(0.5f,1,0.5f,0.9f);draw_text(tx,ty,sz,"GREEN - HEALTH (+25)");ty-=lh;
    glColor4f(1,0.9f,0.4f,0.9f);draw_text(tx,ty,sz,"YELLOW - AMMO (+15P +4S)");ty-=lh;
    glColor4f(0.4f,0.6f,1,0.9f);draw_text(tx,ty,sz,"BLUE - ARMOR (+25)");ty-=lh*1.2f;

    glColor4f(1,0.9f,0.5f,1);draw_text(tx,ty,sz,"FEATURES:");ty-=lh;
    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"FOG, PARTICLES (FIRE+BLOOD),");ty-=lh;
    draw_text(tx,ty,sz,"TRANSPARENCY (GLASS WINDOWS),");ty-=lh;
    draw_text(tx,ty,sz,"SHADOWS (PROJECTED BLOBS)");ty-=lh*1.2f;

    glColor4f(0.8f,0.8f,0.8f,0.9f);
    draw_text(tx,ty,sz,"F1 - HELP   R - RESTART   ESC - QUIT");

    ortho_end();
}

/* ───────────────────────────────── Main ────────────────────────────────── */

int main(int argc, char *argv[]) {
    (void)argc;(void)argv;
    char asset_dir[512]="assets";

    if(SDL_Init(SDL_INIT_VIDEO)<0){fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 1;}
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);

    SDL_Window *win=SDL_CreateWindow(WINDOW_TITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WINDOW_W,WINDOW_H,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    if(!win){fprintf(stderr,"SDL_CreateWindow: %s\n",SDL_GetError());return 1;}
    SDL_GLContext ctx=SDL_GL_CreateContext(win);
    if(!ctx){fprintf(stderr,"SDL_GL_CreateContext: %s\n",SDL_GetError());return 1;}
    SDL_GL_SetSwapInterval(1);SDL_SetRelativeMouseMode(SDL_TRUE);

    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);
    glClearColor(0.02f,0.02f,0.05f,1);glShadeModel(GL_SMOOTH);

    if(!load_all_models(asset_dir)){
        fprintf(stderr,"Failed to load models from '%s/'\n",asset_dir);
        SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(win);SDL_Quit();return 1;
    }

    init_textures(); build_all_display_lists();
    srand((unsigned)time(NULL)); init_level(); set_projection();

    Uint32 prev=SDL_GetTicks(); int running=1;

    while(running) {
        Uint32 now=SDL_GetTicks(); float dt=(now-prev)/1000.0f;
        if(dt>0.05f) { dt=0.05f; }
        prev=now;

        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT) running=0;
            if(ev.type==SDL_KEYDOWN) {
                switch(ev.key.keysym.sym) {
                    case SDLK_ESCAPE:running=0;break;
                    case SDLK_F1:show_help=!show_help;break;
                    case SDLK_f:flashlight_on=!flashlight_on;break;
                    case SDLK_l:light_color_mode=(light_color_mode+1)%3;break;
                    case SDLK_g:fog_enabled=!fog_enabled;break;
                    case SDLK_1:player.weapon=WPN_FIST;break;
                    case SDLK_2:player.weapon=WPN_PISTOL;break;
                    case SDLK_3:player.weapon=WPN_SHOTGUN;break;
                    case SDLK_LEFTBRACKET:
                        fog_density-=FOG_DENSITY_STEP;
                        if(fog_density<FOG_DENSITY_MIN){fog_density=FOG_DENSITY_MIN;}
                        break;
                    case SDLK_RIGHTBRACKET:
                        fog_density+=FOG_DENSITY_STEP;
                        if(fog_density>FOG_DENSITY_MAX){fog_density=FOG_DENSITY_MAX;}
                        break;
                    case SDLK_EQUALS:case SDLK_PLUS:case SDLK_KP_PLUS:
                        light_brightness+=LIGHT_STEP;
                        if(light_brightness>LIGHT_MAX){light_brightness=LIGHT_MAX;}
                        break;
                    case SDLK_MINUS:case SDLK_KP_MINUS:
                        light_brightness-=LIGHT_STEP;
                        if(light_brightness<LIGHT_MIN){light_brightness=LIGHT_MIN;}
                        break;
                    case SDLK_r:if(game_over||game_win){game_over=0;game_win=0;init_level();}break;
                    default:break;
                }
            }
            if(ev.type==SDL_MOUSEMOTION&&!game_over&&!game_win&&!show_help){
                player.yaw-=ev.motion.xrel*TURN_SPEED;
                player.pitch-=ev.motion.yrel*TURN_SPEED;
                if(player.pitch>1.4f)player.pitch=1.4f;
                if(player.pitch<-1.4f)player.pitch=-1.4f;
            }
            if(ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT&&!show_help) shoot();
        }

        if(!show_help) update(dt);

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        set_camera();
        setup_lighting();
        setup_fog();

        /* Render order: opaque -> shadows -> transparent -> particles -> HUD */
        draw_level();
        draw_shadows();
        draw_decorations();
        draw_pickups();
        draw_enemies();
        draw_light_marker();
        draw_windows();       /* transparent - after all opaque */
        particles_draw();     /* blended - after everything 3D */
        draw_doom_hud();

        if(show_help) draw_help_screen();

        SDL_GL_SwapWindow(win);
    }

    glDeleteLists(model_cube.display_list,1);
    glDeleteLists(model_enemy.display_list,1);
    glDeleteLists(model_barrel.display_list,1);
    glDeleteTextures(1,&tex_wall);glDeleteTextures(1,&tex_floor);
    glDeleteTextures(1,&tex_ceil);glDeleteTextures(1,&tex_enemy);
    glDeleteTextures(1,&tex_crate);glDeleteTextures(1,&tex_barrel);
    glDeleteTextures(1,&tex_pickup_health);glDeleteTextures(1,&tex_pickup_ammo);
    glDeleteTextures(1,&tex_pickup_armor);glDeleteTextures(1,&tex_window);
    glDeleteTextures(1,&tex_shadow);
    SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(win);SDL_Quit();
    return 0;
}