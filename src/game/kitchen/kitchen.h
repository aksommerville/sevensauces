/* kitchen.h
 * Model for the nocturnal phase of gameplay.
 */
 
#ifndef KITCHEN_H
#define KITCHEN_H

/* The dining room size constants can be treated like an enum,
 * but their value is also the room's capacity, and the length of (kitchen.kcustomerv).
 */
#define DINING_ROOM_SMALL 23
#define DINING_ROOM_LARGE 320

struct kitchen {
  int size; // DINING_ROOM_*
  struct kcustomer {
    uint8_t race; // 0 if unoccupied.
    int sesp; // Index in (g.session->customerv) or <0.
  } *kcustomerv;
};

void kitchen_del(struct kitchen *kitchen);
struct kitchen *kitchen_new();

void kitchen_commit_to_session(struct kitchen *kitchen);

#endif
