#include "game/sauces.h"
#include "layer.h"

struct layer_condensery {
  struct layer hdr;
};

#define LAYER ((struct layer_condensery*)layer)

/* Cleanup.
 */
 
static void _condensery_del(struct layer *layer) {
}

/* Init.
 */
 
static int _condensery_init(struct layer *layer) {
  fprintf(stderr,"***** %s *****\n",__func__);//TODO
  layer->opaque=1;
  return 0;
}

/* Input.
 */
 
static void _condensery_input(struct layer *layer,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    layer_pop(layer);
  }
}

/* Update.
 */
 
static void _condensery_update(struct layer *layer,double elapsed) {
}

/* Render.
 */
 
static void _condensery_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x400000ff);
}

/* Type definition.
 */
 
const struct layer_type layer_type_condensery={
  .name="condensery",
  .objlen=sizeof(struct layer_condensery),
  .del=_condensery_del,
  .init=_condensery_init,
  .input=_condensery_input,
  .update=_condensery_update,
  .render=_condensery_render,
};
