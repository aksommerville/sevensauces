/* layer_type_beginday.c
 * Spawned at the start of the diurnal phase.
 * We're responsible for initializing the global world, and we'll deliver some introductory content to the user.
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/world/world.h"
#include "layer.h"

struct layer_beginday {
  struct layer hdr;
  int texid,texw,texh;
};

#define LAYER ((struct layer_beginday*)layer)

static void _beginday_del(struct layer *layer) {
  egg_texture_del(LAYER->texid);
}

static int _beginday_init(struct layer *layer) {
  layer->opaque=1;
  if (!g.session||(g.session->day<0)||(g.session->day>6)) return -1;
  
  world_del(g.world);
  if (!(g.world=world_new())) return -1;
  
  LAYER->texid=font_texres_oneline(g.font,RID_strings_ui,6+g.session->day,FBW,0xffffffff);
  egg_texture_get_status(&LAYER->texw,&LAYER->texh,LAYER->texid);
  
  return 0;
}

static void beginday_proceed(struct layer *layer) {
  layer_pop(layer);
  layer_spawn(&layer_type_world);
}

static void _beginday_input(struct layer *layer,int input,int pvinput) {
  if (BTNDOWN(SOUTH)) { beginday_proceed(layer); return; }
  if (BTNDOWN(WEST)) { beginday_proceed(layer); return; }
}

static void _beginday_update(struct layer *layer,double elapsed) {
}

static void _beginday_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_draw_decal(&g.graf,LAYER->texid,
    (FBW>>1)-(LAYER->texw>>1),
    (FBH>>1)-(LAYER->texh>>1),
    0,0,LAYER->texw,LAYER->texh,0
  );
}

const struct layer_type layer_type_beginday={
  .name="beginday",
  .objlen=sizeof(struct layer_beginday),
  .del=_beginday_del,
  .init=_beginday_init,
  .input=_beginday_input,
  .update=_beginday_update,
  .render=_beginday_render,
};
