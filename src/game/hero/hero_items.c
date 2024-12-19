/* hero_items.c
 * Logic peculiar to specific items.
 * The public sprite_hero_action() and sprite_hero_end_action() live here.
 */

#include "hero_internal.h"

/* Drop selected item.
 * Plain hero_drop_item() can be used out of context. It doesn't interact with globals or produce a sound effect.
 */
 
int hero_drop_item(struct sprite *sprite,uint8_t itemid) {
  const struct item *item=itemv+itemid;
  if (!item->flags) return -1;
  double x=sprite->x+SPRITE->facedx,y=sprite->y+SPRITE->facedy;
  sprites_find_spawn_position(&x,&y,sprite->owner);
  struct sprite *nspr=0;
  if (item->usage==NS_itemusage_free) {
    nspr=sprite_new(0,sprite->owner,x,y,itemid<<24,RID_sprite_faun);
  } else {
    nspr=sprite_new(0,sprite->owner,x,y,itemid<<24,RID_sprite_item);
  }
  if (!nspr) return -1;
  return 0;
}
 
static void hero_drop_item_manual(struct sprite *sprite,uint8_t itemid) {
  if (hero_drop_item(sprite,itemid)<0) return;
  egg_play_sound(RID_sound_dropitem);
  g.session->inventory[g.session->invp]=0;
}

static void hero_free_faun(struct sprite *sprite,uint8_t itemid) {
  if (hero_drop_item(sprite,itemid)<0) return;
  egg_play_sound(RID_sound_freefaun);
  g.session->inventory[g.session->invp]=0;
}

/* Sword.
 */
 
static void hero_begin_sword(struct sprite *sprite) {
  SPRITE->item_in_use=NS_item_sword;
  SPRITE->walkdx=SPRITE->walkdy=0;
  egg_play_sound(RID_sound_sword);
  //TODO Need an ongoing check while deployed, for injurable things.
}

static void hero_end_sword(struct sprite *sprite) {
  SPRITE->item_in_use=0;
  hero_rejoin_motion(sprite);
}

static void hero_update_sword(struct sprite *sprite,double elapsed) {
  //TODO
}

/* Bow.
 */
 
static void hero_begin_bow(struct sprite *sprite) {
  double x=sprite->x,y=sprite->y;
  uint32_t arg;
       if (SPRITE->facedx<0) { arg=0; x-=0.5; y+=0.5; }
  else if (SPRITE->facedx>0) { arg=1; x+=0.5; y+=0.5; }
  else if (SPRITE->facedy<0) { arg=2; y-=0.5; }
  else                       { arg=3; y+=1.0; }
  struct sprite *arrow=sprite_new(&sprite_type_arrow,sprite->owner,x,y,arg,0);
  if (!arrow) return;
  egg_play_sound(RID_sound_arrow);
}

/* Grapple.
 */
 
static void hero_begin_grapple(struct sprite *sprite) {
  double x=sprite->x,y=sprite->y;
       if (SPRITE->facedx<0) { x-=0.5; y+=3.0/16.0; }
  else if (SPRITE->facedx>0) { x+=0.5; y+=3.0/16.0; }
  else if (SPRITE->facedy<0) { x+=3.0/16.0; y-=0.5; }
  else if (SPRITE->facedy>0) { x-=3.0/16.0; y+=0.5; }
  else return;
  struct sprite *grapple=sprite_new(&sprite_type_grapple,sprite->owner,x,y,0,0);
  if (!grapple) return;
  sprite_grapple_setup(grapple,SPRITE->facedx,SPRITE->facedy);
  SPRITE->item_in_use=NS_item_grapple;
  SPRITE->walkdx=SPRITE->walkdy=0;
  egg_play_sound(RID_sound_grapple_start);
  //TODO Arrange to notice when the grapple TTLs out.
  //TODO Arrange to get updates from the grapple in general. (lift hero, retrieve item, ttl)
}

static void hero_end_grapple(struct sprite *sprite) {
  sprite_grapple_drop_all(sprite->owner);
  SPRITE->item_in_use=0;
  hero_rejoin_motion(sprite);
  //TODO If we ended up over water, should we drown? As is, we will pop out to a safe place.
  sprite_rectify_physics(sprite); // We don't check physics while grappling.
}

static void hero_update_grapple(struct sprite *sprite,double elapsed) {
  double dist; // Don't actually care.
  struct sprite *grapple=sprites_find_nearest(&dist,sprite->owner,sprite->x,sprite->y,&sprite_type_grapple,0,0,0);
  if (!grapple) {
    hero_end_grapple(sprite);
  }
}

void sprite_hero_engage_grapple(struct sprite *sprite) {
}

void sprite_hero_release_grapple(struct sprite *sprite) {
  sprite_rectify_physics(sprite); // We don't check physics while grappling.
}

/* Fishpole.
 */
 
static void hero_cb_fishing(int result,void *userdata) {
  if (result<1) return;
  struct sprite *sprite=userdata;
  int inventoryp=session_get_free_inventory_slot(g.session);
  if (inventoryp<0) return;
  //g.session->inventory[inventoryp]=NS_item_fish;
}
 
static void hero_begin_fishpole(struct sprite *sprite) {
  /*TODO copied from 20241217
  int col=-123,row;
  hero_get_item_cell(&col,&row,sprite);
  if ((col<0)||(row<0)||(col>=g.world->map->w)||(row>=g.world->map->h)) {
    // TODO rejection sound effect
    return;
  }
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->physics[tileid];
  if (physics!=NS_physics_water) {
    //TODO rejection sound effect
    return;
  }
  //TODO I think we want some animation in place first.
  // Show Dot holding the pole with line extending into the water, bump a little, see the fish fly up out of the water...
  sprite_hero_end_motion(sprite,0,0);
  struct layer *fishing=layer_spawn(&layer_type_fishing);
  if (!fishing) return;
  layer_fishing_setup(fishing,g.world,col,row,hero_cb_fishing,sprite);
  /**/
}

/* Shovel.
 */
 
static void hero_cb_shovel(int invp,void *userdata) {
  struct sprite *sprite=userdata;
  if ((invp<0)||(invp>=16)) return;
  uint8_t itemid=g.session->inventory[invp];
  const struct item *item=itemv+itemid;
  if (!(item->flags&(1<<NS_itemflag_plant))) {
    struct layer *msg=layer_spawn(&layer_type_message);
    layer_message_setup_string(msg,RID_strings_ui,20);
    return;
  }
  int col=-123,row;
  hero_get_item_cell(&col,&row,sprite);
  if ((col<0)||(row<0)||(col>=g.world->map->w)||(row>=g.world->map->h)) {
    // Shouldn't happen; we checked before the query.
    return;
  }
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->physics[tileid];
  if (physics!=NS_physics_diggable) {
    // Shouldn't happen; we checked before the query.
    return;
  }
  int ntileid=world_tileid_for_seed(g.world);
  if (ntileid<0) return;
  struct plant *plant=session_add_plant(g.session);
  if (!plant) {
    struct layer *msg=layer_spawn(&layer_type_message);
    layer_message_setup_string(msg,RID_strings_ui,21);
    return;
  }
  plant->mapid=g.world->map->rid;
  plant->x=col;
  plant->y=row;
  plant->itemid=itemid;
  g.world->map->v[cellp]=ntileid;
  g.session->inventory[invp]=0;
  egg_play_sound(RID_sound_plant);
}
 
static void hero_begin_shovel(struct sprite *sprite) {
  int col=-123,row;
  hero_get_item_cell(&col,&row,sprite);
  if ((col<0)||(row<0)||(col>=g.world->map->w)||(row>=g.world->map->h)) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->physics[tileid];
  if (physics!=NS_physics_diggable) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  if (world_tileid_for_seed(g.world)<0) {
    fprintf(stderr,"tilesheet:%d has no seed tile\n",g.world->map_imageid);
    return;
  }
  struct layer *pause=layer_spawn(&layer_type_pause);
  if (!pause) return;
  const char *prompt,*cancel;
  int promptc=strings_get(&prompt,RID_strings_ui,19);
  int cancelc=strings_get(&cancel,RID_strings_ui,18);
  layer_pause_setup_query(pause,prompt,promptc,cancel,cancelc,hero_cb_shovel,sprite);
}

/* Trap.
 * It functions a lot like the shovel, but a few differences:
 *  - Single-use.
 *  - Bears animals instead of vegetables.
 *  - What blooms will depend entirely on location.
 *  - The placed trap is recorded entirely by a map cell, no extra bookkeeping.
 */
 
static void hero_begin_trap(struct sprite *sprite) {
  int col=-123,row;
  hero_get_item_cell(&col,&row,sprite);
  if ((col<0)||(row<0)||(col>=g.world->map->w)||(row>=g.world->map->h)) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->physics[tileid];
  // Target cell has to be diggable -- those are the tiles we've arranged to be visually replaceable by a trap (or a hole).
  if (physics!=NS_physics_diggable) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  int ntileid=world_tileid_for_trap(g.world);
  if (ntileid<0) {
    fprintf(stderr,"tilesheet:%d does not contain a trap tile\n",g.world->map_imageid);
    // No sound effect here; this is a technical error. sorry.
    return;
  }
  egg_play_sound(RID_sound_placetrap);
  g.world->map->v[cellp]=ntileid;
  g.session->inventory[g.session->invp]=0;
}

/* Stone.
 */
 
static void hero_begin_stone(struct sprite *sprite) {
  double x=sprite->x,y=sprite->y;
  uint32_t arg=4;
       if (SPRITE->facedx<0) { arg|=0; y+=0.5; }
  else if (SPRITE->facedx>0) { arg|=1; y+=0.5; }
  else if (SPRITE->facedy<0) { arg|=2; }
  else                       { arg|=3; y+=0.5; }
  struct sprite *stone=sprite_new(&sprite_type_arrow,sprite->owner,x,y,arg,0);
  if (!stone) return;
  egg_play_sound(RID_sound_stone);
  g.session->inventory[g.session->invp]=0;
}

/* Letter (both Apology and Love Letter).
 */
 
static void hero_begin_letter(struct sprite *sprite,uint8_t itemid) {
  SPRITE->item_in_use=itemid;
  SPRITE->walkdx=SPRITE->walkdy=0;
  egg_play_sound(RID_sound_card);
}

static void hero_end_letter(struct sprite *sprite) {
  SPRITE->item_in_use=0;
  hero_rejoin_motion(sprite);
}

static void hero_update_letter(struct sprite *sprite,double elapsed,uint8_t itemid) {
  //TODO
}

/* Begin action.
 */

void sprite_hero_action(struct sprite *sprite) {
  uint8_t itemid=g.session->inventory[g.session->invp];
  const struct item *item=itemv+itemid;
  switch (item->usage) {
    case NS_itemusage_drop: hero_drop_item_manual(sprite,itemid); break;
    case NS_itemusage_free: hero_free_faun(sprite,itemid); break;
    case NS_itemusage_sword: hero_begin_sword(sprite); break;
    case NS_itemusage_bow: hero_begin_bow(sprite); break;
    case NS_itemusage_grapple: hero_begin_grapple(sprite); break;
    case NS_itemusage_fishpole: hero_begin_fishpole(sprite); break;
    case NS_itemusage_shovel: hero_begin_shovel(sprite); break;
    case NS_itemusage_trap: hero_begin_trap(sprite); break;
    case NS_itemusage_stone: hero_begin_stone(sprite); break;
    case NS_itemusage_letter: hero_begin_letter(sprite,itemid); break;
    default: {
        fprintf(stderr,"no usage for item 0x%02x\n",itemid);
        //TODO sound effect: No item, or no use for this one.
      }
  }
}

/* End action.
 */

void sprite_hero_end_action(struct sprite *sprite) {
  switch (SPRITE->item_in_use) {
    case NS_item_sword: hero_end_sword(sprite); break;
    case NS_item_grapple: hero_end_grapple(sprite); break;
    case NS_item_apology: hero_end_letter(sprite); break;
    case NS_item_loveletter: hero_end_letter(sprite); break;
  }
}

/* Continue action.
 */
 
void hero_update_item(struct sprite *sprite,double elapsed) {
  switch (SPRITE->item_in_use) {
    case NS_item_sword: hero_update_sword(sprite,elapsed); break;
    case NS_item_grapple: hero_update_grapple(sprite,elapsed); break;
    case NS_item_apology: hero_update_letter(sprite,elapsed,NS_item_apology); break;
    case NS_item_loveletter: hero_update_letter(sprite,elapsed,NS_item_apology); break;
  }
}