/* layer_type_toast.c
 * Displays a static message passively for a fixed period of time.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "layer.h"

#define TOAST_TTL 1.000
#define TOAST_FADE_TIME 0.250
#define TOAST_RISE_RATE -15.0 /* px/s */

struct layer_toast {
  struct layer hdr;
  double ttl;
  int texid,texw,texh;
  double dstx,dsty;
  double dy; // m/s or px/s depending on (use_world_coords)
  int use_world_coords;
};

#define LAYER ((struct layer_toast*)layer)

static void _toast_del(struct layer *layer) {
  egg_texture_del(LAYER->texid);
}

static int _toast_init(struct layer *layer) {
  layer->opaque=0;
  LAYER->ttl=TOAST_TTL;
  return 0;
}

static void _toast_update(struct layer *layer,double elapsed) {
  if ((LAYER->ttl-=elapsed)<=0.0) {
    layer_pop(layer);
    return;
  }
  LAYER->dsty+=elapsed*LAYER->dy;
}

static void _toast_render(struct layer *layer) {
  int dstx,dsty;
  if (LAYER->use_world_coords&&g.world) {
    dstx=(int)(LAYER->dstx*NS_sys_tilesize)-g.world->viewx;
    dsty=(int)(LAYER->dsty*NS_sys_tilesize)-g.world->viewy;
  } else {
    dstx=(int)LAYER->dstx;
    dsty=(int)LAYER->dsty;
  }
  if (LAYER->ttl<TOAST_FADE_TIME) {
    int alpha=(int)((LAYER->ttl*256.0f)/TOAST_FADE_TIME);
    if (alpha<1) return;
    if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
  }
  graf_draw_decal(&g.graf,LAYER->texid,dstx,dsty,0,0,LAYER->texw,LAYER->texh,0);
  graf_set_alpha(&g.graf,0xff);
}

const struct layer_type layer_type_toast={
  .name="toast",
  .objlen=sizeof(struct layer_toast),
  .del=_toast_del,
  .init=_toast_init,
  .update=_toast_update,
  .render=_toast_render,
};

int layer_toast_setup_fb(struct layer *layer,int x,int y,const char *msg,int msgc,uint32_t rgba) {
  if (!layer||(layer->type!=&layer_type_toast)) return -1;
  if (LAYER->texid) return -1;
  LAYER->use_world_coords=0;
  LAYER->dstx=x;
  LAYER->dsty=y;
  if ((LAYER->texid=font_tex_oneline(g.font,msg,msgc,FBW,rgba))<1) return -1;
  egg_texture_get_status(&LAYER->texw,&LAYER->texh,LAYER->texid);
  LAYER->dstx-=LAYER->texw>>1;
  LAYER->dsty-=LAYER->texh;
  LAYER->dy=TOAST_RISE_RATE;
  return 0;
}

int layer_toast_setup_world(struct layer *layer,double x,double y,const char *msg,int msgc,uint32_t rgba) {
  if (!layer||(layer->type!=&layer_type_toast)) return -1;
  if (LAYER->texid) return -1;
  LAYER->use_world_coords=1;
  LAYER->dstx=x;
  LAYER->dsty=y;
  if ((LAYER->texid=font_tex_oneline(g.font,msg,msgc,FBW,rgba))<1) return -1;
  egg_texture_get_status(&LAYER->texw,&LAYER->texh,LAYER->texid);
  LAYER->dstx-=((double)LAYER->texw/(double)NS_sys_tilesize)*0.5;
  LAYER->dsty-=(double)LAYER->texh/(double)NS_sys_tilesize;
  LAYER->dy=TOAST_RISE_RATE/(double)NS_sys_tilesize;
  return 0;
}
