/* sprite_type_arrow.c
 * arg&3: Direction: 0,1,2,3=left,right,up,down
 * arg&4: Stone, else Arrow.
 * Our nominal position is the position of the shadow. The arrow itself is 0.5 m higher initially. XXX That's not working. Nominal position must be the object itself.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "game/session/map.h"
#include "sprite.h"

#define ARROW_SPEED 8.0 /* m/s, gets baked into (dx,dy) */
#define ARROW_ARROW_SPEED 1.25 /* Speed of actual arrow, relative to stone. */
#define ARROW_TTL 1.0
#define ARROW_FADE_TIME 0.125
#define ARROW_GRAVITY -15.0 /* m/s**2 */

struct sprite_arrow {
  struct sprite hdr;
  double dx,dy; // m/s, does not include elevation
  double ttl; // s
  double el; // elevation shadow to arrow, m
  double del;
};

#define SPRITE ((struct sprite_arrow*)sprite)

static int _arrow_init(struct sprite *sprite) {
  if (!sprite->imageid) {
    sprite->imageid=RID_image_sprites1;
    sprite->tileid=(sprite->arg&4)?0x4b:0x41;
  }
  switch (sprite->arg&3) {
    case 0: SPRITE->dx=-ARROW_SPEED; sprite->xform=0; break;
    case 1: SPRITE->dx= ARROW_SPEED; sprite->xform=EGG_XFORM_XREV; break;
    case 2: SPRITE->dy=-ARROW_SPEED; sprite->xform=EGG_XFORM_SWAP; break;
    case 3: SPRITE->dy= ARROW_SPEED; sprite->xform=EGG_XFORM_XREV|EGG_XFORM_SWAP; break;
  }
  if (!(sprite->arg&4)) {
    SPRITE->dx*=ARROW_ARROW_SPEED;
    SPRITE->dy*=ARROW_ARROW_SPEED;
    sprite->radius=0.250; // for wall collisions only
  } else {
    sprite->radius=0.250; // for wall collisions only
  }
  sprite->solidmask=(1<<NS_physics_solid)|(1<<NS_physics_grapplable);
  SPRITE->ttl=ARROW_TTL;
  SPRITE->el=0.5;
  SPRITE->del=4.0;
  return 0;
}

static int arrow_on_water(struct sprite *sprite) {
  int col=(int)sprite->x;
  int row=(int)sprite->y;
  if ((col<0)||(row<0)||(col>=g.world->map->w)||(row>=g.world->map->h)) return 0;
  uint8_t tileid=g.world->map->v[row*g.world->map->w+col];
  uint8_t physics=g.world->map->physics[tileid];
  if (physics==NS_physics_water) return 1;
  return 0;
}

static int arrow_blocked_in_business_direction(struct sprite *sprite,double x0,double y0,double elapsed) {
  double trouble=0.0;
  if (SPRITE->dx<0.0) {
    trouble=sprite->x-x0;
  } else if (SPRITE->dx>0.0) {
    trouble=x0-sprite->x;
  } else if (SPRITE->dy<0.0) {
    trouble=sprite->y-y0;
  } else if (SPRITE->dy>0.0) {
    trouble=y0-sprite->y;
  }
  trouble/=elapsed;
  //fprintf(stderr,"Feedback velocity: %.03f m/s\n",trouble);
  if (trouble>=4.0) return 1;
  return 0;
}

static void _arrow_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    if (sprite->arg&4) {
      // You can pick up the stone again and reuse it. The tile used by item sprites is oriented toward the bottom of the cell, hence (y-something).
      // But also, be careful where we put it: It has to remain pickable by the hero. Had a problem at first with its center sticking inside the wall.
      struct sprite *itspr=sprite_new(0,sprite->owner,sprite->x,sprite->y-0.200,NS_item_stone<<24,RID_sprite_item);
      if (itspr) {
        sprite_rectify_physics(itspr,0.5);
      }
    }
    sprite_kill_soon(sprite);
    return;
  }
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  sprite->y-=SPRITE->del*elapsed;
  SPRITE->el+=SPRITE->del*elapsed;
  SPRITE->del+=ARROW_GRAVITY*elapsed;
  double x0=sprite->x,y0=sprite->y;
  if (SPRITE->el<0.0) {
    SPRITE->el=0.0;
    if (SPRITE->del<0.0) { // bounce
      if (arrow_on_water(sprite)) {
        sauces_splash(sprite->x,sprite->y);
        sprite_kill_soon(sprite);
      } else {
        egg_play_sound(RID_sound_bump);
        SPRITE->del*=-0.5;
      }
    }
  } else if (sprite_rectify_physics(sprite,0.5)) {
    if (arrow_blocked_in_business_direction(sprite,x0,y0,elapsed)) {
      egg_play_sound(RID_sound_bump);
      if (sprite->arg&4) {
        SPRITE->ttl=0.0;
      } else {
        sprite_kill_soon(sprite);
      }
    }
  }
}

static void _arrow_render(struct sprite *sprite,int16_t x,int16_t y) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  if ((SPRITE->ttl<ARROW_FADE_TIME)&&!(sprite->arg&4)) { // Arrows fade out but stones don't.
    int alpha=(SPRITE->ttl*255.0)/ARROW_FADE_TIME;
    if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
  }
  graf_draw_tile(&g.graf,texid,x,y+(int)(SPRITE->el*NS_sys_tilesize),sprite->tileid+1,sprite->xform);
  graf_draw_tile(&g.graf,texid,x,y,sprite->tileid,sprite->xform);
  graf_set_alpha(&g.graf,0xff);
}

const struct sprite_type sprite_type_arrow={
  .name="arrow",
  .objlen=sizeof(struct sprite_arrow),
  .init=_arrow_init,
  .update=_arrow_update,
  .render=_arrow_render,
};
