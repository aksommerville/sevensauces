/* map.h
 * Live model corresponding to a map resource.
 * Maps are loaded with each new session.
 * Cells are mutable.
 */
 
#ifndef MAP_H
#define MAP_H

struct map {
  int rid;
  int w,h;
  uint8_t *v;
  const uint8_t *ro; // Matches (v), but the original read-only content.
  const uint8_t *cmdv;
  int cmdc;
  
  /* Commands that may be triggered by a hero step are yoinked at load time and stored here,
   * sorted by (row,col).
   */
  struct poi {
    uint8_t row,col;
    uint8_t opcode;
    const uint8_t *argv; // Includes the leading (u16:pos) that all POI start with.
    int argc;
  } *poiv;
  int poic,poia;
};

void map_cleanup(struct map *map);
int map_init(struct map *map,int rid,const void *src,int srcc);

int map_get_poi(struct poi **v,const struct map *map,int col,int row);

#endif
