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
};

void map_cleanup(struct map *map);
int map_init(struct map *map,int rid,const void *src,int srcc);

#endif
