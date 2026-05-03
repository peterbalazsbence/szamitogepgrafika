/*
 * DOOM3D - A Doom-like 3D FPS
 * Built with C, SDL2, and OpenGL 2.1 (fixed-function pipeline)
 *
 * See README.md for full project description and controls.
 */

#include "common.h"
#include "obj_loader.h"
#include "level.h"
#include "textures.h"
#include "particles.h"
#include "enemy.h"
#include "decoration.h"
#include "pickup.h"
#include "player.h"
#include "lighting.h"
#include "rendering.h"
#include "shadows.h"
#include "hud.h"
#include "font.h"

/* Load everything for a fresh level */
static void init_level(void) {
    enemies_reset();
    decorations_reset();
    pickups_reset();
    particles_reset();

    /* Walk the map and spawn entities */
    for(int r=0;r<MAP_ROWS_N;r++)
    for(int c=0;c<MAP_COLS;c++) {
        char ch = MAP_DATA[r][c];
        if(ch == 'P') {
            player_reset();
            player.pos = v3(c+0.5f, PLAYER_HEIGHT, r+0.5f);
        } else if(ch == 'E') enemy_spawn(c+0.5f, r+0.5f);
          else if(ch == 'B') deco_spawn(c+0.5f, r+0.5f, DECO_BARREL);
          else if(ch == 'C') deco_spawn(c+0.5f, r+0.5f, DECO_CRATE);
    }
    scatter_pickups(PICKUP_COUNT_HEALTH,
                    PICKUP_COUNT_AMMO,
                    PICKUP_COUNT_ARMOR);
}

/* Per-frame update (when not paused by help screen) */
static void update(float dt) {
    if(ui.game_over || ui.game_win) return;
    ui.game_time += dt;

    player_update(dt);
    enemies_update(dt);

    /* Barrels emit fire/smoke */
    for(int i=0;i<deco_count;i++) {
        if(decorations[i].type == DECO_BARREL)
            emit_barrel_fire(decorations[i].pos, dt);
    }
    particles_update(dt);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    const char *asset_dir = "assets";

    /* ─── SDL & GL context setup ─── */
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); /* for shadow stencil */

    SDL_Window *win = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if(!ctx) {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetSwapInterval(1);           /* vsync */
    SDL_SetRelativeMouseMode(SDL_TRUE);  /* capture mouse for FPS look */

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.15f, 0.15f, 0.18f, 1);
    glShadeModel(GL_SMOOTH);

    /* ─── Load all data from assets/ ─── */
    if(!load_all_models(asset_dir)) {
        fprintf(stderr, "Failed to load models from '%s/'\n", asset_dir);
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    if(!load_map_from_file("assets/map.txt")) {
        fprintf(stderr, "Failed to load map\n");
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    if(!load_font_from_csv("assets/font.csv")) {
        fprintf(stderr, "Failed to load font\n");
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    init_textures();
    build_all_display_lists();
    srand((unsigned)time(NULL));
    init_level();
    set_projection();

    /* ─── Main loop ─── */
    Uint32 prev = SDL_GetTicks();
    int running = 1;

    while(running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.0f;
        if(dt > 0.05f) { dt = 0.05f; } /* cap dt to avoid teleports */
        prev = now;

        /* ── Event handling ── */
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type == SDL_QUIT) running = 0;

            if(ev.type == SDL_KEYDOWN) {
                switch(ev.key.keysym.sym) {
                    case SDLK_ESCAPE: running = 0; break;
                    case SDLK_F1:     ui.show_help = !ui.show_help; break;
                    case SDLK_f:      light.flashlight_on = !light.flashlight_on; break;
                    case SDLK_l:      light.color_mode = (light.color_mode + 1) % 3; break;
                    case SDLK_g:      light.fog_enabled = !light.fog_enabled; break;
                    case SDLK_1:      player.weapon = WPN_FIST; break;
                    case SDLK_2:      player.weapon = WPN_PISTOL; break;
                    case SDLK_3:      player.weapon = WPN_SHOTGUN; break;
                    case SDLK_LEFTBRACKET:
                        light.fog_density -= FOG_DENSITY_STEP;
                        if(light.fog_density < FOG_DENSITY_MIN) { light.fog_density = FOG_DENSITY_MIN; }
                        break;
                    case SDLK_RIGHTBRACKET:
                        light.fog_density += FOG_DENSITY_STEP;
                        if(light.fog_density > FOG_DENSITY_MAX) { light.fog_density = FOG_DENSITY_MAX; }
                        break;
                    case SDLK_EQUALS: case SDLK_PLUS: case SDLK_KP_PLUS:
                        light.brightness += LIGHT_STEP;
                        if(light.brightness > LIGHT_MAX) { light.brightness = LIGHT_MAX; }
                        break;
                    case SDLK_MINUS: case SDLK_KP_MINUS:
                        light.brightness -= LIGHT_STEP;
                        if(light.brightness < LIGHT_MIN) { light.brightness = LIGHT_MIN; }
                        break;
                    case SDLK_r:
                        if(ui.game_over || ui.game_win) {
                            ui.game_over = 0; ui.game_win = 0;
                            init_level();
                        }
                        break;
                    default: break;
                }
            }

            /* Mouse look (only while gameplay is active) */
            if(ev.type == SDL_MOUSEMOTION
               && !ui.game_over && !ui.game_win && !ui.show_help) {
                player.yaw   -= ev.motion.xrel * TURN_SPEED;
                player.pitch -= ev.motion.yrel * TURN_SPEED;
                if(player.pitch >  1.4f) player.pitch =  1.4f;
                if(player.pitch < -1.4f) player.pitch = -1.4f;
            }
            if(ev.type == SDL_MOUSEBUTTONDOWN
               && ev.button.button == SDL_BUTTON_LEFT
               && !ui.show_help) {
                player_shoot();
            }
        }

        /* ── Update world (pause while help screen is open) ── */
        if(!ui.show_help) update(dt);

        /* ── Render ── */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        set_camera();
        setup_lighting();
        setup_fog();

        /* Render order matters:
         *   1. Opaque world geometry (walls, floor, ceiling)
         *   2. Shadows (alpha blended on floor)
         *   3. Opaque 3D objects (decorations, pickups, enemies)
         *   4. Scene-light marker
         *   5. Transparent windows (depth-write off)
         *   6. Particles (billboards, additive/alpha blended)
         *   7. 2D HUD in orthographic mode
         */
        draw_level();
        draw_shadows();
        draw_decorations();
        draw_pickups();
        draw_enemies();
        draw_light_marker();
        draw_windows();
        particles_draw();
        draw_doom_hud();

        if(ui.show_help) draw_help_screen();

        SDL_GL_SwapWindow(win);
    }

    /* ─── Cleanup ─── */
    glDeleteLists(model_cube.display_list,   1);
    glDeleteLists(model_enemy.display_list,  1);
    glDeleteLists(model_barrel.display_list, 1);
    free_textures();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
