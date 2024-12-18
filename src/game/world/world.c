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

/* Load tilesheet, after initial read of map.
 */
 
static int world_load_tilesheet(struct world *world) {
  memset(world->physics,0,sizeof(world->physics));
  memset(world->tsitemid,0,sizeof(world->tsitemid));
  const void *src=0;
  int srcc=sauces_res_get(&src,EGG_TID_tilesheet,world->map_imageid);
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    switch (entry.tableid) {
      case NS_tilesheet_physics: memcpy(world->physics+entry.tileid,entry.v,entry.c); break;
      case NS_tilesheet_itemid: memcpy(world->tsitemid+entry.tileid,entry.v,entry.c); break;
    }
  }
  return 0;
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
      case CMD_map_image: world->map_imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_map_hero: ctx->herox=cmd.argv[0]; ctx->heroy=cmd.argv[1]; break;
      case CMD_map_sprite: {
          double x=cmd.argv[0]+0.5;
          double y=cmd.argv[1]+0.5;
          uint16_t rid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          sprite_new(0,world->sprites,x,y,arg,rid);
        } break;
      //TODO song
      //TODO other things to index at load... doors?
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
  world_load_tilesheet(world);
  
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
