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
  int i=session->mapc;
  while (i-->0) map_cleanup(session->mapv+i);
  if (session->tilesheetv) {
    while (session->tilesheetc-->0) {
      struct tilesheet *tilesheet=session->tilesheetv+session->tilesheetc;
      if (tilesheet->physics) free(tilesheet->physics);
      if (tilesheet->itemid) free(tilesheet->itemid);
    }
    free(session->tilesheetv);
  }
  free(session);
}

/* Add customer.
 */
 
struct customer *session_add_customer(struct session *session,int race) {
  if (!race) race=1+rand()%4;
  if ((race<1)||(race>5)) return 0;
  if (session->customerc>=SESSION_CUSTOMER_LIMIT) {
    if ((race>=1)&&(race<=4)) session->custover[race-1]++;
    return 0;
  }
  struct customer *customer=session->customerv+session->customerc++;
  memset(customer,0,sizeof(struct customer));
  customer->race=race;
  return customer;
}

/* Select (mapid,x,y) for a new loss.
 * I'm thinking it will always be the village map, and any vacant cell.
 */
 
static void session_select_loss_location(struct session *session,struct loss *loss) {
  loss->mapid=RID_map_village;
  struct map *map=session_get_map(session,loss->mapid);
  if (!map) { loss->mapid=0; return; }
  uint16_t candidatev[1024];
  int candidatec=0;
  const uint8_t *p=map->v;
  int i=map->w*map->h;
  for (;i-->0;p++) {
    uint8_t physics=map->physics[*p];
    if (physics==NS_physics_diggable) { // "diggable" not "vacant". This way they never end up in doorways or other special spots.
      candidatev[candidatec]=p-map->v;
      if (++candidatec>=sizeof(candidatev)) break;
    }
  }
  if (candidatec<1) { loss->mapid=0; return; }
  int choice=candidatev[rand()%candidatec];
  loss->x=choice%map->w;
  loss->y=choice/map->w;
}

/* Remove customer.
 */
 
void session_remove_customer_at(struct session *session,int p) {
  if (!session||(p<0)||(p>=session->customerc)) return;
  struct customer *customer=session->customerv+p;
  customer->race=0;
}

void session_lose_customer_at(struct session *session,int p) {
  if (!session||(p<0)||(p>=session->customerc)) return;
  struct customer *customer=session->customerv+p;
  if (session->lossc<SESSION_CUSTOMER_LIMIT) {
    struct loss *loss=session->lossv+session->lossc++;
    memset(loss,0,sizeof(struct loss));
    loss->race=customer->race;
    session_select_loss_location(session,loss);
  }
  customer->race=0;
}

void session_commit_removals(struct session *session) {
  struct customer *customer=session->customerv+session->customerc-1;
  int i=session->customerc;
  for (;i-->0;customer--) {
    if (!customer->race) {
      session->customerc--;
      memmove(customer,customer+1,sizeof(struct customer)*(session->customerc-i));
    }
  }
}

void session_unlose_customer_at(struct session *session,int lossp) {
  if (!session||(lossp<0)||(lossp>=session->lossc)) return;
  struct loss *loss=session->lossv+lossp;
  if (!loss->race||!loss->mapid) return;
  session_add_customer(session,loss->race);
  loss->mapid=0; // Prevents it being used again. Don't shuffle the list; loss sprites might have recorded their position in it.
}

/* Add tilesheet.
 */
 
static int session_add_tilesheet(struct session *session,int rid,const uint8_t *src,int srcc) {
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  if (session->tilesheetc>=session->tilesheeta) {
    int na=session->tilesheeta+16;
    if (na>INT_MAX/sizeof(struct tilesheet)) return -1;
    void *nv=realloc(session->tilesheetv,sizeof(struct tilesheet)*na);
    if (!nv) return -1;
    session->tilesheetv=nv;
    session->tilesheeta=na;
  }
  struct tilesheet *tilesheet=session->tilesheetv+session->tilesheetc++;
  memset(tilesheet,0,sizeof(struct tilesheet));
  tilesheet->rid=rid;
  if (!(tilesheet->physics=calloc(1,256))) return -1;
  if (!(tilesheet->itemid=calloc(1,256))) return -1;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    switch (entry.tableid) {
      case NS_tilesheet_physics: memcpy(tilesheet->physics+entry.tileid,entry.v,entry.c); break;
      case NS_tilesheet_item: memcpy(tilesheet->itemid+entry.tileid,entry.v,entry.c); break;
    }
  }
  return 0;
}

/* Find tilesheet.
 */
 
static struct tilesheet *session_get_tilesheet(struct session *session,int rid) {
  int lo=0,hi=session->tilesheetc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct tilesheet *tilesheet=session->tilesheetv+ck;
         if (rid<tilesheet->rid) hi=ck;
    else if (rid>tilesheet->rid) lo=ck+1;
    else return tilesheet;
  }
  return 0;
}

/* Init, the fallible parts.
 */
 
static int session_init(struct session *session) {
  session->nonce=rand();

  /* Gather resources.
   */
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
      case EGG_TID_tilesheet: {
          if (session_add_tilesheet(session,res->rid,res->v,res->c)<0) return -1;
        } break;
    }
  }
  struct map *map=session->mapv;
  int i=session->mapc;
  for (;i-->0;map++) {
    struct tilesheet *tilesheet=session_get_tilesheet(session,map->imageid);
    if (tilesheet) {
      map->physics=tilesheet->physics;
      map->itemid=tilesheet->itemid;
    }
  }
  
  /* Initial customer set.
   */
  session_add_customer(session,NS_race_man);
  session_add_customer(session,NS_race_rabbit);
  session_add_customer(session,NS_race_octopus);
  session_add_customer(session,NS_race_werewolf);
  
  session->gold=10;
  
  /*XXX Start with the works. *
  session->inventory[ 0]=NS_item_shovel;
  session->inventory[ 1]=NS_item_bow;
  session->inventory[ 2]=NS_item_grapple;
  session->inventory[ 3]=NS_item_fishpole;
  session->inventory[ 4]=NS_item_goat;
  session->inventory[ 5]=NS_item_cow;
  session->inventory[ 6]=NS_item_rabbit;
  session->inventory[ 7]=NS_item_rat;
  session->inventory[ 8]=NS_item_carrot;
  session->inventory[ 9]=NS_item_pumpkin;
  session->inventory[10]=NS_item_bread;
  session->inventory[11]=NS_item_brightsauce;
  session->inventory[12]=NS_item_peppermint;
  session->inventory[13]=NS_item_buckeye;
  session->inventory[14]=NS_item_applepie;
  session->inventory[15]=NS_item_arsenic;
  /**/
  
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
  const struct plant *plant=session->plantv;
  int i=session->plantc;
  session->plantc=0;
  for (;i-->0;plant++) {
    if (!plant->mapid) continue;
    
    // Avoid reacquiring map and tilesheet for each plant.
    // The odds are very strong, if there's more than one plant, that they're all on the same map.
    if (plant->mapid!=mapid) {
      mapid=plant->mapid;
      map=session_get_map(session,mapid);
    }
    if (!map) continue;
    if ((plant->x<0)||(plant->y<0)||(plant->x>=map->w)||(plant->y>=map->h)) continue;
    
    int cellp=plant->y*map->w+plant->x;
    uint8_t tileid=map->v[cellp];
    if (map->physics[tileid]!=NS_physics_seed) continue;
    int ntileid=session_get_item_tileid(map->physics,map->itemid,plant->itemid);
    if (ntileid<0) {
      fprintf(stderr,"tilesheet:%d has no plant tile for item 0x%02x\n",imageid,plant->itemid);
      continue;
    }
    map->v[cellp]=ntileid;
  }
}

/* Populate traps.
 */
 
static void session_populate_trap(uint8_t *dst,struct session *session,struct map *map) {
  if (map->trappablec<1) {
    fprintf(stderr,"Trap was placed on map:%d but it has no trappables. Leaving unpopulated.\n",map->rid);
    return;
  }
  int range=1,i=map->trappablec;
  const struct trappable *t=map->trappablev;
  for (;i-->0;t++) range+=t->odds;
  int choice=rand()%range;
  uint8_t itemid=map->trappablev[0].itemid;
  for (t=map->trappablev,i=map->trappablec;i-->0;t++) {
    choice-=t->odds;
    if (choice<=0) {
      itemid=t->itemid;
      break;
    }
  }
  int tileid=0x100;
  while (tileid-->0) {
    if (map->itemid[tileid]!=itemid) continue;
    if (map->physics[tileid]!=NS_physics_harvest) continue;
    *dst=tileid;
    return;
  }
  fprintf(stderr,"Selected item %d for trap on map:%d but there isn't a harvest tile for it.\n",itemid,map->rid);
}
 
void session_populate_traps(struct session *session) {
  // We're going to examine every cell of every map looking for traps... TODO Consider making a list of them as they get placed.
  struct map *map=session->mapv;
  int i=session->mapc;
  for (;i-->0;map++) {
    uint8_t *v=map->v;
    int celli=map->w*map->h;
    for (;celli-->0;v++) {
      if (map->physics[*v]==NS_physics_trap) {
        session_populate_trap(v,session,map);
      }
    }
  }
}

/* Wipe forageables from all the maps and select a new distribution (start of day).
 */
 
static const uint8_t foragev[]={
  NS_item_rabbit,
  NS_item_rabbit,
  NS_item_rabbit,
  NS_item_rat,
  NS_item_rat,
  NS_item_rat,
  NS_item_rat,
  NS_item_quail,
  NS_item_quail,
  NS_item_quail,
  NS_item_quail,
  NS_item_quail,
  NS_item_goat,
  NS_item_goat,
  NS_item_goat,
  NS_item_pig,
  NS_item_pig,
  NS_item_sheep,
  NS_item_sheep,
  NS_item_sheep,
  NS_item_deer,
  NS_item_deer,
  NS_item_deer,
  NS_item_carrot,
  NS_item_carrot,
  NS_item_carrot,
  NS_item_potato,
  NS_item_potato,
  NS_item_cabbage,
  NS_item_cabbage,
  NS_item_banana,
  NS_item_banana,
  NS_item_banana,
  NS_item_watermelon,
  NS_item_watermelon,
  NS_item_pumpkin,
  NS_item_pumpkin,
  NS_item_buckeye,
  NS_item_stone,
  NS_item_stone,
  NS_item_stone,
  NS_item_stone,
  NS_item_stone,
  NS_item_stone,
};
 
static int session_get_forage(void *dstpp,int day) {
  // Allowing that we might eventually have different forage sets for different days.
  *(const void**)dstpp=foragev;
  return sizeof(foragev);
}
 
static int map_is_forageable(struct map *map) {
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode==CMD_map_forage) return 1;
  }
  return 0;
}

static void session_add_forage(struct map *map,uint8_t itemid,uint8_t forageid) {
  if (map->foragec>MAP_FORAGE_LIMIT) return;
  
  // Forage can go on any diggable cell. If we don't find one in a timely manner, forget it.
  int panic=50,x,y;
  for (;;) {
    if (--panic<0) return;
    x=rand()%map->w;
    y=rand()%map->h;
    if (map->physics[map->v[y*map->w+x]]==NS_physics_diggable) break;
  }
  
  struct forage *forage=map->foragev+map->foragec++;
  forage->itemid=itemid;
  forage->forageid=forageid;
  forage->x=x;
  forage->y=y;
}
 
void session_reset_forage(struct session *session) {

  /* Blank all forage lists and collect the set of eligible maps.
   */
  struct map *fmapv[SESSION_MAP_LIMIT];
  int fmapc=0;
  struct map *map=session->mapv;
  int i=session->mapc;
  for (;i-->0;map++) {
    map->foragec=0;
    if (map_is_forageable(map)) fmapv[fmapc++]=map;
  }
  if (fmapc<1) return;
  
  /* Place each forage item in any of the maps.
   * It's OK to skip an item. If the map refuses it, whatever.
   * TODO We probably ought to refine the selection a bit further.
   * eg Forest should accrue more forage than Desert, and Beach should get lobsters instead of squirrels...
   */
  const uint8_t *itemidv;
  int itemidc=session_get_forage(&itemidv,session->day);
  for (;itemidc-->0;itemidv++) {
    int fmapp=rand()%fmapc;
    session_add_forage(fmapv[fmapp],*itemidv,itemidc+1);
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

/* Check for early game-over.
 */
 
int session_may_proceed(const struct session *session) {
  if (!session) return 0;
  if (session->customerc) return 1;
  if (session->lossc) return 1;
  return 0;
}
