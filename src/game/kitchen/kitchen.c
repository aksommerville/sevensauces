#include "game/sauces.h"
#include "game/session/session.h"
#include "kitchen.h"

/* Delete.
 */
 
void kitchen_del(struct kitchen *kitchen) {
  if (!kitchen) return;
  free(kitchen);
}

/* New.
 */
 
struct kitchen *kitchen_new() {
  struct kitchen *kitchen=calloc(1,sizeof(struct kitchen));
  if (!kitchen) return 0;
  return kitchen;
}

/* Commit to session, last chance to persist anything.
 */
 
void kitchen_commit_to_session(struct kitchen *kitchen) {
}
