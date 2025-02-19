/* kitchen.h
 * Model for the nocturnal phase of gameplay.
 */
 
#ifndef KITCHEN_H
#define KITCHEN_H

#include "game/session/session.h"

#define KITCHEN_OUTCOME_PENDING 0
#define KITCHEN_OUTCOME_UNFED   1
#define KITCHEN_OUTCOME_DEAD    2
#define KITCHEN_OUTCOME_SAD     3
#define KITCHEN_OUTCOME_HAPPY   4

#define ADD_CUSTOMER_LIMIT 3

struct kitchen {
  
  /* (kcustomer) are indexed by their seat.
   * All occupied seats correspond to one of (g.session.customerv), but not at the same index.
   */
  struct kcustomer {
    uint8_t race; // 0 if unoccupied.
    int sesp; // Index in (g.session->customerv) or <0.
    uint8_t outcome;
  } kcustomerv[SESSION_CUSTOMER_LIMIT];
  
  uint8_t selected[INVENTORY_SIZE]; // Nonzero for each selected inventory slot.
  double clock; // Counts up.
  double clock_limit;
};

void kitchen_del(struct kitchen *kitchen);
struct kitchen *kitchen_new();

/* Nonzero if there is something obviously wrong with the selected ingredients,
 * to the extent that we should require the user to confirm before cooking.
 */
int kitchen_requires_approval(const struct kitchen *kitchen);
#define KITCHEN_PRE_APPROVED      0 /* Sure, cook it. */
#define KITCHEN_APPROVAL_EMPTY    1 /* No ingredients. */
#define KITCHEN_APPROVAL_POISON   2 /* Poisonous. */
#define KITCHEN_APPROVAL_PRECIOUS 3 /* Contains tools you probably don't want to lose, eg shovel. */

/* Generate canned advice.
 * Returns an index in strings:ui.
 * (*severity) we set to (1,2,3)=(alarm,approve,assist).
 */
int kitchen_assess(int *severity,const struct kitchen *kitchen);

/* Populate (stew) from (kitchen) and the global session.
 * This is strictly read-only. We tell you everything about state that is going to change, but don't do any of it yet.
 * The EOD layer should begin by cooking, and use this stew to pay out its reporting of the day's outcome.
 * At any point after cooking, you can commit the stew to the global session.
 * Ensure that that happen exactly once per day!
 */
struct stew {
  const struct item *itemv[INVENTORY_SIZE];
  uint8_t invpv[INVENTORY_SIZE]; // Also record the inventory positions of items, so we can easily remove at commit.
  int itemc;
  int helpingc,helpingc_sauce; // Sum of densities. Also the sauce density, because it doesn't have to count.
  int vegc,meatc,candyc,saucec,poisonc,otherc; // Count of natural ingredients by food group. (add up to itemc).
  int manc,rabbitc,octopusc,werewolfc; // Count of initial customers by race. Princess counts as Man.
  struct kcustomer kcbeforev[SESSION_CUSTOMER_LIMIT]; // From kitchen, but packed and (outcome) populated.
  struct kcustomer kcaddv[ADD_CUSTOMER_LIMIT]; // New customers being added for tomorrow.
  int kcbeforec,kcaddc;
  int unfedc,deadc,sadc,happyc; // Sum of (kcbeforev[].outcome), for convenience.
  int kcrmc; // (unfedc+deadc+sadc)
  int addreasonv[ADD_CUSTOMER_LIMIT]; // Index in strings:ui saying the reason each of (kcaddv) is added.
  double clock,clock_limit;
};
void kitchen_cook(struct stew *stew,const struct kitchen *kitchen);
void kitchen_commit_stew(const struct stew *stew);

double clock_limit_by_day(int day);

/* Hourglass is the clock visible in the upper left corner.
 * It's strictly a view, it does not perform the actual timekeeping.
 * (t) in 0..1, or >1 if finished.
 * (remaining) counts down in absolute seconds, and negative if finished.
 */
struct hourglass;
struct hourglass *hourglass_new();
void hourglass_del(struct hourglass *hg);
void hourglass_render(int dstx,int dsty,struct hourglass *hg,double t,double remaining);

/* Bobbler is a view of the cauldron, with your ingredients floating around in it.
 * This doesn't convey any new information (the pantry says everything), but it's a big part of the pizzazz.
 */
struct bobbler;
void bobbler_del(struct bobbler *bobbler);
struct bobbler *bobbler_new();
void bobbler_render_surface(int dstx,int dsty,struct bobbler *bobbler);
void bobbler_render_overlay(int dstx,int dsty,struct bobbler *bobbler);

#endif
