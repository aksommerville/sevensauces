/* session.h
 * Model for gameplay things that persist across days.
 * There is one session between visits to the main menu.
 */
 
#ifndef SESSION_H
#define SESSION_H

#include "map.h"

#define SESSION_MAP_LIMIT 8 /* Can change freely but must be at least the count of map resources. */
#define INVENTORY_SIZE 16 /* Do not change. */
#define SESSION_PLANT_LIMIT 64 /* Completely arbitrary. I think 64 will be pretty hard to reach. */
#define SESSION_CUSTOMER_LIMIT 23 /* The rules and the kitchen layout are dependent on 23. Changing would be a big deal. */

struct session {
  int day; // 0..7, if 7 we're done playing
  uint8_t inventory[INVENTORY_SIZE]; // itemid
  uint8_t invp; // 0..15, which slot is selected
  uint16_t gold; // 0..999
  
  struct map mapv[SESSION_MAP_LIMIT];
  int mapc;
  
  /* Plants get recorded here one day, and applied at the start of the next.
   * Session manages that for the most part.
   * You must point each plant to a 'seed' cell. If it's something else it will be ignored.
   * If you need to neutralize a plant, set its mapid or position OOB.
   */
  struct plant {
    int mapid,x,y,itemid;
  } plantv[SESSION_PLANT_LIMIT];
  int plantc;
  
  /* (customerv) only includes real customers, ones that will show up tonight.
   */
  struct customer {
    uint8_t race; // NS_race_*
  } customerv[SESSION_CUSTOMER_LIMIT];
  int customerc;
  int custover[4]; // Overflow customer count by race (-1). Relevant only for scoring the final night.
  
  /* (lossv) gets populated at the end of night, with removed customers who can be apologized to.
   * If that apology happens, remove them from here and add to (customerv).
   */
  struct loss {
    uint8_t race;
    int mapid,x,y;
  } lossv[SESSION_CUSTOMER_LIMIT];
  int lossc;
};

void session_del(struct session *session);
struct session *session_new();

struct map *session_get_map(struct session *session,int mapid);
int session_get_free_inventory_slot(const struct session *session);
int session_count_free_inventory_slots(const struct session *session);

/* Losing a customer removes it but also adds a loss.
 * Those are ones that you can recover the next day by apologizing.
 * Lose and remove don't shuffle the list immediately -- You have to session_commit_removals() after.
 * That affords kitchen a window in which its collected customer indices won't change.
 */
struct customer *session_add_customer(struct session *session,int race); // race zero for random.
void session_remove_customer_at(struct session *session,int p);
void session_lose_customer_at(struct session *session,int p);
void session_commit_removals(struct session *session);

/* Acquire a new item.
 * If you provide either of (cb,userdata), we are allowed to create passive feedback. (sound effect and toast).
 * If you provide (cb) and no slot is available, we'll spawn a modal to ask whether to drop an item.
 * Returns >0 if added, <0 if rejected, or 0 if deferred.
 */
int session_acquire_item(struct session *session,uint8_t itemid,void (*cb)(int invp,void *userdata),void *userdata);

struct plant *session_add_plant(struct session *session);
void session_apply_plants(struct session *session);

/* Nonzero to continue playing. ie we have at least one customer or loss.
 * If this returns zero at the end of a night, it's Game Over.
 */
int session_may_proceed(const struct session *session);

/* Veg, meat, and candy counts include the sauce count divided by 3 -- sauce counts as all 3 groups.
 */
struct invsum {
  int invc; // Count of items in inventory.
  int vegc; // Helpings of vegetables (sum of densities).
  int meatc; // '' meat
  int candyc; // '' candy
};
void session_summarize_inventory(struct invsum *invsum,const struct session *session);

struct item {
  uint8_t flags;
  uint8_t foodgroup;
  uint8_t density; // Must be zero for inedibles.
  uint8_t usage;
  uint8_t harvestq; // For harvestable things, how many at a time? In general, 3 for flora and 1 for fauna.
};
extern const struct item itemv[256];

#endif
