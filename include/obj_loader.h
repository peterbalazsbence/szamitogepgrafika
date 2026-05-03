#ifndef DOOM3D_OBJ_LOADER_H
#define DOOM3D_OBJ_LOADER_H

#include "common.h"

#define MAX_OBJ_VERTS   4096
#define MAX_OBJ_NORMS   4096
#define MAX_OBJ_UVS     4096
#define MAX_OBJ_FACES   4096

/* A single face: triangle (count=3) or quad (count=4).
 * vi/ti/ni are 0-based indices into the model's vert/uv/norm arrays,
 * or -1 if not provided in the OBJ file. */
typedef struct {
    int vi[4], ti[4], ni[4];
    int count;
} ObjFace;

/* Parsed Wavefront OBJ model. After load and display-list build,
 * obj_draw can render it via a single GL call. */
typedef struct {
    float verts[MAX_OBJ_VERTS][3];
    float norms[MAX_OBJ_NORMS][3];
    float uvs[MAX_OBJ_UVS][2];
    ObjFace faces[MAX_OBJ_FACES];
    int vert_count, norm_count, uv_count, face_count;
    GLuint display_list;
} ObjModel;

/* Loads an OBJ from file. Returns 1 on success, 0 if the file is missing.
 * Supports v, vn, vt, and f directives with v/vt/vn, v//vn, v/vt or v formats. */
int  obj_load(ObjModel *m, const char *path);

/* Compiles the loaded geometry into an OpenGL display list. */
void obj_build_display_list(ObjModel *m);

/* Renders the model. Requires obj_build_display_list to have been called. */
void obj_draw(const ObjModel *m);

/* Globally loaded models used by the game. */
extern ObjModel model_cube;
extern ObjModel model_enemy;
extern ObjModel model_barrel;

/* Loads cube.obj, sphere.obj and barrel.obj from the given asset directory.
 * Returns 1 on success, 0 if any file fails to load. */
int  load_all_models(const char *asset_dir);

/* Builds display lists for all globally loaded models. */
void build_all_display_lists(void);

#endif
