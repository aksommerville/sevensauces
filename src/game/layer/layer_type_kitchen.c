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

/* Menu callbacks.
 */
 
static void kitchen_cb_activate(struct menu *menu,struct widget *widget) {
  struct layer *layer=menu->userdata;
  sauces_end_night();
}
 
static void kitchen_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
  sauces_end_night();
}

/* Init.
 */
 
static int _kitchen_init(struct layer *layer) {
  if (!g.kitchen) return -1;
  layer->opaque=1;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->userdata=layer;
  LAYER->menu->cb_activate=kitchen_cb_activate;
  LAYER->menu->cb_cancel=kitchen_cb_cancel;
  
  struct widget *widget=widget_decal_spawn_text(LAYER->menu,FBW>>1,FBH>>1,0,"TODO: kitchen. Any key to proceed.",-1,FBW,0xffffffff,0,0);
  if (!widget) return -1;
  widget->accept_focus=0;
  //TODO populate menu
  
  egg_play_song(RID_song_goddess_of_night,0,1);
  
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
