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

#define KITCHEN_OUTCOME_PENDING   0
#define KITCHEN_OUTCOME_UNFED     1
#define KITCHEN_OUTCOME_DEAD      2
#define KITCHEN_OUTCOME_UNHAPPY   3
#define KITCHEN_OUTCOME_SATISFIED 4
#define KITCHEN_OUTCOME_ECSTATIC  5

struct kitchen {
  int size; // DINING_ROOM_*
  struct kcustomer {
    uint8_t race; // 0 if unoccupied.
    int sesp; // Index in (g.session->customerv) or <0.
    uint8_t outcome;
  } *kcustomerv;
  uint8_t selected[INVENTORY_SIZE]; // Nonzero for each selected inventory slot.
};

void kitchen_del(struct kitchen *kitchen);
struct kitchen *kitchen_new();

/* Ensure that each non-vacant kcustomer has a definite outcome.
 * This is where the principal game logic lives.
 * Nothing in the session is changed.
 */
void kitchen_apply(struct kitchen *kitchen);

/* If we've been applied, now apply those changes to the global session.
 * Items will be removed from inventory, customers removed or added, and losses rewritten.
 */
void kitchen_commit_to_session(struct kitchen *kitchen);

/* Nonzero if there is something obviously wrong with the selected ingredients,
 * to the extent that we should require the user to confirm before cooking.
 */
int kitchen_requires_approval(const struct kitchen *kitchen);
#define KITCHEN_PRE_APPROVED      0 /* Sure, cook it. */
#define KITCHEN_APPROVAL_EMPTY    1 /* No ingredients. */
#define KITCHEN_APPROVAL_POISON   2 /* Poisonous. */
#define KITCHEN_APPROVAL_PRECIOUS 3 /* Contains tools you probably don't want to lose, eg shovel. */

#endif
