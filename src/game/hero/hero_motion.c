/* hero_motion.
 * Receive dpad events and coordinate motion.
 */
 
#include "hero_internal.h"

/* Force animclock to show frame zero.
 */
 
void hero_reset_animclock(struct sprite *sprite) {
  SPRITE->animclock=0.200;
  SPRITE->animframe=0;
}

/* Dpad events.
 */
 
void sprite_hero_motion(struct sprite *sprite,int dx,int dy) {
  // Some items forbid direction change:
  switch (SPRITE->item_in_use) {
    case NS_item_grapple:
    case NS_item_fishpole:
      return;
  }
  // Normal processing...
  if (!SPRITE->walkdx&&!SPRITE->walkdy) hero_reset_animclock(sprite);
  if (dx) {
    SPRITE->walkdx=dx;
    SPRITE->facedx=dx;
    SPRITE->facedy=0;
  } else if (dy) {
    SPRITE->walkdy=dy;
    SPRITE->facedx=0;
    SPRITE->facedy=dy;
  }
  // Some items allow direction change, but not motion:
  switch (SPRITE->item_in_use) {
    case NS_item_sword:
    case NS_item_apology:
    case NS_item_loveletter:
      SPRITE->walkdx=SPRITE->walkdy=0;
      break;
  }
}

void sprite_hero_end_motion(struct sprite *sprite,int dx,int dy) {
  if (!dx&&!dy) {
    SPRITE->walkdx=0;
    SPRITE->walkdy=0;
    return;
  }
  if (dx) {
    if (dx==SPRITE->walkdx) {
      SPRITE->walkdx=0;
      if ((dx==SPRITE->facedx)&&SPRITE->walkdy) {
        SPRITE->facedx=0;
        SPRITE->facedy=SPRITE->walkdy;
      }
    }
  }
  if (dy) {
    if (dy==SPRITE->walkdy) {
      SPRITE->walkdy=0;
      if ((dy==SPRITE->facedy)&&SPRITE->walkdx) {
        SPRITE->facedx=SPRITE->walkdx;
        SPRITE->facedy=0;
      }
    }
  }
}

// At the end of an item usage which was preventing motion, poll input and resume motion if dpad held.
// We cheat and ask Egg directly for the input state.
void hero_rejoin_motion(struct sprite *sprite) {
  int input=egg_input_get_one(0);
  switch (input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: sprite_hero_motion(sprite,-1,0); break;
    case EGG_BTN_RIGHT: sprite_hero_motion(sprite,1,0); break;
  }
  switch (input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: sprite_hero_motion(sprite,0,-1); break;
    case EGG_BTN_DOWN: sprite_hero_motion(sprite,0,1); break;
  }
}
