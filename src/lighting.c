#include "lighting.h"
#include "math3d.h"
#include "player.h"
#include "hud.h"

float light_brightness = 1.0f;
int   light_color_mode = 0;    /* 0=white 1=red 2=blue */
int   flashlight_on   = 1;
float ambient_light_pos[4] = {20.0f, 2.5f, 20.0f, 1.0f};

int   fog_enabled  = 1;
float fog_density  = 0.18f;

void setup_lighting(void) {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* Color tint per mode */
    float r=1, g=1, b=1;
    switch(light_color_mode) {
        case 1: r=1.0f;  g=0.3f; b=0.2f; break;
        case 2: r=0.3f;  g=0.4f; b=1.0f; break;
        default: break;
    }
    r *= light_brightness;
    g *= light_brightness;
    b *= light_brightness;

    /* GL_LIGHT0: positional scene light (movable with arrows) */
    glEnable(GL_LIGHT0);
    float d0[] = {r*0.8f, g*0.8f, b*0.8f, 1};
    float a0[] = {r*0.15f, g*0.15f, b*0.15f, 1};
    float s0[] = {0.5f, 0.5f, 0.5f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, ambient_light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  d0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  a0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, s0);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.3f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.05f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

    /* GL_LIGHT1: spotlight attached to player camera (flashlight) */
    if(flashlight_on) {
        glEnable(GL_LIGHT1);
        Vec3 fwd = player_forward();
        float fp[]   = {player.pos.x, player.pos.y, player.pos.z, 1};
        float fd[]   = {fwd.x, fwd.y, fwd.z};
        float fdif[] = {0.9f*light_brightness, 0.85f*light_brightness,
                        0.7f*light_brightness, 1};
        float fa[]   = {0, 0, 0, 1};
        glLightfv(GL_LIGHT1, GL_POSITION,       fp);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, fd);
        glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,    25);
        glLightf (GL_LIGHT1, GL_SPOT_EXPONENT,  30);
        glLightfv(GL_LIGHT1, GL_DIFFUSE,        fdif);
        glLightfv(GL_LIGHT1, GL_AMBIENT,        fa);
        glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  0.5f);
        glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.05f);
        glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);
    } else {
        glDisable(GL_LIGHT1);
    }

    /* Global ambient so dungeon is not pitch black away from the lights */
    float ga[] = {0.25f, 0.25f, 0.28f, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ga);
}

void setup_fog(void) {
    if(fog_enabled) {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_EXP2); /* density-based exponential-squared */
        float fog_color[] = {0.15f, 0.15f, 0.18f, 1.0f};
        glFogfv(GL_FOG_COLOR, fog_color);
        glFogf (GL_FOG_DENSITY, fog_density);
        glHint (GL_FOG_HINT, GL_NICEST);
    } else {
        glDisable(GL_FOG);
    }
}

void draw_light_marker(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    /* Pulsing color matches current light color mode */
    float pulse = 0.7f + 0.3f*sinf(game_time*4);
    switch(light_color_mode) {
        case 1:  glColor3f(pulse, 0.3f*pulse, 0.1f*pulse); break;
        case 2:  glColor3f(0.2f*pulse, 0.3f*pulse, pulse); break;
        default: glColor3f(pulse, pulse, 0.8f*pulse); break;
    }

    glPushMatrix();
    glTranslatef(ambient_light_pos[0], ambient_light_pos[1], ambient_light_pos[2]);

    /* 8-sided diamond (bipyramid) */
    float s = 0.15f;
    glBegin(GL_TRIANGLES);
    /* Top half */
    glVertex3f(0,s*2,0);  glVertex3f(-s,0,-s); glVertex3f( s,0,-s);
    glVertex3f(0,s*2,0);  glVertex3f( s,0,-s); glVertex3f( s,0, s);
    glVertex3f(0,s*2,0);  glVertex3f( s,0, s); glVertex3f(-s,0, s);
    glVertex3f(0,s*2,0);  glVertex3f(-s,0, s); glVertex3f(-s,0,-s);
    /* Bottom half */
    glVertex3f(0,-s*2,0); glVertex3f( s,0,-s); glVertex3f(-s,0,-s);
    glVertex3f(0,-s*2,0); glVertex3f( s,0, s); glVertex3f( s,0,-s);
    glVertex3f(0,-s*2,0); glVertex3f(-s,0, s); glVertex3f( s,0, s);
    glVertex3f(0,-s*2,0); glVertex3f(-s,0,-s); glVertex3f(-s,0, s);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}
