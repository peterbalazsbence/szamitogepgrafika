#include "obj_loader.h"

ObjModel model_cube;
ObjModel model_enemy;
ObjModel model_barrel;

int obj_load(ObjModel *m, const char *path) {
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
                    if (sscanf(p, "%d/%d/%d", &vi, &ti, &ni) == 3) { }
                    else if (sscanf(p, "%d//%d", &vi, &ni) == 2) { ti = 0; }
                    else if (sscanf(p, "%d/%d", &vi, &ti) == 2) { ni = 0; }
                    else { sscanf(p, "%d", &vi); }
                    face->vi[idx] = vi - 1;
                    face->ti[idx] = ti - 1;
                    face->ni[idx] = ni - 1;
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
    printf("Loaded OBJ: %s (%d verts, %d faces)\n",
           path, m->vert_count, m->face_count);
    return 1;
}

void obj_build_display_list(ObjModel *m) {
    m->display_list = glGenLists(1);
    glNewList(m->display_list, GL_COMPILE);
    for (int i = 0; i < m->face_count; i++) {
        ObjFace *face = &m->faces[i];
        glBegin(face->count == 3 ? GL_TRIANGLES : GL_QUADS);
        for (int j = 0; j < face->count; j++) {
            if (face->ni[j]>=0 && face->ni[j]<m->norm_count)
                glNormal3fv(m->norms[face->ni[j]]);
            if (face->ti[j]>=0 && face->ti[j]<m->uv_count)
                glTexCoord2fv(m->uvs[face->ti[j]]);
            if (face->vi[j]>=0 && face->vi[j]<m->vert_count)
                glVertex3fv(m->verts[face->vi[j]]);
        }
        glEnd();
    }
    glEndList();
}

void obj_draw(const ObjModel *m) {
    glCallList(m->display_list);
}

int load_all_models(const char *asset_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/cube.obj", asset_dir);
    if (!obj_load(&model_cube, path)) return 0;
    snprintf(path, sizeof(path), "%s/sphere.obj", asset_dir);
    if (!obj_load(&model_enemy, path)) return 0;
    snprintf(path, sizeof(path), "%s/barrel.obj", asset_dir);
    if (!obj_load(&model_barrel, path)) return 0;
    return 1;
}

void build_all_display_lists(void) {
    obj_build_display_list(&model_cube);
    obj_build_display_list(&model_enemy);
    obj_build_display_list(&model_barrel);
}
