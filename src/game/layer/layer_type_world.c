/* layer_type_world.c
 * Container for the main diurnal gameplay.
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/world/world.h"
#include "game/sprite/sprite.h"
#include "layer.h"

struct layer_world {
  struct layer hdr;
};

#define LAYER ((struct layer_world*)layer)

static void _lw_del(struct layer *layer) {
}

static int _lw_init(struct layer *layer) {
  if (!g.world) return -1;
  return 0;
}

static void lw_pause(struct layer *layer) {
  struct layer *pause=layer_spawn(&layer_type_pause);
  if (!pause) return;
  layer_pause_setup_global(pause);
}

static void _lw_input(struct layer *layer,int input,int pvinput) {
  struct sprite *hero=sprites_get_hero(g.world->sprites);
  if (BTNDOWN(WEST)) lw_pause(layer);
  if (hero) {
    if (BTNDOWN(SOUTH)) sprite_hero_action(hero); else if (BTNUP(SOUTH)) sprite_hero_end_action(hero);
    if (BTNDOWN(LEFT)) sprite_hero_motion(hero,-1,0); else if (BTNUP(LEFT)) sprite_hero_end_motion(hero,-1,0);
    if (BTNDOWN(RIGHT)) sprite_hero_motion(hero,1,0); else if (BTNUP(RIGHT)) sprite_hero_end_motion(hero,1,0);
    if (BTNDOWN(UP)) sprite_hero_motion(hero,0,-1); else if (BTNUP(UP)) sprite_hero_end_motion(hero,0,-1);
    if (BTNDOWN(DOWN)) sprite_hero_motion(hero,0,1); else if (BTNUP(DOWN)) sprite_hero_end_motion(hero,0,1);
  }
}

static void _lw_update(struct layer *layer,double elapsed) {
  world_update(g.world,elapsed);
  if (g.world->clock<=0.0) {
    if (sauces_end_day()<0) egg_terminate(1);
  }
}

static void _lw_render(struct layer *layer) {
  world_render(g.world);
}

const struct layer_type layer_type_world={
  .name="world",
  .objlen=sizeof(struct layer_world),
  .del=_lw_del,
  .init=_lw_init,
  .input=_lw_input,
  .update=_lw_update,
  .render=_lw_render,
};
