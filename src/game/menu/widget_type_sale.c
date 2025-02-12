#include "game/sauces.h"
#include "game/session/session.h"
#include "menu.h"

struct widget_sale {
  struct widget hdr;
  uint8_t tileid,xform;
  int price_texid;
  int price_w,price_h;
  int tx,ty,px,py; // Relative offsets to tile and price.
};

#define WIDGET ((struct widget_sale*)widget)

static void _sale_del(struct widget *widget) {
  egg_texture_del(WIDGET->price_texid);
}

static void _sale_render(struct widget *widget) {
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_item),widget->x+WIDGET->tx,widget->y+WIDGET->ty,WIDGET->tileid,WIDGET->xform);
  graf_draw_decal(&g.graf,WIDGET->price_texid,widget->x+WIDGET->px,widget->y+WIDGET->py,0,0,WIDGET->price_w,WIDGET->price_h,0);
}

const struct widget_type widget_type_sale={
  .name="sale",
  .objlen=sizeof(struct widget_sale),
  .del=_sale_del,
  .render=_sale_render,
};

/* Spawn.
 */
 
struct widget *widget_sale_spawn(struct menu *menu,int x,int y,const struct item *item,int price) {
  int itemid=item-itemv;
  if ((itemid<0)||(itemid>0xff)) return 0;
  struct widget *widget=widget_spawn(menu,&widget_type_sale);
  if (!widget) return 0;
  widget->x=x;
  widget->y=y;
  widget->w=NS_sys_tilesize<<1;
  widget->h=NS_sys_tilesize<<1;
  WIDGET->tileid=itemid;
  WIDGET->xform=0;
  
  if (price<-999) price=-999;
  else if (price>999) price=999;
  uint32_t pricecolor;
  char tmp[4];
  int tmpc=0;
  if (price<0) {
    pricecolor=0xffc0c0ff;
    tmp[tmpc++]='-';
    if (price<=-100) tmp[tmpc++]='0'-(price/100)%10;
    if (price<= -10) tmp[tmpc++]='0'-(price/ 10)%10;
    tmp[tmpc++]='0'-price%10;
  } else if (price>0) {
    pricecolor=0x80ff80ff;
    tmp[tmpc++]='+';
    if (price>=100) tmp[tmpc++]='0'+(price/100)%10;
    if (price>= 10) tmp[tmpc++]='0'+(price/ 10)%10;
    tmp[tmpc++]='0'+price%10;
  } else {
    pricecolor=0xa0a0a0ff;
    tmp[tmpc++]='-';
    tmp[tmpc++]='-';
    tmp[tmpc++]='-';
  }
  
  if (!(WIDGET->price_texid=font_tex_oneline(g.font,tmp,tmpc,NS_sys_tilesize<<1,pricecolor))) {
    menu_remove_widget(menu,widget);
    widget_del(widget);
    return 0;
  }
  egg_texture_get_status(&WIDGET->price_w,&WIDGET->price_h,WIDGET->price_texid);
  
  WIDGET->tx=widget->w>>1;
  WIDGET->px=(widget->w>>1)-(WIDGET->price_w>>1);
  const int gap=2; // vertical gap between tile and price.
  int totalh=NS_sys_tilesize+gap+WIDGET->price_h;
  int totaly=(widget->h>>1)-(totalh>>1);
  WIDGET->ty=totaly+(NS_sys_tilesize>>1);
  WIDGET->py=totaly+NS_sys_tilesize+gap;
  
  return widget;
}
