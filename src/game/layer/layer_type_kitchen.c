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
  
  fprintf(stderr,"%s customers in g.kitchen:\n",__func__);
  struct kcustomer *kcustomer=g.kitchen->kcustomerv;
  int i=g.kitchen->size;
  for (;i-->0;kcustomer++) {
    const char *name="---";
    switch (kcustomer->race) {
      case NS_race_man: name="man"; break;
      case NS_race_rabbit: name="rabbit"; break;
      case NS_race_octopus: name="octopus"; break;
      case NS_race_werewolf: name="werewolf"; break;
    }
    fprintf(stderr,"  %s %d\n",name,kcustomer->sesp);
  }
  
  egg_play_song(RID_song_watcher_of_pots,0,1);
  
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
 
/* XXX TEMP Layout Notes.
Framebuffer 320x180.
Large customers: 40x40 each, in 3 rows: 8,7,8. Up to 23. Height = 80
Small customers: 8x8 each, in 8 rows of 40. Up to 320 (limit 319). Height = 64
Pantry starts near (164,20). xstride TILESIZE+2, ystride TILESIZE+5.
Details panel: 237,4,79,91.
*/
 
static void _kitchen_render(struct layer *layer) {

  /* Background base, one image filling the framebuffer.
   */
  int texid=texcache_get_image(&g.texcache,RID_image_kitchen_bg);
  graf_draw_decal(&g.graf,texid,0,0,0,0,FBW,FBH,0);
  
  /* Little Sister sits near the left edge of the cauldron.
   */
  //TODO
  
  /* Bubbles and bobbing items floating in the stew.
   */
  //TODO
  
  /* Highlighted item details, right side.
   */
  //TODO
  
  /* Menu handles the pantry items, and "Cook" and "Advice" buttons.
   */
  menu_render(LAYER->menu);
  
  /* Word bubble, if Little Sister is talking.
   */
  //TODO
  
  /* Customers.
   */
  //TODO
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
