/* map.h
 * Live model corresponding to a map resource.
 * Maps are loaded with each new session.
 * Cells are mutable.
 */
 
#ifndef MAP_H
#define MAP_H

#define MAP_TRAPPABLE_LIMIT 8
#define MAP_FORAGE_LIMIT 16

struct map {
  int rid;
  int w,h;
  uint8_t *v;
  const uint8_t *ro; // Matches (v), but the original read-only content.
  const uint8_t *cmdv;
  int cmdc;
  int imageid;
  const uint8_t *physics; // From tilesheet.
  const uint8_t *itemid;
  
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
  
  struct trappable {
    uint8_t itemid;
    uint8_t odds;
  } trappablev[MAP_TRAPPABLE_LIMIT];
  int trappablec;
  
  /* Each of these should cause a sprite to spawn at load.
   * It gets rebuilt each day by session.
   * World may change positions or eliminate members at any time.
   * (forageid) is an arbitrary integer unique across the day's forages.
   */
  struct forage {
    uint8_t itemid;
    uint8_t x,y;
    uint8_t forageid;
  } foragev[MAP_FORAGE_LIMIT];
  int foragec;
};

void map_cleanup(struct map *map);
int map_init(struct map *map,int rid,const void *src,int srcc);

int map_get_poi(struct poi **v,const struct map *map,int col,int row);

#endif
