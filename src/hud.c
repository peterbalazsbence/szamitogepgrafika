#include "hud.h"
#include "font.h"
#include "player.h"
#include "enemy.h"
#include "lighting.h"

int show_help = 0;
int game_over = 0;
int game_win  = 0;
float game_time = 0;

void ortho_begin(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_W, 0, WINDOW_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}

void ortho_end(void) {
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    if(fog_enabled) glEnable(GL_FOG);
}

void fill_rect(float x, float y, float w, float h,
               float r, float g, float b, float a)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x,   y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x,   y+h);
    glEnd();
    glDisable(GL_BLEND);
}

void draw_doom_hud(void) {
    ortho_begin();

    float bar_h   = 60;
    float bar_y   = 0;
    float sec_w   = WINDOW_W/5.0f;
    float inset   = 4;

    /* Bar background + top border */
    fill_rect(0, bar_y, WINDOW_W, bar_h, 0.22f, 0.22f, 0.22f, 0.95f);
    fill_rect(0, bar_y+bar_h-2, WINDOW_W, 2, 0.5f, 0.5f, 0.5f, 1);
    /* Dividers between the 5 sections */
    for(int i=1;i<5;i++)
        fill_rect(sec_w*i - 1, bar_y, 2, bar_h, 0.1f, 0.1f, 0.1f, 1);

    /* ─── Section 1: AMMO for current weapon ─── */
    {
        float sx = 0;
        fill_rect(sx+inset, bar_y+inset, sec_w-inset*2, bar_h-inset*2-4,
                  0.12f, 0.12f, 0.12f, 1);
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+sec_w/2-30, bar_y+bar_h-16, 1.2f, "AMMO");

        int ca = 0;
        switch(player.weapon) {
            case WPN_FIST:    ca = -1; break;
            case WPN_PISTOL:  ca = player.ammo_pistol; break;
            case WPN_SHOTGUN: ca = player.ammo_shotgun; break;
        }
        char buf[16];
        if(ca < 0) snprintf(buf, sizeof(buf), "--");
        else       snprintf(buf, sizeof(buf), "%d", ca);
        glColor4f(0.9f, 0.2f, 0.2f, 1);
        draw_text(sx+sec_w/2-24, bar_y+12, 3.0f, buf);
    }

    /* ─── Section 2: HEALTH ─── */
    {
        float sx = sec_w;
        fill_rect(sx+inset, bar_y+inset, sec_w-inset*2, bar_h-inset*2-4,
                  0.12f, 0.12f, 0.12f, 1);
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+sec_w/2-38, bar_y+bar_h-16, 1.2f, "HEALTH");
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", player.health);
        glColor4f(0.9f, 0.2f, 0.2f, 1);
        draw_text(sx+sec_w/2-36, bar_y+12, 3.0f, buf);
    }

    /* ─── Section 3: ARMS (weapon slot indicators) ─── */
    {
        float sx = sec_w*2;
        fill_rect(sx+inset, bar_y+inset, sec_w-inset*2, bar_h-inset*2-4,
                  0.12f, 0.12f, 0.12f, 1);
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+sec_w/2-24, bar_y+bar_h-16, 1.2f, "ARMS");

        const char *wn[] = {"1","2","3"};
        for(int w=0;w<3;w++) {
            float bx  = sx + inset + 12 + w * (sec_w - inset*2 - 20)/3.0f;
            float by  = bar_y + 10;
            float bw  = (sec_w - inset*2 - 40) / 3.0f;
            float bhs = 30;
            int active = (w == (int)player.weapon);
            fill_rect(bx, by, bw, bhs,
                      active ? 0.35f : 0.18f,
                      active ? 0.30f : 0.18f,
                      active ? 0.20f : 0.18f, 1);
            glColor4f(active ? 1.0f : 0.5f,
                      active ? 0.9f : 0.45f,
                      active ? 0.2f : 0.35f, 1);
            draw_text(bx + bw/2 - 8, by + 8, 2.0f, wn[w]);
        }
    }

    /* ─── Section 4: ARMOR ─── */
    {
        float sx = sec_w*3;
        fill_rect(sx+inset, bar_y+inset, sec_w-inset*2, bar_h-inset*2-4,
                  0.12f, 0.12f, 0.12f, 1);
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+sec_w/2-34, bar_y+bar_h-16, 1.2f, "ARMOR");
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", player.armor);
        glColor4f(0.9f, 0.2f, 0.2f, 1);
        draw_text(sx+sec_w/2-36, bar_y+12, 3.0f, buf);
    }

    /* ─── Section 5: Pistol / Shotgun ammo breakdown ─── */
    {
        float sx = sec_w*4;
        fill_rect(sx+inset, bar_y+inset, sec_w-inset*2, bar_h-inset*2-4,
                  0.12f, 0.12f, 0.12f, 1);
        char buf[32];
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+10, bar_y+34, 1.2f, "PSTL");
        snprintf(buf, sizeof(buf), "%d", player.ammo_pistol);
        glColor4f(0.9f, 0.8f, 0.2f, 1);
        draw_text(sx+70, bar_y+32, 2.0f, buf);
        glColor4f(0.65f, 0.55f, 0.45f, 1);
        draw_text(sx+10, bar_y+10, 1.2f, "SHTG");
        snprintf(buf, sizeof(buf), "%d", player.ammo_shotgun);
        glColor4f(0.9f, 0.8f, 0.2f, 1);
        draw_text(sx+70, bar_y+8, 2.0f, buf);
    }

    /* ─── Weapon icon above the status bar (centered) ─── */
    {
        float icon_w = 80, icon_h = 50;
        float icon_x = WINDOW_W/2 - icon_w/2;
        float icon_y = bar_h + 8;

        fill_rect(icon_x-2, icon_y-2, icon_w+4, icon_h+4,
                  0.1f, 0.1f, 0.1f, 0.7f);
        fill_rect(icon_x, icon_y, icon_w, icon_h,
                  0.18f, 0.18f, 0.18f, 0.8f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        switch(player.weapon) {
            case WPN_FIST:
                /* Simple raised fist silhouette */
                fill_rect(icon_x+30, icon_y+4,  20, 14,
                          0.85f, 0.68f, 0.52f, 0.9f);
                fill_rect(icon_x+24, icon_y+16, 32, 28,
                          0.85f, 0.68f, 0.52f, 0.9f);
                fill_rect(icon_x+18, icon_y+16, 8,  16,
                          0.80f, 0.63f, 0.48f, 0.9f);
                break;

            case WPN_PISTOL:
                /* Side-profile pistol */
                fill_rect(icon_x+24, icon_y+30, 42, 10, 0.45f,0.45f,0.50f, 0.9f);
                fill_rect(icon_x+24, icon_y+31, 2, 8,   0.38f,0.38f,0.42f, 0.7f);
                fill_rect(icon_x+28, icon_y+31, 2, 8,   0.38f,0.38f,0.42f, 0.7f);
                fill_rect(icon_x+32, icon_y+31, 2, 8,   0.38f,0.38f,0.42f, 0.7f);
                fill_rect(icon_x+62, icon_y+32, 12, 6,  0.35f,0.35f,0.40f, 0.9f);
                fill_rect(icon_x+72, icon_y+33, 3, 4,   0.15f,0.15f,0.15f, 0.9f);
                fill_rect(icon_x+26, icon_y+22, 36, 8,  0.50f,0.50f,0.55f, 0.9f);
                fill_rect(icon_x+40, icon_y+14, 14, 8,  0.50f,0.50f,0.55f, 0.9f);
                fill_rect(icon_x+42, icon_y+16, 10, 4,  0.18f,0.18f,0.18f, 0.8f);
                fill_rect(icon_x+46, icon_y+18, 3, 6,   0.55f,0.55f,0.60f, 0.9f);
                fill_rect(icon_x+26, icon_y+6,  14, 18, 0.35f,0.28f,0.18f, 0.9f);
                fill_rect(icon_x+28, icon_y+8,  10, 1,  0.30f,0.23f,0.14f, 0.6f);
                fill_rect(icon_x+28, icon_y+11, 10, 1,  0.30f,0.23f,0.14f, 0.6f);
                fill_rect(icon_x+28, icon_y+14, 10, 1,  0.30f,0.23f,0.14f, 0.6f);
                fill_rect(icon_x+62, icon_y+40, 3, 4,   0.50f,0.50f,0.55f, 0.9f);
                fill_rect(icon_x+26, icon_y+40, 3, 3,   0.50f,0.50f,0.55f, 0.9f);
                fill_rect(icon_x+32, icon_y+40, 3, 3,   0.50f,0.50f,0.55f, 0.9f);
                break;

            case WPN_SHOTGUN:
                /* Double-barrel shotgun with stock and pump */
                fill_rect(icon_x+10, icon_y+30, 62, 6,  0.5f, 0.5f, 0.55f, 0.9f);
                fill_rect(icon_x+10, icon_y+24, 62, 6,  0.55f,0.55f,0.60f, 0.9f);
                fill_rect(icon_x+10, icon_y+18, 24, 14, 0.45f,0.45f,0.50f, 0.9f);
                fill_rect(icon_x+4,  icon_y+12, 14, 20, 0.45f,0.32f,0.18f, 0.9f);
                fill_rect(icon_x+22, icon_y+6,  10, 14, 0.4f, 0.3f, 0.18f, 0.9f);
                fill_rect(icon_x+40, icon_y+18, 16, 6,  0.4f, 0.3f, 0.2f,  0.9f);
                break;
        }

        /* Weapon name below the icon */
        const char *wpn_label[] = { "FIST", "PISTOL", "SHOTGUN" };
        glColor4f(0.9f, 0.85f, 0.6f, 0.9f);
        float lbl_x = icon_x + icon_w/2
                      - (float)strlen(wpn_label[player.weapon])*1.2f*9/2;
        draw_text(lbl_x, icon_y-14, 1.2f, wpn_label[player.weapon]);

        glDisable(GL_BLEND);
    }

    /* ─── Crosshair ─── */
    int cx = WINDOW_W/2, cy = WINDOW_H/2;
    fill_rect(cx-12, cy-2,  10, 4,  1, 1, 1, 0.8f);
    fill_rect(cx+2,  cy-2,  10, 4,  1, 1, 1, 0.8f);
    fill_rect(cx-2,  cy-12, 4,  10, 1, 1, 1, 0.8f);
    fill_rect(cx-2,  cy+2,  4,  10, 1, 1, 1, 0.8f);

    /* Muzzle flash */
    if(player.muzzle_flash > 0) {
        float a = player.muzzle_flash / MUZZLE_TIME;
        fill_rect(0, 0, WINDOW_W, WINDOW_H, 1, 0.8f, 0, 0.15f*a);
    }

    /* Damage vignette (red screen edges) */
    if(player.damage_flash > 0) {
        float a = (player.damage_flash / 0.3f) * 0.45f;
        int vw = 80;
        fill_rect(0,          0,         vw, WINDOW_H,  0.8f, 0, 0, a);
        fill_rect(WINDOW_W-vw, 0,        vw, WINDOW_H,  0.8f, 0, 0, a);
        fill_rect(0, WINDOW_H-vw,        WINDOW_W, vw,  0.8f, 0, 0, a*0.7f);
        fill_rect(0, bar_h,               WINDOW_W, vw,  0.8f, 0, 0, a*0.7f);
    }

    /* Status line at bottom-left */
    glColor4f(1, 1, 0.6f, 0.5f);
    char lbuf[80];
    const char *cnames[] = { "WHITE", "RED", "BLUE" };
    snprintf(lbuf, sizeof(lbuf), "LIGHT:%.0f%% %s %s  FOG:%s %.0f%%",
             light_brightness*100,
             cnames[light_color_mode],
             flashlight_on ? "FLASH:ON" : "FLASH:OFF",
             fog_enabled   ? "ON" : "OFF",
             fog_density*1000);
    draw_text(10, WINDOW_H-20, 1.3f, lbuf);

    /* F1 hint */
    glColor4f(0.6f, 0.6f, 0.6f, 0.4f);
    draw_text(WINDOW_W-160, WINDOW_H-20, 1.3f, "F1: HELP");

    /* Enemy counter */
    int alive = 0;
    for(int i=0;i<enemy_count;i++) alive += enemies[i].alive;
    glColor4f(1, 0.4f, 0.4f, 0.7f);
    char ebuf[32];
    snprintf(ebuf, sizeof(ebuf), "ENEMIES: %d", alive);
    draw_text(10, WINDOW_H-40, 1.5f, ebuf);

    /* Game over / win overlays */
    if(game_over) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-50, 400, 100,
                  0.8f, 0.05f, 0.05f, 0.85f);
        glColor4f(1, 1, 1, 1);
        draw_text(WINDOW_W/2-100, WINDOW_H/2-10, 3.0f, "YOU DIED");
        glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
        draw_text(WINDOW_W/2-120, WINDOW_H/2-35, 1.5f, "PRESS R TO RESTART");
    }
    if(game_win) {
        fill_rect(WINDOW_W/2-200, WINDOW_H/2-50, 400, 100,
                  0.05f, 0.6f, 0.05f, 0.85f);
        glColor4f(1, 1, 1, 1);
        draw_text(WINDOW_W/2-100, WINDOW_H/2-10, 3.0f, "YOU WIN!");
        glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
        draw_text(WINDOW_W/2-120, WINDOW_H/2-35, 1.5f, "PRESS R TO RESTART");
    }

    ortho_end();
}

void draw_help_screen(void) {
    ortho_begin();
    fill_rect(0, 0, WINDOW_W, WINDOW_H, 0, 0, 0, 0.75f);

    float px = 140, py = 30, pw = WINDOW_W-280, ph = WINDOW_H-60;
    fill_rect(px, py, pw, ph, 0.1f, 0.1f, 0.15f, 0.95f);
    fill_rect(px+2, py+2, pw-4, ph-4, 0.15f, 0.15f, 0.2f, 0.95f);

    float tx = px + 25, ty = py + ph - 40;
    float lh = 24, sz = 1.7f;

    glColor4f(1, 0.4f, 0.3f, 1);
    draw_text(tx, ty, 2.5f, "DOOM3D - HELP"); ty -= lh*1.4f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "MOVEMENT:"); ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "W S A D  - MOVE    MOUSE - LOOK"); ty -= lh;
    draw_text(tx, ty, sz, "SPACE - JUMP   L.CLICK - SHOOT"); ty -= lh*1.2f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "WEAPONS:"); ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "1 - FIST   2 - PISTOL   3 - SHOTGUN"); ty -= lh*1.2f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "LIGHTING:"); ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "+/- BRIGHTNESS   L - COLOR   F - FLASH"); ty -= lh;
    draw_text(tx, ty, sz, "ARROWS - MOVE SCENE LIGHT"); ty -= lh*1.2f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "FOG:"); ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "G - TOGGLE FOG   [ ] - FOG DENSITY"); ty -= lh*1.2f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "PICKUPS:"); ty -= lh;
    glColor4f(0.5f, 1, 0.5f, 0.9f);
    draw_text(tx, ty, sz, "GREEN  - HEALTH (+25)"); ty -= lh;
    glColor4f(1, 0.9f, 0.4f, 0.9f);
    draw_text(tx, ty, sz, "YELLOW - AMMO (+15P +4S)"); ty -= lh;
    glColor4f(0.4f, 0.6f, 1, 0.9f);
    draw_text(tx, ty, sz, "BLUE   - ARMOR (+25)"); ty -= lh*1.2f;

    glColor4f(1, 0.9f, 0.5f, 1);
    draw_text(tx, ty, sz, "FEATURES:"); ty -= lh;
    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "FOG, PARTICLES (FIRE+BLOOD),"); ty -= lh;
    draw_text(tx, ty, sz, "TRANSPARENCY (GLASS WINDOWS),"); ty -= lh;
    draw_text(tx, ty, sz, "SHADOWS (PROJECTED GEOMETRY)"); ty -= lh*1.2f;

    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    draw_text(tx, ty, sz, "F1 - HELP   R - RESTART   ESC - QUIT");

    ortho_end();
}
