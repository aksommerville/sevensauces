/* widget_type_decal.c
 * Render a decal from any texture. For images larger than a tile, or text generated on demand.
 */
 
#include "game/sauces.h"
#include "menu.h"

#define DECAL_BLINK_PERIOD 20
#define DECAL_BLINK_DUTY_CYCLE 10
#define DECAL_BLINK_ALPHA 0x40

struct widget_decal {
  struct widget hdr;
  int texid; // STRONG, overrides (imageid) if nonzero.
  int imageid;
  int srcx,srcy,srcw,srch;
  uint8_t xform;
  uint16_t align;
  int blinkc;
  int blink_clock; // Widgets don't get an update hook, so it's based on render cycles.
};

#define WIDGET ((struct widget_decal*)widget)

static void _decal_del(struct widget *widget) {
  if (WIDGET->texid) egg_texture_del(WIDGET->texid);
}

static void _decal_render(struct widget *widget) {

  if (WIDGET->blinkc) {
    if (++(WIDGET->blink_clock)>=DECAL_BLINK_PERIOD) {
      if (WIDGET->blinkc>0) WIDGET->blinkc--;
      if (WIDGET->blinkc) WIDGET->blink_clock=0; // Leave the clock high if we ended the final blink.
    }
    if (WIDGET->blink_clock<DECAL_BLINK_DUTY_CYCLE) graf_set_alpha(&g.graf,DECAL_BLINK_ALPHA);
  }

  int texid=WIDGET->texid?WIDGET->texid:texcache_get_image(&g.texcache,widget->imageid);
  int dstx,dsty;
  switch (WIDGET->align&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: dstx=0; break;
    case EGG_BTN_RIGHT: dstx=widget->w-WIDGET->srcw; break;
    default: dstx=(widget->w>>1)-(WIDGET->srcw>>1); break;
  }
  switch (WIDGET->align&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: dsty=0; break;
    case EGG_BTN_DOWN: dsty=widget->h-WIDGET->srch; break;
    default: dsty=(widget->h>>1)-(WIDGET->srch>>1); break;
  }
  dstx+=widget->x;
  dsty+=widget->y;
  graf_draw_decal(&g.graf,texid,
    dstx,dsty,
    WIDGET->srcx,WIDGET->srcy,
    WIDGET->srcw,WIDGET->srch,
    WIDGET->xform
  );
  
  if (WIDGET->blinkc&&(WIDGET->blink_clock<DECAL_BLINK_DUTY_CYCLE)) graf_set_alpha(&g.graf,0xff);
}

const struct widget_type widget_type_decal={
  .name="decal",
  .objlen=sizeof(struct widget_decal),
  .del=_decal_del,
  .render=_decal_render,
};

int widget_decal_setup_image(struct widget *widget,int imageid,int x,int y,int w,int h) {
  if (!widget||(widget->type!=&widget_type_decal)) return -1;
  if (WIDGET->texid) {
    egg_texture_del(WIDGET->texid);
    WIDGET->texid=0;
  }
  if (!x&&!y&&!w&&!h) {
    int texid=texcache_get_image(&g.texcache,imageid);
    egg_texture_get_status(&w,&h,texid);
  }
  widget->imageid=imageid;
  WIDGET->srcx=x;
  WIDGET->srcy=y;
  WIDGET->srcw=w;
  WIDGET->srch=h;
  WIDGET->xform=0;
  if (!widget->w) {
    widget->w=w;
    widget->h=h;
  }
  WIDGET->blinkc=0;
  return 0;
}

int widget_decal_setup_text(struct widget *widget,const char *src,int srcc,int wlimit,uint32_t rgba) {
  if (!widget||(widget->type!=&widget_type_decal)) return -1;
  if (WIDGET->texid) egg_texture_del(WIDGET->texid);
  if (wlimit) {
    WIDGET->texid=font_tex_multiline(g.font,src,srcc,wlimit,FBH,rgba);
  } else {
    WIDGET->texid=font_tex_oneline(g.font,src,srcc,FBW,rgba);
  }
  if (WIDGET->texid<1) {
    WIDGET->texid=0;
    return -1;
  }
  WIDGET->srcx=0;
  WIDGET->srcy=0;
  egg_texture_get_status(&WIDGET->srcw,&WIDGET->srch,WIDGET->texid);
  WIDGET->xform=0;
  // Very debatable: I'm automatically adding margins, I think we'll want that every time.
  if (!widget->w) {
    widget->w=WIDGET->srcw+7;
    widget->h=WIDGET->srch+3;
  }
  WIDGET->blinkc=0;
  return 0;
}

int widget_decal_setup_string(struct widget *widget,int rid,int ix,int wlimit,uint32_t rgba) {
  const char *src=0;
  int srcc=strings_get(&src,rid,ix);
  if (srcc<0) srcc=0;
  return widget_decal_setup_text(widget,src,srcc,wlimit,rgba);
}

static void widget_decal_set_bounds(struct widget *widget,int x,int y,uint16_t align) {
  switch (align&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: widget->x=x; break;
    case EGG_BTN_RIGHT: widget->x=x-widget->w; break;
    default: widget->x=x-(widget->w>>1);
  }
  switch (align&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: widget->y=y; break;
    case EGG_BTN_DOWN: widget->y=y-widget->h; break;
    default: widget->y=y-(widget->h>>1);
  }
}

struct widget *widget_decal_spawn_image(struct menu *menu,int x,int y,uint16_t align,int imageid,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_decal);
  if (widget_decal_setup_image(widget,imageid,0,0,0,0)<0) {
    menu_remove_widget(menu,widget);
    widget_del(widget);
    return 0;
  }
  widget->cb=cb;
  widget->userdata=userdata;
  widget_decal_set_bounds(widget,x,y,align);
  return widget;
}

struct widget *widget_decal_spawn_partial(struct menu *menu,int x,int y,uint16_t align,int imageid,int srcx,int srcy,int srcw,int srch,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_decal);
  if (widget_decal_setup_image(widget,imageid,srcx,srcy,srcw,srch)<0) {
    menu_remove_widget(menu,widget);
    widget_del(widget);
    return 0;
  }
  widget->cb=cb;
  widget->userdata=userdata;
  widget_decal_set_bounds(widget,x,y,align);
  return widget;
}

struct widget *widget_decal_spawn_text(struct menu *menu,int x,int y,uint16_t align,const char *src,int srcc,int wlimit,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_decal);
  if (widget_decal_setup_text(widget,src,srcc,wlimit,rgba)<0) {
    menu_remove_widget(menu,widget);
    widget_del(widget);
    return 0;
  }
  widget->cb=cb;
  widget->userdata=userdata;
  widget_decal_set_bounds(widget,x,y,align);
  return widget;
}

struct widget *widget_decal_spawn_string(struct menu *menu,int x,int y,uint16_t align,int rid,int ix,int wlimit,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata) {
  struct widget *widget=widget_spawn(menu,&widget_type_decal);
  if (widget_decal_setup_string(widget,rid,ix,wlimit,rgba)<0) {
    menu_remove_widget(menu,widget);
    widget_del(widget);
    return 0;
  }
  widget->cb=cb;
  widget->userdata=userdata;
  widget_decal_set_bounds(widget,x,y,align);
  return widget;
}

void widget_decal_align(struct widget *widget,uint16_t align) {
  if (!widget||(widget->type!=&widget_type_decal)) return;
  WIDGET->align=align;
}

void widget_decal_blink(struct widget *widget,int repc) {
  if (!widget||(widget->type!=&widget_type_decal)) return;
  WIDGET->blinkc=repc;
  WIDGET->blink_clock=0;
}
