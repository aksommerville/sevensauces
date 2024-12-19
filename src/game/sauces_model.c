/* sauces_model.c
 * Interface to key model events, from a global scope.
 */

#include "sauces.h"
#include "game/session/session.h"
#include "game/world/world.h"
#include "game/kitchen/kitchen.h"
#include "game/layer/layer.h"

/* Layer filter: Only those we can keep when dropping the session.
 * Might only be hello.
 */
 
static int filter_layer_nonsession(struct layer *layer) {
  if (layer->type==&layer_type_hello) return 1;
  return 0;
}

/* Begin session.
 */
 
int sauces_begin_session() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) { world_del(g.world); g.world=0; }
  if (g.kitchen) { kitchen_del(g.kitchen); g.kitchen=0; }
  if (g.session) { session_del(g.session); g.session=0; }
  if (!(g.session=session_new())) return -1;
  //TODO model prep
  if (!layer_spawn(&layer_type_beginday)) return -1;
  return 0;
}

/* End session manually.
 */
 
void sauces_end_session() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) { world_del(g.world); g.world=0; }
  if (g.kitchen) { kitchen_del(g.kitchen); g.kitchen=0; }
  if (g.session) { session_del(g.session); g.session=0; }
}

/* End day.
 */
 
int sauces_end_day() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) {
    world_commit_to_session(g.world);
    world_del(g.world);
    g.world=0;
  }
  if (g.kitchen) kitchen_del(g.kitchen);
  if (!(g.kitchen=kitchen_new())) return -1;
  if (!layer_spawn(&layer_type_kitchen)) return -1;
  return 0;
}

/* End night.
 */
 
int sauces_end_night() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.kitchen) {
    kitchen_commit_to_session(g.kitchen);
    kitchen_del(g.kitchen);
    g.kitchen=0;
  }
  g.session->day++;
  if (g.session->day>=7) {
    fprintf(stderr,"*** end of week ***\n");//TODO wrap-up layer
  } else {
    if (!layer_spawn(&layer_type_beginday)) return -1;
  }
  return 0;
}
