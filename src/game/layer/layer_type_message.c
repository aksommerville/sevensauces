/* layer_type_message.c
 * Generic modal message.
 */
 
#include "game/sauces.h"
#include "layer.h"

struct layer_message {
  struct layer hdr;
  int texid,texw,texh;
};

#define LAYER ((struct layer_message*)layer)

static void _message_del(struct layer *layer) {
  egg_texture_del(LAYER->texid);
}

static int _message_init(struct layer *layer) {
  layer->opaque=0;
  return 0;
}

static void _message_input(struct layer *layer,int input,int pvinput) {
  if (BTNDOWN(SOUTH)||BTNDOWN(WEST)) layer_pop(layer);
}

static void _message_render(struct layer *layer) {
  int16_t dstx=(FBW>>1)-(LAYER->texw>>1);
  int16_t dsty=(FBH>>1)-(LAYER->texh>>1);
  const int16_t margin=5;
  graf_draw_rect(&g.graf,dstx-margin,dsty-margin,LAYER->texw+(margin<<1),LAYER->texh+(margin<<1),0x000000c0);
  graf_draw_decal(&g.graf,LAYER->texid,dstx,dsty,0,0,LAYER->texw,LAYER->texh,0);
}

const struct layer_type layer_type_message={
  .name="message",
  .objlen=sizeof(struct layer_message),
  .del=_message_del,
  .init=_message_init,
  .input=_message_input,
  .render=_message_render,
};

int layer_message_setup_raw(struct layer *layer,const char *src,int srcc) {
  if (!layer||(layer->type!=&layer_type_message)) return -1;
  if (LAYER->texid) return -1;
  LAYER->texid=font_tex_multiline(g.font,src,srcc,FBW>>1,FBH,0xffffffff);
  egg_texture_get_status(&LAYER->texw,&LAYER->texh,LAYER->texid);
  return 0;
}

int layer_message_setup_string(struct layer *layer,int rid,int ix) {
  if (!layer||(layer->type!=&layer_type_message)) return -1;
  if (LAYER->texid) return -1;
  LAYER->texid=font_texres_multiline(g.font,rid,ix,FBW>>1,FBH,0xffffffff);
  egg_texture_get_status(&LAYER->texw,&LAYER->texh,LAYER->texid);
  return 0;
}
