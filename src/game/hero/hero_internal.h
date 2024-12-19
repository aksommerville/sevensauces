/* hero_internal.h
 * Hero is a sprite type, which are usually at game/sprite/sprite_type_*.c.
 * But the hero sprite is a special case, an enormous amount of outer-world game logic lives here.
 * So it's a top-level unit of game, and split out for clarity.
 */
 
#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/sauces.h"
#include "game/sprite/sprite.h"
#include "game/world/world.h"
#include "game/session/session.h"
#include "game/session/map.h"
#include "game/layer/layer.h"

#define HERO_TOUCH_LIMIT 16 /* How many simultaneously touched sprites? Really, one should do the trick. */

struct sprite_hero {
  struct sprite hdr;
  int walkdx,walkdy; // State of the dpad.
  int facedx,facedy; // One must be zero and the other nonzero.
  double cursorclock;
  int cursorframe;
  int mapid,col,row; // Track cell for quantized actions like harvest.
  struct sprite *touchv[HERO_TOUCH_LIMIT]; // WEAK and possibly invalid. Confirm residency before dereferencing.
  int touchc;
  double animclock;
  int animframe;
  uint8_t item_in_use;
  int fish_outcome; // We determine at the moment fishing begins, whether there will be a minigame or not.
};

#define SPRITE ((struct sprite_hero*)sprite)

// hero_triggers.c
void hero_check_touches(struct sprite *sprite);
void hero_check_cell(struct sprite *sprite);
void hero_get_item_cell(int *col,int *row,struct sprite *sprite);

// hero_motion.c
void hero_reset_animclock(struct sprite *sprite);
void hero_rejoin_motion(struct sprite *sprite);

// hero_items.c
int hero_drop_item(struct sprite *sprite,uint8_t itemid);
void hero_update_item(struct sprite *sprite,double elapsed);
void hero_fishpole_ready(struct sprite *sprite);

#endif
