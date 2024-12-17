/* widget_type_tile.c
 * Just an icon, from some tilesheet.
 */
 
#include "game/sauces.h"
#include "menu.h"

struct widget_tile {
  struct widget hdr;
  uint8_t tileid,xform;
};

#define WIDGET ((struct widget_tile*)widget)

static void _tile_render(struct widget *widget) {
  int texid=texcache_get_image(&g.texcache,widget->imageid);
  graf_draw_tile(&g.graf,texid,widget->x+(widget->w>>1),widget->y+(widget->h>>1),WIDGET->tileid,WIDGET->xform);
}

const struct widget_type widget_type_tile={
  .name="tile",
  .objlen=sizeof(struct widget_tile),
  .render=_tile_render,
};

int widget_tile_setup(struct widget *widget,int imageid,uint8_t tileid,uint8_t xform) {
  if (!widget||(widget->type!=&widget_type_tile)) return -1;
  widget->imageid=imageid;
  WIDGET->tileid=tileid;
  WIDGET->xform=xform;
  return 0;
}

struct widget *widget_tile_spawn(struct menu *menu,int x,int y,int imageid,uint8_t tileid,uint8_t xform,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_tile);
  if (!widget) return 0;
  widget->x=x-(NS_sys_tilesize>>1);
  widget->y=y-(NS_sys_tilesize>>1);
  widget->w=NS_sys_tilesize;
  widget->h=NS_sys_tilesize;
  widget->cb=cb;
  widget->userdata=userdata;
  widget->imageid=imageid;
  WIDGET->tileid=tileid;
  WIDGET->xform=xform;
  return widget;
}
