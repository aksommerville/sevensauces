/* sprite_type_item.c
 * A loose item that you can pick up and add to inventory.
 * Spawn arg: u8 itemid, u24 reserved
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/world/world.h"
#include "sprite.h"

struct sprite_item {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_item*)sprite)

static void _item_del(struct sprite *sprite) {
}

static int _item_init(struct sprite *sprite) {
  sprite->tileid=sprite->arg>>24;
  return 0;
}

static void item_cb_chosen(int invp,void *userdata) {
  struct sprite *sprite=userdata;
  if ((invp<0)||(invp>=16)) return; // Cancelled, no worries.
  uint8_t dropid=g.session->inventory[invp];
  uint8_t takeid=sprite->arg>>24;
  if (!dropid||!takeid) return; // Shouldn't have gotten this far, but please go no further.
  const struct item *item=itemv+dropid;
  
  // With NS_itemusage_free, we must create a new sprite for the dropped item.
  if (item->usage==NS_itemusage_free) {
    struct sprite *faun=sprite_new(&sprite_type_faun,g.world->sprites,sprite->x,sprite->y,dropid<<24,0);
    sprite_kill_soon(sprite);
    g.session->inventory[invp]=takeid;
    return;
  }
  
  // All others behave like NS_itemusage_drop: Swap IDs with me.
  sprite->arg=(sprite->arg&0x00ffffff)|(dropid<<24);
  sprite->tileid=dropid;
  g.session->inventory[invp]=takeid;
}

static void _item_hero_touch(struct sprite *sprite,struct sprite *hero) {
  int result=session_acquire_item(g.session,sprite->arg>>24,item_cb_chosen,sprite);
  if (result>0) {
    sprite_kill_soon(sprite);
  }
}

const struct sprite_type sprite_type_item={
  .name="item",
  .objlen=sizeof(struct sprite_item),
  .del=_item_del,
  .init=_item_init,
  .hero_touch=_item_hero_touch,
};
