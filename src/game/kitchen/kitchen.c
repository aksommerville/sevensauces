#include "game/sauces.h"
#include "game/session/session.h"
#include "kitchen.h"

/* Delete.
 */
 
void kitchen_del(struct kitchen *kitchen) {
  if (!kitchen) return;
  if (kitchen->kcustomerv) free(kitchen->kcustomerv);
  free(kitchen);
}

/* Initialize (kcustomerv). Caller must allocate it first.
 */
 
static void kitchen_init_customer_1(struct kcustomer *dst,const struct customer *src) {
  dst->race=src->race;
  if ((dst->sesp=src-g.session->customerv)>=g.session->customerc) dst->sesp=0;
}

// Start vacant and place customers pure-random.
static void kitchen_init_customers_light(struct kitchen *kitchen) {
  struct customer *customer=g.session->customerv;
  int i=g.session->customerc;
  for (;i-->0;customer++) {
    int p;
    struct kcustomer *kcustomer;
    do {
      p=rand()%kitchen->size;
      kcustomer=kitchen->kcustomerv+p;
    } while (kcustomer->race);
    kitchen_init_customer_1(kcustomer,customer);
  }
}

// Place vacancies pure-random, then fill in customers iteratively.
static void kitchen_init_customers_heavy(struct kitchen *kitchen) {
  // They all start vacant due to (race==0). We'll mark vacancies with (sesp<0).
  int i=kitchen->size-g.session->customerc;
  while (i-->0) {
    int p;
    struct kcustomer *kcustomer;
    do {
      p=rand()%kitchen->size;
      kcustomer=kitchen->kcustomerv+p;
    } while (kcustomer->sesp);
    kcustomer->sesp=-1;
  }
  struct customer *customer=g.session->customerv;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  for (i=kitchen->size;i-->0;kcustomer++) {
    if (kcustomer->sesp) continue;
    kitchen_init_customer_1(kcustomer,customer);
    customer++;
  }
}

// Split the dining room into buckets and put one customer in each.
static void kitchen_init_customers_middling(struct kitchen *kitchen) {
  int bucketsize=kitchen->size/g.session->customerc;
  int bucketslop=kitchen->size%g.session->customerc;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  struct customer *customer=g.session->customerv;
  int i=g.session->customerc;
  while (i-->0) {
    int seatc=bucketsize;
    if (bucketslop>0) { // Larger buckets all appear first. TODO Will that be noticeable?
      seatc++;
      bucketslop--;
    }
    int choice=rand()%seatc;
    kitchen_init_customer_1(kcustomer+choice,customer);
    kcustomer+=seatc;
    customer++;
  }
}

/* Init, fallible parts.
 */
 
static int kitchen_init(struct kitchen *kitchen) {

  /* Determine size of the dining room and allocate it.
   * You're not allowed to create a kitchen when there aren't any customers; must go straight to game-over in that case.
   * Defaulting kcustomer to zero makes the slot vacant, due to (race).
   */
  if (kitchen->size) return -1;
  if (g.session->customerc<1) return -1;
  if (g.session->customerc<=DINING_ROOM_SMALL) kitchen->size=DINING_ROOM_SMALL;
  else if (g.session->customerc<=DINING_ROOM_LARGE) kitchen->size=DINING_ROOM_LARGE;
  else return -1;
  if (!(kitchen->kcustomerv=calloc(kitchen->size,sizeof(struct kcustomer)))) return -1;
  
  /* If the customer count is under 1/4 or over 3/4 of capacity, fill in the smaller quarter randomly,
   * then iterate across the rest.
   * When it's in the middle, we'll use a more involved spreading algorithm.
   */
  int quarter=kitchen->size>>2;
  if (g.session->customerc<=quarter) kitchen_init_customers_light(kitchen);
  else if (g.session->customerc>=kitchen->size-quarter) kitchen_init_customers_heavy(kitchen);
  else kitchen_init_customers_middling(kitchen);
  
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

/* Commit to session, last chance to persist anything.
 */
 
void kitchen_commit_to_session(struct kitchen *kitchen) {
  //TODO
}
