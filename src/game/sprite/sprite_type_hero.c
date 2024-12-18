#include "game/sauces.h"
#include "game/session/session.h"
#include "game/session/map.h"
#include "game/layer/layer.h"
#include "game/world/world.h"
#include "game/kitchen/kitchen.h"
#include "sprite.h"
#include "opt/rom/rom.h"

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

/* Get the item metadata whose planted tile is given.
 * itemid is (item-item_metadata).
 * Can be null.
 */
 
static const struct item_metadata *hero_item_for_tileid(uint8_t tileid) {
  /*TODO copied from 20241217
  const struct item_metadata *meta=item_metadata;
  int i=256;
  for (;i-->0;meta++) {
    if (meta->plant_tileid!=tileid) continue;
    return meta;
  }
  /**/
  return 0;
}

/* Harvesting a crop, but we have no room in inventory: Create an item sprite somewhere close.
 */
 
static void hero_harvest_and_drop(struct sprite *sprite,uint8_t itemid) {
  //TODO Look for a decent spot. In particular, there can be two harvest-and-drops at the same time, don't put them on top of each other!
  /*TODO copied from 20241217
  double x=sprite->x+SPRITE->facedx;
  double y=sprite->y+SPRITE->facedy;
  struct sprite *item=sprite_new(0,sprite->owner,x,y,itemid,RID_sprite_item);
  if (!item) return;
  /**/
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
        /*TODO copied from 20241217
        const struct item_metadata *meta=hero_item_for_tileid(tileid);
        if (meta) {
          uint8_t itemid=meta-item_metadata;
          int harvested=0;
          int quantity=(meta->foodgroup==NS_foodgroup_meat)?1:3;
          int i=0; for (;i<quantity;i++) {
            int inventoryp=session_get_free_inventory_slot(g.session);
            if (inventoryp<0) {
              if (!harvested) break; // No room at all? Don't harvest it.
              hero_harvest_and_drop(sprite,itemid);
            } else {
              harvested++;
              g.session->inventory[inventoryp]=itemid;
            }
          }
          if (harvested) {
            //TODO sound effect
            //TODO passive toast like "Picked up 2 pumpkins"
            g.world->map->v[cellp]=0;
          }
        }
        /**/
      } break;

  }
  struct rom_command_reader reader={.v=g.world->map->cmdv,.c=g.world->map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    // All positioned commands start with (u8 x,u8 y).
    if ((cmd.argc<2)||(cmd.argv[0]!=col)||(cmd.argv[1]!=row)) continue;
    switch (cmd.opcode) {
      /*TODO copied from 20241217
      case CMD_map_home: {
          struct layer *prompt=layer_spawn(&layer_type_prompt);
          if (!prompt) return;
          layer_prompt_setup(prompt,RID_strings_ui,8,3,hero_cb_home,0);
          sprite_hero_end_motion(sprite,0,0);
        } break;
      case CMD_map_shop: {
          struct layer *shop=layer_spawn(&layer_type_shop);
          if (!shop) return;
          layer_shop_setup(shop,g.session,(cmd.argv[2]<<8)|cmd.argv[3]);
          sprite->x=col+0.5;
          sprite->y=row+0.5;
          sprite_hero_end_motion(sprite,0,0);
          SPRITE->facedx=0;
          SPRITE->facedy=1;
        } break;
      case CMD_map_condensery: {
          struct layer *cry=layer_spawn(&layer_type_condensery);
          if (!cry) return;
          layer_condensery_setup(cry,g.session);
          sprite->x=col+0.5;
          sprite->y=row+0.5;
          sprite_hero_end_motion(sprite,0,0);
          SPRITE->facedx=0;
          SPRITE->facedy=1;
        } break;
      /**/
    }
  }
  //TODO doors
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
 */
 
static void hero_drop_item(struct sprite *sprite,uint8_t itemid) {
  double x=sprite->x+SPRITE->facedx,y=sprite->y+SPRITE->facedy;
  sprites_find_spawn_position(&x,&y,sprite->owner);
  struct sprite *item=sprite_new(0,sprite->owner,x,y,itemid<<24,RID_sprite_item);
  if (!item) return;
  egg_play_sound(RID_sound_dropitem);
  g.session->inventory[g.session->invp]=0;
}

static void hero_free_faun(struct sprite *sprite,uint8_t itemid) {
  double x=sprite->x+SPRITE->facedx,y=sprite->y+SPRITE->facedy;
  sprites_find_spawn_position(&x,&y,sprite->owner);
  struct sprite *faun=sprite_new(0,sprite->owner,x,y,itemid<<24,RID_sprite_faun);
  if (!faun) return;
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
  //TODO sound effect
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
 
static void hero_cb_shovel(int inventoryp,void *userdata) {
  /*TODO copied from 20241217
  if ((inventoryp<0)||(inventoryp>=INVENTORY_SIZE)) return; // Cancelled, don't care.
  struct sprite *sprite=userdata;
  uint8_t itemid=g.session->inventory[inventoryp];
  const struct item_metadata *meta=item_metadata+itemid;
  if (!(meta->itemflag&(1<<NS_itemflag_plantable))) { // Invalid item for planting.
    struct layer *msg=layer_spawn(&layer_type_message);
    layer_message_setup(msg,RID_strings_ui,5);
    return;
  }
  if (g.session->plantc>=PLANT_LIMIT) { // Too many plants.
    struct layer *msg=layer_spawn(&layer_type_message);
    layer_message_setup(msg,RID_strings_ui,6);
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
  g.world->map->v[cellp]=0x0f;
  g.session->inventory[inventoryp]=0;
  struct plant *plant=g.session->plantv+g.session->plantc++;
  plant->mapid=g.world->map->rid;
  plant->x=col;
  plant->y=row;
  plant->itemid=itemid;
  /**/
}
 
static void hero_begin_shovel(struct sprite *sprite) {
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
  if (physics!=NS_physics_diggable) {
    //TODO rejection sound effect
    return;
  }
  struct layer *pickitem=layer_spawn(&layer_type_pickitem);
  if (!pickitem) return;
  layer_pickitem_setup(pickitem,g.session,RID_strings_ui,4,hero_cb_shovel,sprite);
  sprite_hero_end_motion(sprite,0,0);
  /**/
}

/* Trap.
 * It functions a lot like the shovel, but a few differences:
 *  - Single-use.
 *  - Bears animals instead of vegetables.
 *  - You don't commit a separate item like planting. TODO It would make narrative sense, if you have to supply bait with each trap. Should we?
 *  - What blooms will depend on location rather than bait.
 */
 
static void hero_begin_trap(struct sprite *sprite) {
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
  // Not sure if this makes sense, but we're restricting to NS_physics_diggable like the shovel.
  // Main reason is we're going to overwrite the map cell, and NS_physics_vacant would include too many important things.
  if (physics!=NS_physics_diggable) {
    //TODO rejection sound effect
    return;
  }
  if (g.session->trapc>=TRAP_LIMIT) {
    struct layer *msg=layer_spawn(&layer_type_message);
    layer_message_setup(msg,RID_strings_ui,7);
    return;
  }
  g.world->map->v[cellp]=0x0d;
  g.session->inventory[g.session->inventoryp]=0;
  struct trap *trap=g.session->trapv+g.session->trapc++;
  trap->mapid=g.world->map->rid;
  trap->x=col;
  trap->y=row;
  /**/
}

/* Stone.
 */
 
static void hero_begin_stone(struct sprite *sprite) {
  fprintf(stderr,"TODO %s\n",__func__);
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
    case NS_itemusage_drop: hero_drop_item(sprite,itemid); break;
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
