/* sprite_type_animonce.c
 * Decorative sprite that plays thru a series of frames then self-deletes.
 * arg=framec (0 equivalent to 1)
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "sprite.h"

#define FRAME_TIME 0.125

struct sprite_animonce {
  struct sprite hdr;
  double clock; // counts up
};

#define SPRITE ((struct sprite_animonce*)sprite)

static int _animonce_init(struct sprite *sprite) {
  return 0;
}

static void _animonce_update(struct sprite *sprite,double elapsed) {
  SPRITE->clock+=elapsed;
  if (SPRITE->clock>=FRAME_TIME) {
    SPRITE->clock-=FRAME_TIME;
    if (sprite->arg>1) {
      sprite->arg--;
      sprite->tileid++;
    } else {
      sprite_kill_soon(sprite);
    }
  }
}

const struct sprite_type sprite_type_animonce={
  .name="animonce",
  .objlen=sizeof(struct sprite_animonce),
  .init=_animonce_init,
  .update=_animonce_update,
};
