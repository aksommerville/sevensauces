#include "game/sauces.h"
#include "game/session/session.h"
#include "game/session/map.h"
#include "game/layer/layer.h"
#include "game/world/world.h"
#include "game/kitchen/kitchen.h"
#include "sprite.h"
#include "opt/rom/rom.h"

#define HERO_TOUCH_LIMIT 16 /* How many simultaneously touched sprites? Really, one should do the trick. */

static int hero_drop_item(struct sprite *sprite,uint8_t itemid);

struct sprite_hero {
  struct sprite hdr;
  int walkdx,walkdy; // State of the dpad.
  int facedx,facedy; // One must be zero and the other nonzero.
  double cursorclock;
  int cursorframe;
  int mapid,col,row; // Track cell for quantized actions like harvest.
  struct sprite *touchv[HERO_TOUCH_LIMIT]; // WEAK and possibly invalid. Confirm residency before dereferencing.
  int touchc;
};

#define SPRITE ((struct sprite_hero*)sprite)

static void _hero_del(struct sprite *sprite) {
}

static int _hero_init(struct sprite *sprite) {
  sprite->imageid=RID_image_sprites1;
  sprite->tileid=0x00;
  SPRITE->facedy=1;
  return 0;
}

/* Check for sprites with (hero_touch) that our center is within their bounds.
 * We track touched sprites, and only send this event when newly touched.
 * We're equally capable of detecting "untouch", if there's ever a need.
 */
 
static int hero_touchv_search(const struct sprite *sprite,const struct sprite *other) {
  int i=SPRITE->touchc; while (i-->0) {
    if (SPRITE->touchv[i]==other) return i;
  }
  return -1;
}
 
static void hero_check_touches(struct sprite *sprite) {
  uint16_t still=0; // Little-endian bits, which members of (touchv) are still touched? Will include new additions.
  struct layer *focus=layer_stack_get_focus();
  int stop_adding=0;
  struct sprite **otherv;
  int otherc=sprites_get_all(&otherv,sprite->owner);
  while (otherc-->0) {
    struct sprite *other=otherv[otherc];
    if (!other->type->hero_touch) continue;
    double dx=sprite->x-other->x;
    if ((dx<-0.5)||(dx>0.5)) continue;
    double dy=sprite->y-other->y;
    if ((dy<-0.5)||(dy>0.5)) continue;
    int p=hero_touchv_search(sprite,other);
    if (p>=0) {
      still|=1<<p;
    } else if (stop_adding) {
    } else if (SPRITE->touchc<HERO_TOUCH_LIMIT) {
      still|=1<<SPRITE->touchc;
      SPRITE->touchv[SPRITE->touchc++]=other;
      other->type->hero_touch(other,sprite);
      // Touch handlers can cause modals to push (eg "drop item to make room?"), and it's not unheard-of for two to happen in the same pass.
      // So if the focus layer changes during iteration, stop adding touches.
      if (layer_stack_get_focus()!=focus) stop_adding=1;
    } else {
      // Touching more than 16 things. Don't trigger the event for ones we can't track.
    }
  }
  uint16_t mask=1<<(SPRITE->touchc-1);
  int i=SPRITE->touchc;
  for (;i-->0;mask>>=1) {
    if (!(still&mask)) {
      // untouch
      SPRITE->touchc--;
      memmove(SPRITE->touchv+i,SPRITE->touchv+i+1,sizeof(void*)*(SPRITE->touchc-i));
    }
  }
}

/* Callback from a deferred single-item harvest.
 */
 
static void hero_cb_harvest(int invp,void *userdata) {
  struct sprite *sprite=userdata;
  if ((invp<0)||(invp>=16)) return; // Cancel, no worries.
  uint8_t itemid_drop=g.session->inventory[invp];
  if (!itemid_drop) return;
  int cellp=SPRITE->row*g.world->map->w+SPRITE->col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t itemid_take=g.world->tsitemid[tileid];
  const struct item *item=itemv+itemid_take;
  if (item->harvestq!=1) return; // Impossible but stay safe.
  g.world->map->v[cellp]=g.world->map->ro[cellp];
  g.session->inventory[invp]=itemid_take;
  hero_drop_item(sprite,itemid_drop);
}

/* Check whether I've moved to a new cell, and if so look for quantized actions.
 * This runs every frame to be on the safe side, so get out fast if there's no change.
 */
 
static void hero_check_cell(struct sprite *sprite) {
  int col=(int)sprite->x,row=(int)sprite->y;
  if ((SPRITE->mapid==g.world->map->rid)&&(SPRITE->col==col)&&(SPRITE->row==row)) return;
  SPRITE->mapid=g.world->map->rid;
  SPRITE->col=col;
  SPRITE->row=row;
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->physics[tileid];
  switch (physics) {
  
    case NS_physics_harvest: {
        uint8_t itemid=g.world->tsitemid[tileid];
        const struct item *item=itemv+itemid;
        if (!item->harvestq) {
          fprintf(stderr,"tilesheet:%d: Tile 0x%02x has itemid 0x%02x with invalid harvestq=%d\n",g.world->map_imageid,tileid,itemid,item->harvestq);
          break;
        }
        // Single-harvest items like animal traps are easy.
        if (item->harvestq==1) {
          int result=session_acquire_item(g.session,itemid,hero_cb_harvest,sprite);
          if (result>0) {
            g.world->map->v[cellp]=g.world->map->ro[cellp];
          }
          break;
        }
        // Multi-harvest like crops are a trick.
        int freec=session_count_free_inventory_slots(g.session);
        if (item->harvestq>freec) {
          // If you don't have 3 available slots, you can't pick crops.
          // Picking less than 3 isn't an option, we don't have tiles for the 1 and 2 cases.
          // We could prompt like in the single case, but I don't like prompting 3 times.
          // Alternately, we could pick what's possible and create sprites for the remainder. I'm not crazy about that either.
          // Leaving it blank is not too bad, and I'll keep it this way, at least for now.
        } else {
          session_acquire_item(g.session,itemid,0,"passive");
          int i=1; for (;i<item->harvestq;i++) { // Don't produce feedback after the first one.
            session_acquire_item(g.session,itemid,0,0);
          }
          g.world->map->v[cellp]=g.world->map->ro[cellp];
        }
      } break;

  }
  struct poi *poi;
  int i=map_get_poi(&poi,g.world->map,col,row);
  for (;i-->0;poi++) {
    switch (poi->opcode) {
    
      case CMD_map_home: {
          //XXX quick hack during dev. In real life, present a query modal "Are you sure?"
          g.world->clock=0.0;
        } return;
        
      case CMD_map_shop: break;//TODO
      case CMD_map_condensery: break;//TODO
      case CMD_map_door: break;//TODO
    
    }
  }
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  if ((SPRITE->cursorclock-=elapsed)<0.0) {
    SPRITE->cursorclock+=0.200;
    if (++(SPRITE->cursorframe)>=4) SPRITE->cursorframe=0;
  }

  if (SPRITE->walkdx||SPRITE->walkdy) {
    const double walkspeed=6.0; // m/s
    sprite->x+=walkspeed*elapsed*SPRITE->walkdx;
    sprite->y+=walkspeed*elapsed*SPRITE->walkdy;
    sprite_rectify_physics(sprite);
  }
  
  hero_check_touches(sprite);
  hero_check_cell(sprite);
}

/* For items that quantize against the map, which cell will they affect?
 * Note that we don't care which item it is.
 */
 
static void hero_get_item_cell(int *col,int *row,struct sprite *sprite) {
  const double adjust=0.6; // Must be >0.5, at exactly 0.5 it can't reach out left or up.
  double x=sprite->x+SPRITE->facedx*adjust;
  double y=sprite->y+SPRITE->facedy*adjust;
  *col=(int)x; if (x<0.0) (*col)--;
  *row=(int)y; if (y<0.0) (*row)--;
}

/* Render.
 */

static void _hero_render(struct sprite *sprite,int16_t x,int16_t y) {
  const struct item *item=itemv+g.session->inventory[g.session->invp];
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  
  if (item->flags&(1<<NS_itemflag_cell)) {
    int col=-100,row=0;
    hero_get_item_cell(&col,&row,sprite);
    int16_t dstx=col*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.world->viewx;
    int16_t dsty=row*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.world->viewy;
    uint8_t xform=0;
    switch (SPRITE->cursorframe) {
      case 1: xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 2: xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    }
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x40,xform);
  }

  // (facedx,facedy) are the authority for our face direction. Update (tileid,xform) accordingly.
       if (SPRITE->facedx<0) { sprite->tileid=0x02; sprite->xform=0; }
  else if (SPRITE->facedx>0) { sprite->tileid=0x02; sprite->xform=EGG_XFORM_XREV; }
  else if (SPRITE->facedy<0) { sprite->tileid=0x01; sprite->xform=0; }
  else                       { sprite->tileid=0x00; sprite->xform=0; }
  
  //TODO Update (tileid) per animation.

  graf_draw_tile(&g.graf,texid,x,y,sprite->tileid,sprite->xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};

/* Dpad events.
 */
 
void sprite_hero_motion(struct sprite *sprite,int dx,int dy) {
  if (dx) {
    SPRITE->walkdx=dx;
    SPRITE->facedx=dx;
    SPRITE->facedy=0;
  } else if (dy) {
    SPRITE->walkdy=dy;
    SPRITE->facedx=0;
    SPRITE->facedy=dy;
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

/* Drop selected item.
 * Plain hero_drop_item() can be used out of context. It doesn't interact with globals or produce a sound effect.
 */
 
static int hero_drop_item(struct sprite *sprite,uint8_t itemid) {
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
  fprintf(stderr,"TODO %s\n",__func__);
}

/* Bow.
 */
 
static void hero_begin_bow(struct sprite *sprite) {
  double x=sprite->x,y=sprite->y;
  uint32_t arg;
       if (SPRITE->facedx<0) { arg=0; y+=0.5; }
  else if (SPRITE->facedx>0) { arg=1; y+=0.5; }
  else if (SPRITE->facedy<0) { arg=2; }
  else                       { arg=3; y+=0.5; }
  struct sprite *arrow=sprite_new(&sprite_type_arrow,sprite->owner,x,y,arg,0);
  if (!arrow) return;
  egg_play_sound(RID_sound_arrow);
}

/* Grapple.
 */
 
static void hero_begin_grapple(struct sprite *sprite) {
  fprintf(stderr,"TODO %s\n",__func__);
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
 
static void hero_begin_letter(struct sprite *sprite) {
  fprintf(stderr,"TODO %s\n",__func__);
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
    case NS_itemusage_letter: hero_begin_letter(sprite); break;
    default: {
        fprintf(stderr,"no usage for item 0x%02x\n",itemid);
        //TODO sound effect: No item, or no use for this one.
      }
  }
  /**/
}

/* End action.
 */

void sprite_hero_end_action(struct sprite *sprite) {
}
