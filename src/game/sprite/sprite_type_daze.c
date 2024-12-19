/* sprite_type_daze.c
 * Animated decoration to put above a beast's head when it's stunned.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "sprite.h"

struct sprite_daze {
  struct sprite hdr;
  double animclock;
  int animframe;
  uint8_t tileid0;
  struct sprite *master; // WEAK
  uint8_t master_tileid;
  double dx,dy; // Difference from master.
};

#define SPRITE ((struct sprite_daze*)sprite)

static int _daze_init(struct sprite *sprite) {
  if (!sprite->imageid) sprite->imageid=RID_image_sprites1;
  if (!sprite->tileid) sprite->tileid=0x43;
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

static void _daze_update(struct sprite *sprite,double elapsed) {
  if (!sprite_is_resident(sprite->owner,SPRITE->master)) {
    sprite_kill_soon(sprite);
    return;
  }
  if (SPRITE->master->tileid!=SPRITE->master_tileid) {
    sprite_kill_soon(sprite);
    return;
  }
  sprite->x=SPRITE->master->x+SPRITE->dx;
  sprite->y=SPRITE->master->y+SPRITE->dy;
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.050;
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
    sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
  }
}

const struct sprite_type sprite_type_daze={
  .name="daze",
  .objlen=sizeof(struct sprite_daze),
  .init=_daze_init,
  .update=_daze_update,
};

void sprite_daze_setup(struct sprite *sprite,struct sprite *master) {
  if (!sprite||(sprite->type!=&sprite_type_daze)) return;
  if (!(SPRITE->master=master)) return;
  SPRITE->master_tileid=SPRITE->master->tileid;
  SPRITE->dx=sprite->x-SPRITE->master->x;
  SPRITE->dy=sprite->y-SPRITE->master->y;
}

void sprite_daze_drop_for_master(struct sprites *sprites,struct sprite *master) {
  struct sprite **v;
  int c=sprites_get_all(&v,sprites);
  for (;c-->0;v++) {
    struct sprite *sprite=*v;
    if (sprite->type!=&sprite_type_daze) continue;
    if (SPRITE->master!=master) continue;
    sprite_kill_soon(sprite);
    // Keep going, there might be more than one.
  }
}
