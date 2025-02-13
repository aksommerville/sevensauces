/* sprite_type_arrow.c
 * arg&3: Direction: 0,1,2,3=left,right,up,down
 * arg&4: Stone, else Arrow.
 * Our nominal position is the position of the shadow. The arrow itself is 0.5 m higher initially.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "sprite.h"

#define ARROW_SPEED 8.0 /* m/s, gets baked into (dx,dy) */
#define ARROW_TTL 1.0
#define ARROW_FADE_TIME 0.125
#define ARROW_GRAVITY -15.0 /* m/s**2 */

struct sprite_arrow {
  struct sprite hdr;
  double dx,dy; // m/s
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
  SPRITE->ttl=ARROW_TTL;
  SPRITE->el=0.5;
  SPRITE->del=4.0;
  return 0;
}

static void _arrow_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    if (sprite->arg&4) {
      // You can pick up the stone again and reuse it. The tile used by item sprites is oriented toward the bottom of the cell, hence (y-something).
      struct sprite *itspr=sprite_new(0,sprite->owner,sprite->x,sprite->y-0.400,NS_item_stone<<24,RID_sprite_item);
    }
    sprite_kill_soon(sprite);
    return;
  }
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  SPRITE->el+=SPRITE->del*elapsed;
  SPRITE->del+=ARROW_GRAVITY*elapsed;
  if (SPRITE->el<0.0) {
    SPRITE->el=0.0;
    if (SPRITE->del<0.0) { // bounce
      //TODO If it's water below, arrange a splash and delete self.
      SPRITE->del*=-0.5;
      //TODO sound effect, just the tiniest "thump"
    }
  }
}

static void _arrow_render(struct sprite *sprite,int16_t x,int16_t y) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  if ((SPRITE->ttl<ARROW_FADE_TIME)&&!(sprite->arg&4)) { // Arrows fade out but stones don't.
    int alpha=(SPRITE->ttl*255.0)/ARROW_FADE_TIME;
    if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
  }
  graf_draw_tile(&g.graf,texid,x,y,sprite->tileid+1,sprite->xform);
  graf_draw_tile(&g.graf,texid,x,y-(int)(SPRITE->el*NS_sys_tilesize),sprite->tileid,sprite->xform);
  graf_set_alpha(&g.graf,0xff);
}

const struct sprite_type sprite_type_arrow={
  .name="arrow",
  .objlen=sizeof(struct sprite_arrow),
  .init=_arrow_init,
  .update=_arrow_update,
  .render=_arrow_render,
};
