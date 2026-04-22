#include "particles.h"

Particle particles[MAX_PARTICLES];
int particle_count = 0;

void particle_emit(Vec3 pos, Vec3 vel, float life, float size,
                   float r, float g, float b, float a, ParticleType type)
{
    /* Find an inactive slot first */
    int slot = -1;
    for(int i=0;i<particle_count;i++) {
        if(!particles[i].active) { slot=i; break; }
    }
    if(slot<0) {
        if(particle_count<MAX_PARTICLES) {
            slot=particle_count++;
        } else {
            /* Overwrite particle with least life remaining */
            float min_life = 999;
            slot = 0;
            for(int i=0;i<MAX_PARTICLES;i++) {
                if(particles[i].life < min_life) {
                    min_life = particles[i].life;
                    slot = i;
                }
            }
        }
    }
    Particle *p = &particles[slot];
    p->pos = pos; p->vel = vel;
    p->life = life; p->max_life = life;
    p->size = size;
    p->r = r; p->g = g; p->b = b; p->a = a;
    p->type = type; p->active = 1;
}

void particles_update(float dt) {
    for(int i=0;i<particle_count;i++) {
        Particle *p = &particles[i];
        if(!p->active) continue;
        p->life -= dt;
        if(p->life <= 0) { p->active = 0; continue; }

        /* Physics integration */
        p->pos = v3add(p->pos, v3scale(p->vel, dt));

        switch(p->type) {
            case PART_FIRE:
                p->vel.y += 1.5f*dt; /* rise faster */
                p->a = (p->life/p->max_life) * 0.8f;
                /* Yellow → red as it ages */
                p->r = 1.0f;
                p->g = 0.3f + 0.7f * (p->life/p->max_life);
                p->b = 0.1f * (p->life/p->max_life);
                p->size *= (1.0f + 0.5f*dt);
                break;
            case PART_SMOKE:
                p->vel.y += 0.5f*dt;
                p->a = (p->life/p->max_life) * 0.4f;
                p->size *= (1.0f + 1.0f*dt);
                break;
            case PART_BLOOD:
                p->vel.y -= 9.8f*dt; /* gravity */
                p->a = (p->life/p->max_life) * 0.9f;
                if(p->pos.y < FLOOR_Y + 0.05f) {
                    p->pos.y = FLOOR_Y + 0.05f;
                    p->vel = v3(0,0,0);
                    p->life -= dt*2; /* fade faster on ground */
                }
                break;
        }
    }
}

void particles_draw(void) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    /* Camera-facing billboards - read right/up vectors from modelview */
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    Vec3 cam_right = v3(mv[0], mv[4], mv[8]);
    Vec3 cam_up    = v3(mv[1], mv[5], mv[9]);

    for(int i=0;i<particle_count;i++) {
        Particle *p = &particles[i];
        if(!p->active) continue;

        float s = p->size * 0.5f;
        Vec3 pos = p->pos;

        /* Additive blending for fire (glow), standard for others */
        if(p->type == PART_FIRE)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        else
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void emit_barrel_fire(Vec3 pos, float dt) {
    static float accum = 0;
    accum += dt;
    if(accum < 0.03f) return;
    accum = 0;

    /* Fire particle */
    Vec3 fire_pos = v3(pos.x + randf_range(-0.15f, 0.15f),
                       pos.y + 0.7f,
                       pos.z + randf_range(-0.15f, 0.15f));
    Vec3 fire_vel = v3(randf_range(-0.3f, 0.3f),
                       randf_range( 0.8f, 1.8f),
                       randf_range(-0.3f, 0.3f));
    particle_emit(fire_pos, fire_vel,
                  randf_range(0.3f, 0.7f),
                  randf_range(0.08f, 0.15f),
                  1.0f, 0.8f, 0.2f, 0.8f, PART_FIRE);

    /* Occasional smoke puff */
    if(randf() < 0.3f) {
        Vec3 smoke_pos = v3(pos.x+randf_range(-0.1f,0.1f),
                            pos.y+1.0f,
                            pos.z+randf_range(-0.1f,0.1f));
        Vec3 smoke_vel = v3(randf_range(-0.2f,0.2f),
                            randf_range( 0.3f,0.8f),
                            randf_range(-0.2f,0.2f));
        particle_emit(smoke_pos, smoke_vel,
                      randf_range(0.8f, 1.5f),
                      randf_range(0.1f, 0.2f),
                      0.4f, 0.4f, 0.4f, 0.4f, PART_SMOKE);
    }
}

void emit_blood(Vec3 pos) {
    int count = 8 + rand()%8;
    for(int i=0;i<count;i++) {
        Vec3 vel = v3(randf_range(-3.0f, 3.0f),
                      randf_range( 1.0f, 4.0f),
                      randf_range(-3.0f, 3.0f));
        float size = randf_range(0.04f, 0.10f);
        particle_emit(pos, vel,
                      randf_range(0.5f, 1.2f), size,
                      0.8f, 0.05f, 0.05f, 0.9f, PART_BLOOD);
    }
}

void particles_reset(void) {
    particle_count = 0;
    for(int i=0;i<MAX_PARTICLES;i++) particles[i].active = 0;
}
