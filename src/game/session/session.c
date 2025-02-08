#include "game/sauces.h"
#include "game/world/world.h"
#include "game/sprite/sprite.h"
#include "game/layer/layer.h"
#include "map.h"
#include "session.h"
#include "opt/rom/rom.h"

/* Delete.
 */
 
void session_del(struct session *session) {
  if (!session) return;
  free(session);
}

/* Init, the fallible parts.
 */
 
static int session_init(struct session *session) {
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    switch (res->tid) {
      case EGG_TID_map: {
          if (session->mapc>SESSION_MAP_LIMIT) {
            fprintf(stderr,"map limit (%d) exceeded\n",SESSION_MAP_LIMIT);
            return -1;
          }
          struct map *map=session->mapv+session->mapc++;
          if (map_init(map,res->rid,res->v,res->c)<0) return -1;
        } break;
    }
  }
  return 0;
}

/* New.
 */
 
struct session *session_new() {
  struct session *session=calloc(1,sizeof(struct session));
  if (!session) return 0;
  if (session_init(session)<0) {
    session_del(session);
    return 0;
  }
  return session;
}

/* Get map.
 */
 
struct map *session_get_map(struct session *session,int mapid) {
  int lo=0,hi=session->mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct map *q=session->mapv+ck;
         if (mapid<q->rid) hi=ck;
    else if (mapid>q->rid) lo=ck+1;
    else return q;
  }
  return 0;
}

/* Inventory.
 */
 
int session_get_free_inventory_slot(const struct session *session) {
  // Prefer the selected slot, if it's empty.
  if (!session->inventory[session->invp]) return session->invp;
  // Otherwise, take the lowest index.
  int i=0; for (;i<INVENTORY_SIZE;i++) if (!session->inventory[i]) return i;
  return -1;
}

int session_count_free_inventory_slots(const struct session *session) {
  int c=0,i=INVENTORY_SIZE;
  const uint8_t *v=session->inventory;
  for (;i-->0;v++) if (!*v) c++;
  return c;
}

int session_acquire_item(struct session *session,uint8_t itemid,void (*cb)(int invp,void *userdata),void *userdata) {
  const struct item *item=itemv+itemid;
  if (!item->flags) { // All real items must have nonzero flags; there's a "valid" flag for this purpose.
    // This is a technical error, so don't create passive feedback even if allowed.
    fprintf(stderr,"%s: Invalid itemid 0x%02x\n",__func__,itemid);
    return -1;
  }
  
  int invp=session_get_free_inventory_slot(session);
  if (invp>=0) {
    session->inventory[invp]=itemid;
    if (cb||userdata) {
      egg_play_sound(RID_sound_pickup);
      if (g.world) {
        struct sprite *hero=sprites_get_hero(g.world->sprites);
        if (hero) {
          char msg[64];
          int msgc=sauces_format_item_string(msg,sizeof(msg),RID_strings_ui,13,itemid);
          if ((msgc>0)&&(msgc<=sizeof(msg))) {
            struct layer *toast=layer_spawn(&layer_type_toast);
            layer_toast_setup_world(toast,hero->x,hero->y,msg,msgc,0xffffffff);
          }
        }
      }
    }
    return 1;
    
  } else if (cb) {
    struct layer *pause=layer_spawn(&layer_type_pause);
    if (!pause) return -1;
    char msg[64];
    int msgc=sauces_format_item_string(msg,sizeof(msg),RID_strings_ui,17,itemid);
    const char *cancel=0;
    int cancelc=strings_get(&cancel,RID_strings_ui,18);
    if (layer_pause_setup_query(pause,msg,msgc,cancel,cancelc,cb,userdata)<0) return -1;
    return 0;
  }
  
  if (cb||userdata) {
    egg_play_sound(RID_sound_cant_pickup);
    if (g.world) {
      struct sprite *hero=sprites_get_hero(g.world->sprites);
      if (hero) {
        const char *msg=0;
        int msgc=strings_get(&msg,RID_strings_ui,14);
        if (msgc>0) {
          struct layer *toast=layer_spawn(&layer_type_toast);
          layer_toast_setup_world(toast,hero->x,hero->y,msg,msgc,0xff0000ff);
        }
      }
    }
  }
  return -1;
}

/* Plants.
 */
 
struct plant *session_add_plant(struct session *session) {
  if (session->plantc>=SESSION_PLANT_LIMIT) return 0;
  struct plant *plant=session->plantv+session->plantc++;
  memset(plant,0,sizeof(struct plant));
  return plant;
}

static int session_get_map_imageid(const struct map *map) {
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode==CMD_map_image) return (cmd.argv[0]<<8)|cmd.argv[1];
  }
  return 0;
}

static void session_read_tilesheet(uint8_t *physics,uint8_t *tsitemid,int imageid) {
  memset(physics,0,256);
  memset(tsitemid,0,256);
  const void *src=0;
  int srcc=sauces_res_get(&src,EGG_TID_tilesheet,imageid);
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,src,srcc)<0) return;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    switch (entry.tableid) {
      case NS_tilesheet_physics: memcpy(physics+entry.tileid,entry.v,entry.c); break;
      case NS_tilesheet_itemid: memcpy(tsitemid+entry.tileid,entry.v,entry.c); break;
    }
  }
}

static int session_get_item_tileid(const uint8_t *physics,const uint8_t *tsitemid,uint8_t itemid) {
  int tileid=0; for (;tileid<0x100;tileid++,physics++,tsitemid++) {
    if (*physics!=NS_physics_harvest) continue;
    if (*tsitemid!=itemid) continue;
    return tileid;
  }
  return -1;
}

void session_apply_plants(struct session *session) {
  int mapid=0,imageid=0;
  struct map *map;
  uint8_t physics[256],tsitemid[256];
  const struct plant *plant=session->plantv;
  int i=session->plantc;
  session->plantc=0;
  for (;i-->0;plant++) {
    if (!plant->mapid) continue;
    
    // Avoid reacquiring map and tilesheet for each plant.
    // The odds are very strong, if there's more than one plant, that they're all on the same map.
    // And even across maps, they might use the same image ie tilesheet. Loading tilesheets is expensive.
    if (plant->mapid!=mapid) {
      mapid=plant->mapid;
      map=session_get_map(session,mapid);
    }
    if (!map) continue;
    if ((plant->x<0)||(plant->y<0)||(plant->x>=map->w)||(plant->y>=map->h)) continue;
    int nimageid=session_get_map_imageid(map);
    if (nimageid!=imageid) {
      imageid=nimageid;
      session_read_tilesheet(physics,tsitemid,imageid);
    }
    
    int cellp=plant->y*map->w+plant->x;
    uint8_t tileid=map->v[cellp];
    if (physics[tileid]!=NS_physics_seed) continue;
    int ntileid=session_get_item_tileid(physics,tsitemid,plant->itemid);
    if (ntileid<0) {
      fprintf(stderr,"tilesheet:%d has no plant tile for item 0x%02x\n",imageid,plant->itemid);
      continue;
    }
    map->v[cellp]=ntileid;
  }
}

/* Summarize inventory.
 */
 
void session_summarize_inventory(struct invsum *invsum,const struct session *session) {
  memset(invsum,0,sizeof(struct invsum));
  const uint8_t *itemid=session->inventory;
  int i=INVENTORY_SIZE;
  int saucec=0;
  for (;i-->0;itemid++) {
    if (!*itemid) continue;
    invsum->invc++;
    const struct item *item=itemv+(*itemid);
    switch (item->foodgroup) {
      case NS_foodgroup_veg: invsum->vegc+=item->density; break;
      case NS_foodgroup_meat: invsum->meatc+=item->density; break;
      case NS_foodgroup_candy: invsum->candyc+=item->density; break;
      case NS_foodgroup_sauce: saucec+=item->density; break;
    }
  }
  if (saucec) {
    saucec/=3; // Sauce densities will always be a multiple of 3.
    invsum->vegc+=saucec;
    invsum->meatc+=saucec;
    invsum->candyc+=saucec;
  }
}
