/* sprite_type_grapple.c
 * The grippy end of the grapple, and we also render the decorative chain.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "game/session/session.h"
#include "game/session/map.h"
#include "sprite.h"

#define GRAPPLE_SPEED 12.0
#define GRAPPLE_TTL 1.000 /* Coordinate with speed to arrange the maximum reach. */
#define GRAPPLE_RETURN_SPEED -0.75 /* Relative to outbound speed. Must be negative. */
#define GRAPPLE_PULL_SPEED 0.75 /* Relative to outbound speed. Must be positive. */
#define GRAPPLE_LINKC 20

#define GRAPPLE_PHASE_OUT 0 /* Flying outward (dx,dy) */
#define GRAPPLE_PHASE_RETURN 1 /* Caught something moveable, returning to the handle. */
#define GRAPPLE_PHASE_LOCK 2 /* Caught something immoveable, dragging hero. */

struct sprite_grapple {
  struct sprite hdr;
  double dx,dy;
  double handlex,handley;
  double ttl;
  int phase;
  struct sprite *pumpkin; // Hero in LOCK phase, or the fetched item in RETURN phase. WEAK, check residence before deref.
};

#define SPRITE ((struct sprite_grapple*)sprite)

static int _grapple_init(struct sprite *sprite) {
  sprite->imageid=RID_image_sprites1;
  sprite->tileid=0x25; // Grip end. Chain is n-1, and Handle is not our problem, hero renders it.
  SPRITE->handlex=sprite->x;
  SPRITE->handley=sprite->y;
  SPRITE->ttl=GRAPPLE_TTL;
  SPRITE->phase=GRAPPLE_PHASE_OUT;
  return 0;
}

// During OUT phase, look for things we can grapple.
static void grapple_check_world(struct sprite *sprite) {
  struct sprite **p;
  int c=sprites_get_all(&p,sprite->owner);
  for (;c-->0;p++) {
    struct sprite *pumpkin=*p;
    if (!pumpkin->grapplable) continue; // The grapple itself is not grapplable, and we depend on that.
    double dx=sprite->x-pumpkin->x;
    if ((dx<=-0.5)||(dx>=0.5)) continue;
    double dy=sprite->y-pumpkin->y;
    if ((dy<=-0.5)||(dy>=0.5)) continue;
    egg_play_sound(RID_sound_grapple_catch);
    SPRITE->phase=GRAPPLE_PHASE_RETURN;
    SPRITE->pumpkin=pumpkin;
    if (pumpkin->type->paralyze) pumpkin->type->paralyze(pumpkin,sprite);
    return;
  }
  int col=(int)sprite->x,row=(int)sprite->y;
  if ((col>=0)&&(row>=0)&&(col<g.world->map->w)&&(row<g.world->map->h)) {
    int cellp=row*g.world->map->w+col;
    uint8_t tileid=g.world->map->v[cellp];
    uint8_t physics=g.world->map->physics[tileid];
    switch (physics) {
      case NS_physics_grapplable: {
          egg_play_sound(RID_sound_grapple_pull);
          SPRITE->phase=GRAPPLE_PHASE_LOCK;
          SPRITE->pumpkin=sprites_get_hero(sprite->owner);
          sprite_hero_engage_grapple(SPRITE->pumpkin);
        } return;
      case NS_physics_solid: {
          egg_play_sound(RID_sound_grapple_reject);
          sprite_kill_soon(sprite);
        } return;
    }
  }
}

static void _grapple_update(struct sprite *sprite,double elapsed) {
  switch (SPRITE->phase) {
  
    case GRAPPLE_PHASE_OUT: {
        if ((SPRITE->ttl-=elapsed)<=0.0) {
          egg_play_sound(RID_sound_grapple_miss);
          sprite_kill_soon(sprite);
          return;
        }
        sprite->x+=SPRITE->dx*elapsed;
        sprite->y+=SPRITE->dy*elapsed;
        grapple_check_world(sprite);
      } break;
      
    case GRAPPLE_PHASE_RETURN: {
        if (!sprite_is_resident(sprite->owner,SPRITE->pumpkin)) SPRITE->pumpkin=0;
        if (!SPRITE->pumpkin) { sprite_kill_soon(sprite); return; }
        double dx=SPRITE->dx*elapsed*GRAPPLE_RETURN_SPEED;
        double dy=SPRITE->dy*elapsed*GRAPPLE_RETURN_SPEED;
        sprite->x+=dx;
        sprite->y+=dy;
        if (
          ((SPRITE->dx>0.0)&&(sprite->x>SPRITE->handlex))||
          ((SPRITE->dx<0.0)&&(sprite->x<SPRITE->handlex))||
          ((SPRITE->dy>0.0)&&(sprite->y>SPRITE->handley))||
          ((SPRITE->dy<0.0)&&(sprite->y<SPRITE->handley))
        ) {
          SPRITE->pumpkin->x+=dx;
          SPRITE->pumpkin->y+=dy;
          return;
        }
        if (SPRITE->pumpkin) {
          if ((SPRITE->pumpkin->type==&sprite_type_item)||(SPRITE->pumpkin->type==&sprite_type_faun)) {
            uint8_t itemid=SPRITE->pumpkin->arg>>24; // True for both types.
            if (session_acquire_item(g.session,itemid,0,"passive")>0) {
              sprite_kill_soon(SPRITE->pumpkin);
              SPRITE->pumpkin=0;
            }
          }
          if (SPRITE->pumpkin&&SPRITE->pumpkin->type->paralyze) {
            SPRITE->pumpkin->type->paralyze(SPRITE->pumpkin,0);
          }
        }
        sprite_kill_soon(sprite);
      } break;
      
    case GRAPPLE_PHASE_LOCK: {
        if (!sprite_is_resident(sprite->owner,SPRITE->pumpkin)) SPRITE->pumpkin=0;
        if (!SPRITE->pumpkin) { sprite_kill_soon(sprite); return; }
        double dx=SPRITE->dx*elapsed*GRAPPLE_PULL_SPEED;
        double dy=SPRITE->dy*elapsed*GRAPPLE_PULL_SPEED;
        SPRITE->handlex+=dx;
        SPRITE->handley+=dy;
        if (
          ((SPRITE->dx>0.0)&&(sprite->x>SPRITE->handlex))||
          ((SPRITE->dx<0.0)&&(sprite->x<SPRITE->handlex))||
          ((SPRITE->dy>0.0)&&(sprite->y>SPRITE->handley))||
          ((SPRITE->dy<0.0)&&(sprite->y<SPRITE->handley))
        ) {
          SPRITE->pumpkin->x+=dx;
          SPRITE->pumpkin->y+=dy;
          return;
        }
        sprite_hero_release_grapple(SPRITE->pumpkin);
        sprite_kill_soon(sprite);
      } break;
  }
}

static void _grapple_render(struct sprite *sprite,int16_t x,int16_t y) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  double dt=1.0/(GRAPPLE_LINKC+1);
  double t=dt;
  int i=GRAPPLE_LINKC;
  for (;i-->0;t+=dt) {
    double x=sprite->x*t+SPRITE->handlex*(1.0-t);
    double y=sprite->y*t+SPRITE->handley*(1.0-t);
    graf_draw_tile(&g.graf,texid,
      (int16_t)(x*NS_sys_tilesize)-g.world->viewx,
      (int16_t)(y*NS_sys_tilesize)-g.world->viewy,
      sprite->tileid-1,0 // Link tile is a circle; xform doesn't matter.
    );
  }
  graf_draw_tile(&g.graf,texid,x,y,sprite->tileid,sprite->xform);
}

const struct sprite_type sprite_type_grapple={
  .name="grapple",
  .objlen=sizeof(struct sprite_grapple),
  .init=_grapple_init,
  .update=_grapple_update,
  .render=_grapple_render,
};

void sprite_grapple_setup(struct sprite *sprite,int dx,int dy) {
  if (!sprite||(sprite->type!=&sprite_type_grapple)) return;
  if (!dx&&!dy) return;
  if (dx&&dy) return;
  SPRITE->dx=dx*GRAPPLE_SPEED;
  SPRITE->dy=dy*GRAPPLE_SPEED;
  if (dx<0) sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_YREV;
  else if (dx>0) sprite->xform=EGG_XFORM_SWAP;
  else if (dy<0) sprite->xform=EGG_XFORM_YREV;
}

void sprite_grapple_drop_all(struct sprites *sprites) {
  struct sprite **p;
  int c=sprites_get_all(&p,sprites);
  for (;c-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type!=&sprite_type_grapple) continue;
    if (SPRITE->pumpkin&&sprite_is_resident(sprite->owner,SPRITE->pumpkin)&&SPRITE->pumpkin->type->paralyze) {
      SPRITE->pumpkin->type->paralyze(SPRITE->pumpkin,0);
    }
    sprite_kill_soon(sprite);
  }
}
