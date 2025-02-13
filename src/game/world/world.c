#include "game/sauces.h"
#include "game/session/session.h"
#include "game/sprite/sprite.h"
#include "world.h"
#include "opt/rom/rom.h"

/* Delete.
 */
 
void world_del(struct world *world) {
  if (!world) return;
  sprites_del(world->sprites);
  free(world);
  if (world==g.world) g.world=0;
}

/* Init, fallible parts.
 */
 
static int world_init(struct world *world) {
  if (!g.world) g.world=world;
  if (!(world->sprites=sprites_new())) return -1;
  if (world_load_map(world,RID_map_village)<0) return -1;
  return 0;
}

/* New.
 */
 
struct world *world_new() {
  struct world *world=calloc(1,sizeof(struct world));
  if (!world) return 0;
  
  world->clock=WORLD_PLAY_TIME;
  
  if (world_init(world)<0) {
    world_del(world);
    return 0;
  }
  return world;
}

/* Generate a sprite for any loss on the current map.
 */
 
static void world_generate_loss_sprites(struct world *world) {
  int i=g.session->lossc;
  struct loss *loss=g.session->lossv+i-1;
  for (;i-->0;loss--) {
    if (loss->mapid!=world->map->rid) continue;
    struct sprite *sprite=sprite_new(&sprite_type_loss,world->sprites,loss->x+0.5,loss->y+0.5,loss->race|(i<<8),0);
  }
}

/* Generate sprites for forages.
 */
 
static void world_generate_forage_sprites(struct world *world) {
  struct forage *forage=world->map->foragev;
  int i=world->map->foragec;
  for (;i-->0;forage++) {
    const struct item *item=itemv+forage->itemid;
    double x=forage->x+0.5,y=forage->y+0.5;
    switch (item->usage) {
      case NS_itemusage_drop:
      case NS_itemusage_stone: {
          struct sprite *sprite=sprite_new(0,world->sprites,x,y,(forage->itemid<<24)|(forage->forageid<<16),RID_sprite_item);
        } break;
      case NS_itemusage_free: {
          struct sprite *sprite=sprite_new(0,world->sprites,x,y,(forage->itemid<<24)|(forage->forageid<<16),RID_sprite_faun);
        } break;
    }
  }
}

/* Load map: Initial read of command list.
 */
 
struct world_load_map {
  uint8_t herox,heroy;
};

static int world_read_initial_map_commands(struct world_load_map *ctx,struct world *world) {
  struct rom_command_reader reader={.v=world->map->cmdv,.c=world->map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_image: world->map->imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_map_hero: ctx->herox=cmd.argv[0]; ctx->heroy=cmd.argv[1]; break;
      case CMD_map_sprite: {
          double x=cmd.argv[0]+0.5;
          double y=cmd.argv[1]+0.5;
          uint16_t rid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          sprite_new(0,world->sprites,x,y,arg,rid);
        } break;
      //TODO song
    }
  }
  return 0;
}

/* Load map.
 */
 
int world_load_map(struct world *world,int mapid) {
  struct map *map=session_get_map(g.session,mapid);
  if (!map) {
    fprintf(stderr,"map:%d not found\n",mapid);
    return -1;
  }
  
  struct sprite *hero=sprites_get_hero(world->sprites);
  sprites_unlist(world->sprites,hero);
  sprites_drop_all(world->sprites);
  sprites_list(world->sprites,hero);
  
  world->map=map;
  struct world_load_map ctx={0};
  if (world_read_initial_map_commands(&ctx,world)<0) return -1;
  
  if (!hero) {
    hero=sprite_new(&sprite_type_hero,world->sprites,ctx.herox+0.5,ctx.heroy+0.5,0,0);
  }
  
  world_generate_loss_sprites(world);
  world_generate_forage_sprites(world);
  
  return 0;
}

/* Update.
 */
 
void world_update(struct world *world,double elapsed) {
  if ((world->clock-=elapsed)<=0.0) return;
  sprites_update(world->sprites,elapsed);
}

/* Commit to session.
 */
 
void world_commit_to_session(struct world *world) {
}

/* Special tiles.
 */
 
int world_tileid_for_trapped_faun(const struct world *world,uint8_t itemid) {
  if (!itemid) return -1;
  const uint8_t *p=world->map->itemid;
  int tileid=0;
  for (;tileid<0x100;tileid++,p++) {
    if (*p==itemid) {
      if (world->map->physics[tileid]!=NS_physics_harvest) {
        fprintf(stderr,"tilesheet:%d: Tile 0x%02x has itemid 0x%02x but physics is not NS_physics_harvest (%d). Can't use it.\n",world->map->imageid,tileid,itemid,world->map->physics[tileid]);
        continue;
      }
      return tileid;
    }
  }
  return -1;
}

int world_tileid_for_trap(const struct world *world) {
  const uint8_t *p=world->map->physics;
  int tileid=0;
  for (;tileid<0x100;tileid++,p++) {
    if (*p==NS_physics_trap) return tileid;
  }
  return -1;
}

int world_tileid_for_seed(const struct world *world) {
  const uint8_t *p=world->map->physics;
  int tileid=0;
  for (;tileid<0x100;tileid++,p++) {
    if (*p==NS_physics_seed) return tileid;
  }
  return -1;
}

/* Public forage adjustments.
 */
 
void world_update_forage(struct world *world,uint8_t forageid,int col,int row) {
  if (!world||!world->map) return;
  if (col<0) col=0; else if (col>=world->map->w) col=world->map->w-1;
  if (row<0) row=0; else if (row>=world->map->h) row=world->map->h-1;
  int p=world->map->foragec;
  while (p-->0) {
    struct forage *forage=world->map->foragev+p;
    if (forage->forageid!=forageid) continue;
    forage->x=col;
    forage->y=row;
    return;
  }
}

void world_delete_forage(struct world *world,uint8_t forageid) {
  if (!world||!world->map) return;
  int p=world->map->foragec;
  while (p-->0) {
    struct forage *forage=world->map->foragev+p;
    if (forage->forageid!=forageid) continue;
    world->map->foragec--;
    memmove(forage,forage+1,sizeof(struct forage)*(world->map->foragec-p));
    return;
  }
}
