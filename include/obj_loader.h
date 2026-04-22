#ifndef DOOM3D_OBJ_LOADER_H
#define DOOM3D_OBJ_LOADER_H

#include "common.h"

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

int  obj_load(ObjModel *m, const char *path);
void obj_build_display_list(ObjModel *m);
void obj_draw(const ObjModel *m);

/* Global models (loaded once, used everywhere) */
extern ObjModel model_cube;
extern ObjModel model_enemy;
extern ObjModel model_barrel;

int  load_all_models(const char *asset_dir);
void build_all_display_lists(void);

#endif
