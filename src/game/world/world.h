/* world.h
 * Modal for the diurnal phase of gameplay.
 */
 
#ifndef WORLD_H
#define WORLD_H

struct map;
struct sprites;

#define WORLD_PLAY_TIME 180

struct world {
  struct map *map; // WEAK, owned by session
  int viewx,viewy; // Position in world pixels of the last render.
  struct sprites *sprites; // STRONG. Stays alive, but we empty it on map changes, except for the hero.
  //TODO transient flora and fauna -- need a level of persistence above sprite
  double clock; // Counts down to end of day.
};

void world_del(struct world *world);
struct world *world_new();

int world_load_map(struct world *world,int mapid);

void world_update(struct world *world,double elapsed);
void world_render(struct world *world);

// Last chance to sync state sessionward before the world gets deleted.
void world_commit_to_session(struct world *world);

/* tileid on the current tilesheet for a beast of this itemid caught in a trap.
 * <0 if no such tile exists.
 */
int world_tileid_for_trapped_faun(const struct world *world,uint8_t itemid);
int world_tileid_for_trap(const struct world *world);
int world_tileid_for_seed(const struct world *world);

/* These only work on the current map's forages.
 */
void world_update_forage(struct world *world,uint8_t forageid,int col,int row);
void world_delete_forage(struct world *world,uint8_t forageid);

#endif
