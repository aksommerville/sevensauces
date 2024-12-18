/* session.h
 * Model for gameplay things that persist across days.
 * There is one session between visits to the main menu.
 */
 
#ifndef SESSION_H
#define SESSION_H

#include "map.h"

#define SESSION_MAP_LIMIT 8 /* Can change freely but must be at least the count of map resources. */
#define INVENTORY_SIZE 16 /* Do not change. */

struct session {
  int day; // 0..7, if 7 we're done playing
  uint8_t inventory[INVENTORY_SIZE]; // itemid
  uint8_t invp; // 0..15, which slot is selected
  
  struct map mapv[SESSION_MAP_LIMIT];
  int mapc;
};

void session_del(struct session *session);
struct session *session_new();

struct map *session_get_map(struct session *session,int mapid);
int session_get_free_inventory_slot(const struct session *session);

/* Acquire a new item.
 * If you provide either of (cb,userdata), we are allowed to create passive feedback. (sound effect and toast).
 * If you provide (cb) and no slot is available, we'll spawn a modal to ask whether to drop an item.
 * Returns >0 if added, <0 if rejected, or 0 if deferred.
 */
int session_acquire_item(struct session *session,uint8_t itemid,void (*cb)(int invp,void *userdata),void *userdata);

struct item {
  uint8_t flags;
  uint8_t foodgroup;
  uint8_t density;
  uint8_t usage;
};
extern const struct item itemv[256];

#endif
