/* layer_type_eod.c
 * Shows up immediately after night (kitchen), shows user a summary of the meal's results.
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/kitchen/kitchen.h"
#include "layer.h"

struct layer_eod {
  struct layer hdr;
  struct stew stew;
};

#define LAYER ((struct layer_eod*)layer)

/* Cleanup.
 */
 
static void _eod_del(struct layer *layer) {
}

/* Init.
 */
 
static int _eod_init(struct layer *layer) {
  if (!g.session||!g.kitchen) return -1;
  kitchen_cook(&LAYER->stew,g.kitchen);
  kitchen_commit_stew(&LAYER->stew);//TODO I think this is ok? Get it out of the way. If needed, we can wait until dismiss to commit it.
  return 0;
}

/* Input.
 */
 
static void _eod_input(struct layer *layer,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    sauces_end_night();
  }
}

/* Update.
 */
 
static void _eod_update(struct layer *layer,double elapsed) {
}

/* Render.
 */
 
static void _eod_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x002040ff);
}

/* Type definition.
 */
 
const struct layer_type layer_type_eod={
  .name="eod",
  .objlen=sizeof(struct layer_eod),
  .del=_eod_del,
  .init=_eod_init,
  .input=_eod_input,
  .update=_eod_update,
  .render=_eod_render,
};
