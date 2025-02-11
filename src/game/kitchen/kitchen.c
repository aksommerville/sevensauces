#include "game/sauces.h"
#include "game/session/session.h"
#include "kitchen.h"

/* Delete.
 */
 
void kitchen_del(struct kitchen *kitchen) {
  if (!kitchen) return;
  free(kitchen);
}

/* Initialize a kcustomer from one of the session's customers.
 * Vacant seats don't get initialized; they start with (race==0) which means vacant.
 */
 
static void kitchen_init_customer(struct kcustomer *dst,const struct customer *src) {
  dst->race=src->race;
  if ((dst->sesp=src-g.session->customerv)>=g.session->customerc) dst->sesp=-1;
}

/* Init, fallible parts.
 */
 
static int kitchen_init(struct kitchen *kitchen) {
  uint8_t lut[SESSION_CUSTOMER_LIMIT];
  int i=SESSION_CUSTOMER_LIMIT;
  while (i-->0) lut[i]=i;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  for (i=SESSION_CUSTOMER_LIMIT;i-->0;kcustomer++) {
    int lutp=rand()%(i+1);
    int cp=lut[lutp];
    memmove(lut+lutp,lut+lutp+1,i-lutp);
    if (cp<g.session->customerc) kitchen_init_customer(kcustomer,g.session->customerv+cp);
  }
  return 0;
}

/* New.
 */
 
struct kitchen *kitchen_new() {
  if (!g.session) return 0;
  struct kitchen *kitchen=calloc(1,sizeof(struct kitchen));
  if (!kitchen) return 0;
  if (kitchen_init(kitchen)<0) {
    kitchen_del(kitchen);
    return 0;
  }
  return kitchen;
}

/* Point out obvious flaws in Dot's plan.
 */
 
int kitchen_requires_approval(const struct kitchen *kitchen) {
  if (!kitchen) return KITCHEN_PRE_APPROVED;
  int ingredientc=0,invp=0;
  for (;invp<INVENTORY_SIZE;invp++) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    switch (item->foodgroup) {
      case NS_foodgroup_veg:
      case NS_foodgroup_meat:
      case NS_foodgroup_candy:
      case NS_foodgroup_sauce: {
          ingredientc++;
        } break;
      case NS_foodgroup_poison: return KITCHEN_APPROVAL_POISON;
    }
    if (item->flags&(1<<NS_itemflag_precious)) return KITCHEN_APPROVAL_PRECIOUS;
  }
  if (!ingredientc) return KITCHEN_APPROVAL_EMPTY;
  return KITCHEN_PRE_APPROVED;
}
