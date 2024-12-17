#include "game/sauces.h"
#include "game/menu/menu.h"
#include "layer.h"

struct layer_credits {
  struct layer hdr;
  struct menu *menu;
};

#define LAYER ((struct layer_credits*)layer)

static void _credits_del(struct layer *layer) {
  menu_del(LAYER->menu);
}

static void credits_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
  layer_pop(layer);
}

static void credits_cb_main_menu(struct widget *widget) {
  struct layer *layer=widget->userdata;
  layer_pop(layer);
}

static int _credits_init(struct layer *layer) {
  layer->opaque=1;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->userdata=layer;
  LAYER->menu->cb_cancel=credits_cb_cancel;
  
  struct widget *widget=widget_decal_spawn_text(LAYER->menu,FBW>>1,50,0,"TODO: Credits",-1,FBW,0xffffffff,0,0);
  if (!widget) return -1;
  widget->accept_focus=0;
  
  widget_decal_spawn_string(LAYER->menu,FBW>>1,100,0,RID_strings_ui,5,FBW,0xffffffff,credits_cb_main_menu,layer);
  
  return 0;
}

static void _credits_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _credits_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

static void _credits_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x104010ff);
  menu_render(LAYER->menu);
}

const struct layer_type layer_type_credits={
  .name="credits",
  .objlen=sizeof(struct layer_credits),
  .del=_credits_del,
  .init=_credits_init,
  .input=_credits_input,
  .update=_credits_update,
  .render=_credits_render,
};
