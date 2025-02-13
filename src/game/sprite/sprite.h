/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;
struct sprites;

/* sprites plural: Container object for all live sprites.
 * World creates one of these, and there should only be one at a time.
 * Every instantiated sprite requires a container.
 * In the past, this responsibility has been global and static. Trying something new here.
 *******************************************************************/
 
void sprites_del(struct sprites *sprites);
struct sprites *sprites_new();

void sprites_update(struct sprites *sprites,double elapsed);
void sprites_render(struct sprites *sprite);

/* Helpers for map loader.
 * Listing and unlisting hands off ownership.
 * An unlisted sprite still has (owner), and you can only relist into that owner.
 */
void sprites_drop_all(struct sprites *sprites);
void sprites_unlist(struct sprites *sprites,struct sprite *sprite);
int sprites_list(struct sprites *sprites,struct sprite *sprite);

struct sprite *sprites_get_hero(struct sprites *sprites);
int sprites_get_all(struct sprite ***dstppp,const struct sprites *sprites);

// Safely check whether your WEAK reference (sprite) is still kosher.
int sprite_is_resident(const struct sprites *sprites,const struct sprite *sprite);

/* Return the sprite closest to (x,y), optionally filtering.
 * match: <0 to match (closer) and abort, >0 to match (closer), 0 to keep (prev). (prev) may be null.
 * Do not add or remove sprites during iteration.
 * We compare by Manhattan distance, not true distance.
 */
struct sprite *sprites_find_nearest(
  double *dist,
  const struct sprites *sprites,
  double x,double y,
  const struct sprite_type *type, // Any if null.
  int rid, // Any if zero.
  int (*match)(struct sprite *closer,struct sprite *prev,void *userdata),
  void *userdata
);

/* (x,y) are in and out.
 * For a position near (x,y), find a good place to create a new sprite.
 */
void sprites_find_spawn_position(double *x,double *y,const struct sprites *sprites);

/* Generic sprite instance.
 ***************************************************************/
 
struct sprite {
  const struct sprite_type *type;
  struct sprites *owner; // WEAK, REQUIRED
  int rid;
  const void *cmdv; // From "sprite" resource, if there was one.
  int cmdc;
  uint32_t arg; // From map's spawn point.
  double x,y; // Center of sprite, in world cells.
  int imageid; // For generic render.
  uint8_t tileid,xform; // For generic render.
  int grapplable;
  int layer;
};

/* Deletes the sprite immediately and unlists from its owner.
 * Outside parties should not do this, use sprite_kill_soon() instead.
 */
void sprite_del(struct sprite *sprite);

/* You must supply one of (type,rid) and not both.
 * With (rid), we fail if the resource doesn't exist.
 */
struct sprite *sprite_new(
  const struct sprite_type *type,
  struct sprites *owner,
  double x,double y,
  uint32_t arg,
  int rid
);

/* Schedule this sprite for deletion the next time owner feels it's safe.
 * Caller should treat it as deleted, ie stop using it immediately.
 */
void sprite_kill_soon(struct sprite *sprite);

/* I'm avoiding generalized physics, let's see if we can do without.
 * Sprites that want collision resolution against the map should call this on their own, any time they've moved.
 * There is no provision for sprite-on-sprite collisions.
 * All sprites are circles with a radius of 0.5 m.
 * (border_radius) is the minimum distance to world edges. Clamps low to 0.5.
 * Returns nonzero if the position changed.
 */
int sprite_rectify_physics(struct sprite *sprite,double border_radius);

/* Sprite type.
 *************************************************************/
 
struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  
  /* Optional event hooks.
   * Implement if you want those events. Leave null if not -- there might be efficiency's on the management end.
   * hero_touch: Called each frame when the hero's center is within your bounds, if the hero is moving.
   */
  void (*update)(struct sprite *sprite,double elapsed);
  void (*hero_touch)(struct sprite *sprite,struct sprite *hero);
  
  /* Sprites that move on their own and are subject to the grapple must implement this.
   * When you have a non-null controller, don't move.
   * It's currently not possible for two controllers to contend over one paralyzee.
   * If that comes up in the future, it will need dealt with.
   */
  void (*paralyze)(struct sprite *sprite,struct sprite *controller);
  
  /* Implement (render) only if a single tile won't suffice.
   * (x,y) are (sprite->x,y) in framebuffer pixels.
   * If implemented, the usual culling will not happen, you'll be called every frame, possibly with a very OOB (x,y).
   */
  void (*render)(struct sprite *sprite,int16_t x,int16_t y);
};

/* Specific sprite types.
 ********************************************************/
 
#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRITE_TYPE
#undef _

void sprite_hero_motion(struct sprite *sprite,int dx,int dy);
void sprite_hero_end_motion(struct sprite *sprite,int dx,int dy);
void sprite_hero_action(struct sprite *sprite);
void sprite_hero_end_action(struct sprite *sprite);
void sprite_hero_engage_grapple(struct sprite *sprite);
void sprite_hero_release_grapple(struct sprite *sprite);
int sprite_hero_is_apologizing(struct sprite *sprite,struct sprite *apologizee);

// Daze remains locked to master's position, and deletes self when master's tileid changes. You must set the initial position.
// If you're not going to change tileid, you can manually drop all dazes for a given master too.
void sprite_daze_setup(struct sprite *sprite,struct sprite *master);
void sprite_daze_drop_for_master(struct sprites *sprites,struct sprite *master);

void sprite_grapple_setup(struct sprite *sprite,int dx,int dy);
void sprite_grapple_drop_all(struct sprites *sprites);
 
#endif
