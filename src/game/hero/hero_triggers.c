/* hero_triggers.c
 * Things we make happen due to touching a sprite or map cell.
 */
 
#include "hero_internal.h"

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
 
void hero_check_touches(struct sprite *sprite) {
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

/* Enter shop.
 */
 
static void hero_enter_shop(struct sprite *sprite,int16_t shopid) {
  struct layer *layer=layer_spawn(&layer_type_shop);
  if (layer_shop_setup(layer,shopid)<0) {
    layer_pop(layer);
    return;
  }
  SPRITE->facedx=0;
  SPRITE->facedy=1;
}

/* Enter condensery.
 */
 
static void hero_enter_condensery(struct sprite *sprite) {
  struct layer *layer=layer_spawn(&layer_type_condensery);
  if (!layer) return;
  SPRITE->facedx=0;
  SPRITE->facedy=1;
}

/* Enter door.
 */
 
static void hero_enter_door(struct sprite *sprite,int16_t dstmapid,uint8_t dstcol,uint8_t dstrow) {
  //TODO Transition
  if (world_load_map(g.world,dstmapid)<0) return;
  sprite->x=dstcol+0.5;
  sprite->y=dstrow+0.5;
  
  // Important! Update (col,row) eagerly so we don't trip the remote side of this door.
  SPRITE->mapid=dstmapid;
  SPRITE->col=dstcol;
  SPRITE->row=dstrow;
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
  uint8_t itemid_take=g.world->map->itemid[tileid];
  const struct item *item=itemv+itemid_take;
  if (item->harvestq!=1) return; // Impossible but stay safe.
  g.world->map->v[cellp]=g.world->map->ro[cellp];
  g.session->inventory[invp]=itemid_take;
  hero_drop_item(sprite,itemid_drop);
}

/* Check whether I've moved to a new cell, and if so look for quantized actions.
 * This runs every frame to be on the safe side, so get out fast if there's no change.
 */
 
void hero_check_cell(struct sprite *sprite) {
  int col=(int)sprite->x,row=(int)sprite->y;
  if ((SPRITE->mapid==g.world->map->rid)&&(SPRITE->col==col)&&(SPRITE->row==row)) return;
  SPRITE->mapid=g.world->map->rid;
  SPRITE->col=col;
  SPRITE->row=row;
  int cellp=row*g.world->map->w+col;
  uint8_t tileid=g.world->map->v[cellp];
  uint8_t physics=g.world->map->physics[tileid];
  switch (physics) {
  
    case NS_physics_harvest: {
        uint8_t itemid=g.world->map->itemid[tileid];
        const struct item *item=itemv+itemid;
        if (!item->harvestq) {
          fprintf(stderr,"tilesheet:%d: Tile 0x%02x has itemid 0x%02x with invalid harvestq=%d\n",g.world->map->imageid,tileid,itemid,item->harvestq);
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
        
      case CMD_map_shop: hero_enter_shop(sprite,(poi->argv[2]<<8)|poi->argv[3]); return;
      case CMD_map_condensery: hero_enter_condensery(sprite); return;
      case CMD_map_door: hero_enter_door(sprite,(poi->argv[2]<<8)|poi->argv[3],poi->argv[4],poi->argv[5]); return;
    
    }
  }
}

/* For items that quantize against the map, which cell will they affect?
 * Note that we don't care which item it is.
 */
 
void hero_get_item_cell(int *col,int *row,struct sprite *sprite) {
  const double adjust=0.6; // Must be >0.5, at exactly 0.5 it can't reach out left or up.
  double x=sprite->x+SPRITE->facedx*adjust;
  double y=sprite->y+SPRITE->facedy*adjust;
  *col=(int)x; if (x<0.0) (*col)--;
  *row=(int)y; if (y<0.0) (*row)--;
}
