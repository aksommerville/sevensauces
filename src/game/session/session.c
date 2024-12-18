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
