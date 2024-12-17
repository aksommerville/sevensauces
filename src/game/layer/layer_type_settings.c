#include "game/sauces.h"
#include "game/menu/menu.h"
#include "layer.h"

struct layer_settings {
  struct layer hdr;
  struct menu *menu;
};

#define LAYER ((struct layer_settings*)layer)

static void _settings_del(struct layer *layer) {
  menu_del(LAYER->menu);
}

static void settings_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
  layer_pop(layer);
}

static void settings_cb_main_menu(struct widget *widget) {
  struct layer *layer=widget->userdata;
  layer_pop(layer);
}

static int _settings_init(struct layer *layer) {
  layer->opaque=1;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->userdata=layer;
  LAYER->menu->cb_cancel=settings_cb_cancel;
  
  struct widget *widget=widget_decal_spawn_text(LAYER->menu,FBW>>1,50,0,"TODO: Settings",-1,FBW,0xffffffff,0,0);
  if (!widget) return -1;
  widget->accept_focus=0;
  
  widget_decal_spawn_string(LAYER->menu,FBW>>1,100,0,RID_strings_ui,5,FBW,0xffffffff,settings_cb_main_menu,layer);
  
  return 0;
}

static void _settings_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _settings_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

static void _settings_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x401010ff);
  menu_render(LAYER->menu);
}

const struct layer_type layer_type_settings={
  .name="settings",
  .objlen=sizeof(struct layer_settings),
  .del=_settings_del,
  .init=_settings_init,
  .input=_settings_input,
  .update=_settings_update,
  .render=_settings_render,
};
