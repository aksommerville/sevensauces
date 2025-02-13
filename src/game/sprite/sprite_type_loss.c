/* sprite_type_loss.c
 * They appear in the village the next day, if the stew made them unhappy.
 * Hero can apologize and win them back.
 * arg: race(LSB), lossp(<<8)
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "game/session/session.h"
#include "sprite.h"

struct sprite_loss {
  struct sprite hdr;
  uint8_t tileid0;
  double sated_ttl;
};

#define SPRITE ((struct sprite_loss*)sprite)

/* Init.
 */
 
static int _loss_init(struct sprite *sprite) {
  sprite->imageid=RID_image_sprites1;
  sprite->tileid=0x50;
  switch (sprite->arg&0xff) {
    case NS_race_man: sprite->tileid=0x50; break;
    case NS_race_rabbit: sprite->tileid=0x60; break;
    case NS_race_octopus: sprite->tileid=0x70; break;
    case NS_race_werewolf: sprite->tileid=0x80; break;
    case NS_race_princess: sprite->tileid=0x90; break;
  }
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

/* Update.
 */
 
static void _loss_update(struct sprite *sprite,double elapsed) {

  if (SPRITE->sated_ttl>0.0) {
    sprite->tileid=SPRITE->tileid0+2;
    if ((SPRITE->sated_ttl-=elapsed)<=0.0) {
      sprite_kill_soon(sprite);
    }
    return;
  }
  
  struct sprite *hero=sprites_get_hero(sprite->owner);
  if (hero) {
    if (sprite_hero_is_apologizing(hero,sprite)) {
      SPRITE->sated_ttl=1.0;
      int p=(sprite->arg>>8)&0xff;
      session_unlose_customer_at(g.session,p);
      egg_play_sound(RID_sound_apologize);
      sprite_hero_end_action(hero);
      if (g.session->inventory[g.session->invp]==NS_item_apology) {
        g.session->inventory[g.session->invp]=0;
      }
    }
  }
  
  //TODO walk around slowly.
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_loss={
  .name="loss",
  .objlen=sizeof(struct sprite_loss),
  .init=_loss_init,
  .update=_loss_update,
};
