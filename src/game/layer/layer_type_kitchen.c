/* layer_type_kitchen.c
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/kitchen/kitchen.h"
#include "game/menu/menu.h"
#include "layer.h"

struct layer_kitchen {
  struct layer hdr;
  struct menu *menu;
};

#define LAYER ((struct layer_kitchen*)layer)

/* Cleanup.
 */
 
static void _kitchen_del(struct layer *layer) {
  menu_del(LAYER->menu);
}

/* Init.
 */
 
static int _kitchen_init(struct layer *layer) {
  if (!g.kitchen) return -1;
  layer->opaque=1;
  if (!(LAYER->menu=menu_new())) return -1;
  //TODO populate menu
  return 0;
}

/* Input.
 */
 
static void _kitchen_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _kitchen_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

/* Render.
 */
 
static void _kitchen_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x002040ff);
  menu_render(LAYER->menu);
}

/* Type definition.
 */
 
const struct layer_type layer_type_kitchen={
  .name="kitchen",
  .objlen=sizeof(struct layer_kitchen),
  .del=_kitchen_del,
  .init=_kitchen_init,
  .input=_kitchen_input,
  .update=_kitchen_update,
  .render=_kitchen_render,
};
