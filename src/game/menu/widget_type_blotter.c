/* widget_type_blotter.c
 * Flat colored rectangle. Probably only useful for troubleshooting.
 */
 
#include "game/sauces.h"
#include "menu.h"

struct widget_blotter {
  struct widget hdr;
  uint32_t rgba;
};

#define WIDGET ((struct widget_blotter*)widget)

static void _blotter_render(struct widget *widget) {
  graf_draw_rect(&g.graf,widget->x,widget->y,widget->w,widget->h,WIDGET->rgba);
}

const struct widget_type widget_type_blotter={
  .name="blotter",
  .objlen=sizeof(struct widget_blotter),
  .render=_blotter_render,
};

int widget_blotter_setup(struct widget *widget,uint32_t rgba) {
  if (!widget||(widget->type!=&widget_type_blotter)) return -1;
  WIDGET->rgba=rgba;
  return 0;
}

struct widget *widget_blotter_spawn(struct menu *menu,int x,int y,int w,int h,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_blotter);
  if (!widget) return 0;
  widget->x=x;
  widget->y=y;
  widget->w=w;
  widget->h=h;
  widget->cb=cb;
  widget->userdata=userdata;
  WIDGET->rgba=rgba;
  return widget;
}
