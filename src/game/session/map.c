#include "game/sauces.h"
#include "map.h"
#include "opt/rom/rom.h"

const uint8_t dummy_tilesheet[256]={0};

/* Cleanup.
 */
 
void map_cleanup(struct map *map) {
  if (map->v) free(map->v);
  if (map->poiv) free(map->poiv);
  memset(map,0,sizeof(struct map));
}

/* POI list internals.
 */
 
static int map_poiv_search(const struct map *map,int col,int row) {
  int lo=0,hi=map->poic;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct poi *q=map->poiv+ck;
         if (row<q->row) hi=ck;
    else if (row>q->row) lo=ck+1;
    else if (col<q->col) hi=ck;
    else if (col>q->col) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct poi *map_poiv_insert(struct map *map,int p,int col,int row) {
  if ((p<0)||(p>map->poic)) return 0;
  if (map->poic>=map->poia) {
    int na=map->poia+16;
    if (na>INT_MAX/sizeof(struct poi)) return 0;
    void *nv=realloc(map->poiv,sizeof(struct poi)*na);
    if (!nv) return 0;
    map->poiv=nv;
    map->poia=na;
  }
  struct poi *poi=map->poiv+p;
  memmove(poi+1,poi,sizeof(struct poi)*(map->poic-p));
  map->poic++;
  memset(poi,0,sizeof(struct poi));
  poi->col=col;
  poi->row=row;
  return poi;
}

/* Load POI or other initial commands.
 */
 
static int map_load_poi(struct map *map) {
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.argc<2) continue;
    switch (cmd.opcode) {
      case CMD_map_home:
      case CMD_map_condensery:
      case CMD_map_shop:
      case CMD_map_door:
        {
          int p=map_poiv_search(map,cmd.argv[0],cmd.argv[1]);
          if (p<0) p=-p-1;
          struct poi *poi=map_poiv_insert(map,p,cmd.argv[0],cmd.argv[1]);
          if (!poi) return -1;
          poi->opcode=cmd.opcode;
          poi->argv=cmd.argv;
          poi->argc=cmd.argc;
        } break;
      case CMD_map_image: map->imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_map_trappable: {
          if (map->trappablec>=MAP_TRAPPABLE_LIMIT) {
            fprintf(stderr,"map:%d: Too many trappables. Limit %d.\n",map->rid,MAP_TRAPPABLE_LIMIT);
            break;
          }
          struct trappable *t=map->trappablev+map->trappablec++;
          t->itemid=cmd.argv[0];
          t->odds=cmd.argv[1];
        } break;
    }
  }
  return 0;
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
  map->physics=dummy_tilesheet;
  map->itemid=dummy_tilesheet;
  if (map_load_poi(map)<0) {
    map_cleanup(map);
    return -1;
  }
  return 0;
}

/* Get all POI for one cell.
 */
 
int map_get_poi(struct poi **v,const struct map *map,int col,int row) {
  int p=map_poiv_search(map,col,row);
  if (p<0) return 0;
  int c=1;
  while ((p>0)&&(map->poiv[p-1].col==col)&&(map->poiv[p-1].row==row)) { p--; c++; }
  while ((p+c<map->poic)&&(map->poiv[p+c].col==col)&&(map->poiv[p+c].row==row)) c++;
  *v=map->poiv+p;
  return c;
}
