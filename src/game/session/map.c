#include "game/sauces.h"
#include "map.h"
#include "opt/rom/rom.h"

/* Cleanup.
 */
 
void map_cleanup(struct map *map) {
  if (map->v) free(map->v);
  memset(map,0,sizeof(struct map));
}

/* Decode.
 */
 
int map_init(struct map *map,int rid,const void *src,int srcc) {
  struct rom_map rmap;
  if (rom_map_decode(&rmap,src,srcc)<0) return -1;
  memset(map,0,sizeof(struct map));
  int cellc=rmap.w*rmap.h;
  if (!(map->v=malloc(cellc))) return -1;
  memcpy(map->v,rmap.v,cellc);
  map->rid=rid;
  map->w=rmap.w;
  map->h=rmap.h;
  map->ro=rmap.v;
  map->cmdv=rmap.cmdv;
  map->cmdc=rmap.cmdc;
  return 0;
}
