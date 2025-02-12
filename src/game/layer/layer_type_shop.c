#include "game/sauces.h"
#include "layer.h"
#include "opt/rom/rom.h"
#include "game/menu/menu.h"
#include "game/session/session.h"

#define SHOP_ID_IMAGE 1
#define SHOP_ID_GOLD 2
#define SHOP_ID_DIALOGUE 3
#define SHOP_ID_SLOP 10 /* Everything >=this is transient and gets dropped on page changes. */
#define SHOP_ID_BUY 10 /* Root menu... */
#define SHOP_ID_SELL 11
#define SHOP_ID_EXIT 12
#define SHOP_ID_BUY_ITEM 100 /* "Buy" menu, +invp in shop */
#define SHOP_ID_SELL_ITEM 200 /* "Sell" menu, +invp in session */

struct layer_shop {
  struct layer hdr;
  struct menu *menu;
  int rid; // For logging etc.
  int dialogue_rid; // String we display at the root menu.
  int dialogue_ix;
  int state; // 0|SHOP_ID_BUY|SHOP_ID_SELL
  struct widget *focustrack; // WEAK
  int gold_displayed; // g.session->gold is allowed to change behind our back (not that it does); we'll detect it and update the view.
  
  // The shop's inventory is limited to the size of the player's inventory, but doesn't really need to be.
  const struct item *itemv[INVENTORY_SIZE];
  uint8_t pricev[INVENTORY_SIZE];
  int itemc; // 0..INVENTORY_SIZE
  uint8_t salepricev[256]; // Indexed by itemid. Nonzero if the shop buys it. Shops can buy things they don't sell.
};

#define LAYER ((struct layer_shop*)layer)

/* Cleanup.
 */
 
static void _shop_del(struct layer *layer) {
  menu_del(LAYER->menu);
}

/* Conveniences for inventory search.
 */

static int shop_sell_price_for_item(const struct layer *layer,const struct item *item) {
  int itemid=item-itemv;
  if ((itemid<0)||(itemid>0xff)) return 0;
  return LAYER->salepricev[itemid];
}

/* Zap volatile widgets and rebuild.
 */
 
static void shop_drop_slop(struct layer *layer) {
  int i=LAYER->menu->widgetc;
  while (i-->0) {
    struct widget *widget=LAYER->menu->widgetv[i];
    if (widget->id<SHOP_ID_SLOP) continue;
    menu_delete_widget_at(LAYER->menu,i);
  }
}
 
static void shop_build_root(struct layer *layer) {
  shop_drop_slop(layer);
  LAYER->state=0;
  struct widget *widget;
  const int yspacing=2;
  int x=8,y=18;
  
  if (!(widget=widget_decal_spawn_string(LAYER->menu,x,y,EGG_BTN_LEFT,RID_strings_ui,53,100,0xffffffff,0,0))) return;
  widget->id=SHOP_ID_BUY;
  y+=widget->h+yspacing;
  
  if (!(widget=widget_decal_spawn_string(LAYER->menu,x,y,EGG_BTN_LEFT,RID_strings_ui,54,100,0xffffffff,0,0))) return;
  widget->id=SHOP_ID_SELL;
  y+=widget->h+yspacing;
  
  if (!(widget=widget_decal_spawn_string(LAYER->menu,x,y,EGG_BTN_LEFT,RID_strings_ui,55,100,0xffffffff,0,0))) return;
  widget->id=SHOP_ID_EXIT;
  y+=widget->h+yspacing;
  
  if (!(widget=menu_widget_by_id(LAYER->menu,SHOP_ID_DIALOGUE))) return;
  widget_decal_setup_string(widget,LAYER->dialogue_rid,LAYER->dialogue_ix,widget->w,0xffffffff);
}

static void shop_build_buy(struct layer *layer) {
  shop_drop_slop(layer);
  LAYER->state=SHOP_ID_BUY;
  const int x0=17,y0=16;
  const int xstride=40,ystride=40;
  const int colc=4;
  int i=0,col=0,row=0;
  for (;i<LAYER->itemc;i++) {
    int x=x0+xstride*col;
    int y=y0+ystride*row;
    struct widget *widget=widget_sale_spawn(LAYER->menu,x,y,LAYER->itemv[i],-LAYER->pricev[i]);
    if (!widget) return;
    widget->id=SHOP_ID_BUY_ITEM+i;
    if (++col>=colc) {
      col=0;
      row++;
    }
  }
  
  LAYER->focustrack=0;
  struct widget *widget;
  if (!(widget=menu_widget_by_id(LAYER->menu,SHOP_ID_DIALOGUE))) return;
  widget_decal_setup_text(widget,"",0,widget->w,0xffffffff);
}

static void shop_build_sell(struct layer *layer) {
  shop_drop_slop(layer);
  LAYER->state=SHOP_ID_SELL;
  const int x0=17,y0=16;
  const int xstride=40,ystride=40;
  const int colc=4;
  int i=0,col=0,row=0;
  for (;i<INVENTORY_SIZE;i++) {
    if (g.session->inventory[i]) {
      const struct item *item=itemv+g.session->inventory[i];
      int price=shop_sell_price_for_item(layer,item);
      int x=x0+xstride*col;
      int y=y0+ystride*row;
      struct widget *widget=widget_sale_spawn(LAYER->menu,x,y,item,price);
      if (!widget) return;
      widget->id=SHOP_ID_SELL_ITEM+i;
    }
    // Advance the position even if the slot is empty, so we mimic the pause and kitchen layouts.
    if (++col>=colc) {
      col=0;
      row++;
    }
  }
  
  LAYER->focustrack=0;
  struct widget *widget;
  if (!(widget=menu_widget_by_id(LAYER->menu,SHOP_ID_DIALOGUE))) return;
  widget_decal_setup_text(widget,"",0,widget->w,0xffffffff);
}

/* Buy and sell items.
 */
 
static void shop_buy(struct layer *layer,int wid) {
  int invp=wid-SHOP_ID_BUY_ITEM;
  if ((invp<0)||(invp>=LAYER->itemc)) return;
  const struct item *item=LAYER->itemv[invp];
  int itemid=item-itemv;
  if ((itemid<0)||(itemid>0xff)) return;
  int price=LAYER->pricev[invp];
  if (price>g.session->gold) {
    fprintf(stderr,"*** can't buy item due to gold %d>%d ***\n",price,g.session->gold);//TODO feedback to user
    return;
  }
  if (session_acquire_item(g.session,itemid,0,0)>0) {
    g.session->gold-=price;
    egg_play_sound(RID_sound_buy);
  } else {
    fprintf(stderr,"*** purchase rejected, probably due to full inventory ***\n");//TODO feedback to user
  }
}

static void shop_sell(struct layer *layer,int wid) {
  int invp=wid-SHOP_ID_SELL_ITEM;
  if ((invp<0)||(invp>=INVENTORY_SIZE)) return;
  uint8_t itemid=g.session->inventory[invp];
  if (!itemid) return;
  const struct item *item=itemv+itemid;
  int price=shop_sell_price_for_item(layer,item);
  if (price<1) {
    fprintf(stderr,"*** sale rejected, merchant doesn't want this item ***\n");//TODO feedback to user
    return;
  }
  if ((g.session->gold+=price)>999) g.session->gold=999;
  egg_play_sound(RID_sound_sell);
  g.session->inventory[invp]=0;
  menu_delete_widget_id(LAYER->menu,wid);
}

/* Menu callbacks.
 */
 
static void shop_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
  if (LAYER->state) {
    int refocus=LAYER->state; // The nonzero state constants are widget IDs, the widget IDs we happen to be interested in.
    shop_build_root(layer);
    menu_focus_id(menu,refocus);
  } else {
    layer_pop(layer);
  }
}

static void shop_cb_activate(struct menu *menu,struct widget *widget) {
  struct layer *layer=menu->userdata;
  int wid=widget?widget->id:0;
  switch (wid) {
    case SHOP_ID_BUY: shop_build_buy(layer); break;
    case SHOP_ID_SELL: shop_build_sell(layer); break;
    case SHOP_ID_EXIT: layer_pop(layer); break;
    default: {
        if ((wid>=SHOP_ID_BUY_ITEM)&&(wid<SHOP_ID_BUY_ITEM+INVENTORY_SIZE)) {
          shop_buy(layer,wid);
        } else if ((wid>=SHOP_ID_SELL_ITEM)&&(wid<SHOP_ID_SELL_ITEM+INVENTORY_SIZE)) {
          shop_sell(layer,wid);
        }
      }
  }
}

/* Init.
 */
 
static int _shop_init(struct layer *layer) {
  if (!g.session) return -1;
  layer->opaque=1;
  
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->cb_cancel=shop_cb_cancel;
  LAYER->menu->cb_activate=shop_cb_activate;
  LAYER->menu->userdata=layer;
  struct widget *widget;
  
  if (!(widget=widget_spawn(LAYER->menu,&widget_type_decal))) return -1;
  widget->id=SHOP_ID_IMAGE;
  widget->accept_focus=0;
  widget->x=NS_sys_tilesize*13;
  widget->y=2;
  widget->w=NS_sys_tilesize*6;
  widget->h=NS_sys_tilesize*4;
  
  if (!(widget=widget_spawn(LAYER->menu,&widget_type_decal))) return -1;
  widget_decal_align(widget,EGG_BTN_RIGHT);
  widget->id=SHOP_ID_GOLD;
  widget->accept_focus=0;
  widget->x=NS_sys_tilesize*16-10;
  widget->y=2+NS_sys_tilesize*4;
  widget->w=NS_sys_tilesize*3;
  widget->h=NS_sys_tilesize;
  LAYER->gold_displayed=-1;
  
  if (!(widget=widget_spawn(LAYER->menu,&widget_type_decal))) return -1;
  widget_decal_align(widget,EGG_BTN_LEFT|EGG_BTN_UP);
  widget->id=SHOP_ID_DIALOGUE;
  widget->accept_focus=0;
  widget->x=NS_sys_tilesize*12+8;
  widget->y=2+NS_sys_tilesize*5+8;
  widget->w=NS_sys_tilesize*8-16;
  widget->h=NS_sys_tilesize*6-16;
  
  shop_build_root(layer);
  
  return 0;
}

/* Replace the dialogue section's text per focussed widget.
 */
 
static void shop_rebuild_dialogue_for_focus(struct layer *layer,struct widget *focus) {
  const struct item *item=0;
  int invp;
  if (((invp=focus->id-SHOP_ID_BUY_ITEM)>=0)&&(invp<LAYER->itemc)) {
    item=LAYER->itemv[invp];
  } else if (((invp=focus->id-SHOP_ID_SELL_ITEM)>=0)&&(invp<INVENTORY_SIZE)) {
    uint8_t itemid=g.session->inventory[invp];
    if (itemid) item=itemv+itemid;
  }
  struct widget *dialogue=menu_widget_by_id(LAYER->menu,SHOP_ID_DIALOGUE);
  if (!dialogue) return;
  if (item) {
    char tmp[1024];
    int tmpc=sauces_generate_item_description(tmp,sizeof(tmp),item-itemv,0);
    if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
    widget_decal_setup_text(dialogue,tmp,tmpc,dialogue->w,0xffffffff);
  } else {
    widget_decal_setup_text(dialogue,0,0,dialogue->w,0xffffffff);
  }
}

/* Update the gold widget.
 */
 
static void shop_rewrite_gold(struct layer *layer) {
  if (LAYER->gold_displayed==g.session->gold) return;
  struct widget *widget=menu_widget_by_id(LAYER->menu,SHOP_ID_GOLD);
  if (!widget) return;
  
  char tmp[4];
  int tmpc=0;
  tmp[tmpc++]='$';
  if (g.session->gold>=100) tmp[tmpc++]='0'+(g.session->gold/100)%10;
  if (g.session->gold>= 10) tmp[tmpc++]='0'+(g.session->gold/ 10)%10;
  tmp[tmpc++]='0'+g.session->gold%10;
  widget_decal_setup_text(widget,tmp,tmpc,widget->w,0xffff00ff);
  
  // If we're changing from something valid, animate it. This won't happen the first time.
  if (LAYER->gold_displayed>=0) {
    widget_decal_blink(widget,3);
  }
  LAYER->gold_displayed=g.session->gold;
}

/* Hooks mostly managed by menu.
 */
 
static void _shop_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}
 
static void _shop_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
  
  if (LAYER->menu->focus.widget!=LAYER->focustrack) {
    LAYER->focustrack=LAYER->menu->focus.widget;
    if (LAYER->state!=0) { // State zero (root), we show a constant message regardless of focus.
      shop_rebuild_dialogue_for_focus(layer,LAYER->focustrack);
    }
  }
  
  if (LAYER->gold_displayed!=g.session->gold) {
    shop_rewrite_gold(layer);
  }
}

/* Render.
 */

#define COLC 20
#define ROWC 11
#define YOFFSET 2
static const uint8_t map[COLC*ROWC]={
  0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x25,0x00,0x01,0x01,0x01,0x01,0x02,0x25,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x25,0x10,0x11,0x11,0x11,0x11,0x12,0x25,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x25,0x10,0x11,0x11,0x11,0x11,0x12,0x25,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x25,0x20,0x21,0x21,0x21,0x21,0x22,0x25,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x25,0x25,0x25,0x25,0x26,0x27,0x28,0x25,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x12,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x12,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x12,
  0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x12,
  0x20,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x22,0x20,0x21,0x21,0x21,0x21,0x21,0x21,0x22,
};
 
static void shop_render_boxes(struct layer *layer) {
  /* The box frames are a grid of tiles.
   * I think it will be more efficient to do them in one giant pass.
   * But this means widgets must line up to the grid.
   */
  graf_draw_tile_buffer(&g.graf,texcache_get_image(&g.texcache,RID_image_shop),
    (NS_sys_tilesize>>1),YOFFSET+(NS_sys_tilesize>>1),
    map,COLC,ROWC,COLC
  );
  graf_draw_rect(&g.graf,0,0,FBW,YOFFSET,0x000000ff);
  graf_draw_rect(&g.graf,0,YOFFSET+NS_sys_tilesize*ROWC,FBW,FBH,0x000000ff);
}
#undef COLC
#undef ROWC
#undef YOFFSET
 
static void _shop_render(struct layer *layer) {

  /* Background color shows inside the little boxes.
   * The area outside the boxes is black, and that's built in to their borders.
   */
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x102040ff);
  
  menu_render_widgets(LAYER->menu);
  shop_render_boxes(layer);
  menu_render_cursor(LAYER->menu);
}

/* Type definition.
 */
 
const struct layer_type layer_type_shop={
  .name="shop",
  .objlen=sizeof(struct layer_shop),
  .del=_shop_del,
  .init=_shop_init,
  .input=_shop_input,
  .update=_shop_update,
  .render=_shop_render,
};

/* Setup bits.
 */
 
static void shop_add_item(struct layer *layer,uint8_t itemid,uint8_t price) {
  if (LAYER->itemc>=INVENTORY_SIZE) {
    fprintf(stderr,"shop:%d too many items\n",LAYER->rid);
    return;
  }
  const struct item *item=itemv+itemid;
  LAYER->itemv[LAYER->itemc]=item;
  LAYER->pricev[LAYER->itemc]=price;
  LAYER->itemc++;
  LAYER->salepricev[itemid]=(price+1)>>1;
}

static void shop_set_image(struct layer *layer,uint16_t imageid,uint8_t srcx,uint8_t srcy) {
  struct widget *widget=menu_widget_by_id(LAYER->menu,SHOP_ID_IMAGE);
  if (!widget) return;
  widget_decal_setup_image(widget,imageid,srcx,srcy,widget->w,widget->h);
}

static void shop_set_greeting(struct layer *layer,uint16_t rid,uint16_t ix) {
  LAYER->dialogue_rid=rid;
  LAYER->dialogue_ix=ix;
  if (LAYER->state==0) {
    struct widget *widget=menu_widget_by_id(LAYER->menu,SHOP_ID_DIALOGUE);
    if (!widget) return;
    widget_decal_setup_string(widget,LAYER->dialogue_rid,LAYER->dialogue_ix,widget->w,0xffffffff);
  }
}

/* Setup.
 */
 
int layer_shop_setup(struct layer *layer,int shopid) {
  if (!layer||(layer->type!=&layer_type_shop)||(shopid<1)) return -1;
  LAYER->rid=shopid;
  const uint8_t *src=0;
  int srcc=sauces_res_get(&src,EGG_TID_shop,shopid);
  if (srcc<1) {
    fprintf(stderr,"shop:%d not found\n",shopid);
    return -1;
  }
  struct rom_command_reader reader={.v=src,.c=srcc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_shop_item: shop_add_item(layer,cmd.argv[0],cmd.argv[1]); break;
      case CMD_shop_sellonly: LAYER->salepricev[cmd.argv[0]]=cmd.argv[1]; break;
      case CMD_shop_image: shop_set_image(layer,(cmd.argv[0]<<8)|cmd.argv[1],cmd.argv[2],cmd.argv[3]); break;
      case CMD_shop_greeting: shop_set_greeting(layer,(cmd.argv[0]<<8)|cmd.argv[1],(cmd.argv[2]<<8)|cmd.argv[3]); break;
    }
  }
  return 0;
}
