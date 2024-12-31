#include "game/sauces.h"
#include "game/menu/menu.h"
#include "layer.h"

struct layer_hello {
  struct layer hdr;
  struct menu *menu;
};

#define LAYER ((struct layer_hello*)layer)

static void _hello_del(struct layer *layer) {
  menu_del(LAYER->menu);
}

static void hello_cb_new_game(struct widget *widget) {
  struct layer *layer=widget->userdata;
  sauces_begin_session();
}

static void hello_cb_settings(struct widget *widget) {
  struct layer *layer=widget->userdata;
  layer_spawn(&layer_type_settings);
}

static void hello_cb_credits(struct widget *widget) {
  struct layer *layer=widget->userdata;
  layer_spawn(&layer_type_credits);
}

static void hello_cb_quit(struct widget *widget) {
  struct layer *layer=widget->userdata;
  egg_terminate(0);
}

static int _hello_init(struct layer *layer) {
  layer->opaque=1;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->userdata=layer;
  
  struct widget *widget=widget_decal_spawn_string(LAYER->menu,FBW>>1,50,0,1,2,FBW,0xffffffff,0,0);
  if (!widget) return -1;
  widget->accept_focus=0;
  
  widget_decal_spawn_string(LAYER->menu,FBW>>1,100,0,RID_strings_ui,1,FBW,0xffffffff,hello_cb_new_game,layer);
  widget_decal_spawn_string(LAYER->menu,FBW>>1,115,0,RID_strings_ui,2,FBW,0xffffffff,hello_cb_settings,layer);
  widget_decal_spawn_string(LAYER->menu,FBW>>1,130,0,RID_strings_ui,3,FBW,0xffffffff,hello_cb_credits,layer);
  widget_decal_spawn_string(LAYER->menu,FBW>>1,145,0,RID_strings_ui,4,FBW,0xffffffff,hello_cb_quit,layer);
  
  egg_play_song(0,0,1);
  
  return 0;
}

static void _hello_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _hello_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

static void _hello_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x402060ff);
  menu_render(LAYER->menu);
}

const struct layer_type layer_type_hello={
  .name="hello",
  .objlen=sizeof(struct layer_hello),
  .del=_hello_del,
  .init=_hello_init,
  .input=_hello_input,
  .update=_hello_update,
  .render=_hello_render,
};
