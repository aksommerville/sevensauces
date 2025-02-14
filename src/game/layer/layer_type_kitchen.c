/* layer_type_kitchen.c
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/kitchen/kitchen.h"
#include "game/menu/menu.h"
#include "layer.h"

#define KITCHEN_ID_INVA 0
#define KITCHEN_ID_INVZ 15
#define KITCHEN_ID_READY 16
#define KITCHEN_ID_ADVICE 17

struct layer_kitchen {
  struct layer hdr;
  struct menu *menu;
  int details_texid;
  int details_w,details_h; // Zero to skip render, otherwise (79,91), the full available space.
  int details_dirty; // Rebuilds texture at the last moment.
  struct widget *focus; // WEAK, tracks menu focus so we know when details are dirty.
  struct hourglass *hourglass;
};

#define LAYER ((struct layer_kitchen*)layer)

/* Cleanup.
 */
 
static void _kitchen_del(struct layer *layer) {
  menu_del(LAYER->menu);
  if (LAYER->details_texid) egg_texture_del(LAYER->details_texid);
  hourglass_del(LAYER->hourglass);
}

/* Ready.
 */
 
static void kitchen_on_ready(struct layer *layer) {
  int approval=kitchen_requires_approval(g.kitchen);
  if (approval) {
    fprintf(stderr,"*** Kitchen requires approval, condition %d. Proceeding anyway. ***\n",approval);//TODO
  }
  if (!layer_spawn(&layer_type_eod)) {
    sauces_end_night();
  }
}

/* Advice.
 */
 
static void kitchen_on_advice(struct layer *layer) {
  fprintf(stderr,"%s\n",__func__);//TODO
}

/* Menu callbacks.
 */
 
static void kitchen_cb_activate(struct menu *menu,struct widget *widget) {
  struct layer *layer=menu->userdata;
  if (!widget) return;
  
  if ((widget->id>=KITCHEN_ID_INVA)&&(widget->id<=KITCHEN_ID_INVZ)) {
    int invp=widget->id-KITCHEN_ID_INVA;
    if (g.kitchen->selected[invp]) {
      egg_play_sound(RID_sound_unselect_ingredient);
      g.kitchen->selected[invp]=0;
    } else if (g.session->inventory[invp]) {
      egg_play_sound(RID_sound_select_ingredient);
      g.kitchen->selected[invp]=1;
    } else {
      egg_play_sound(RID_sound_reject);
    }
  
  } else switch (widget->id) {
    case KITCHEN_ID_READY: kitchen_on_ready(layer); break;
    case KITCHEN_ID_ADVICE: kitchen_on_advice(layer); break;
  }
}
 
static void kitchen_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
}

/* Init.
 */
 
static int _kitchen_init(struct layer *layer) {
  if (!g.kitchen||!g.session) return -1;
  layer->opaque=1;
  if (!(LAYER->hourglass=hourglass_new())) return -1;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->userdata=layer;
  LAYER->menu->cb_activate=kitchen_cb_activate;
  LAYER->menu->cb_cancel=kitchen_cb_cancel;
  struct widget *widget;
  
  /* Populate menu from inventory.
   * Include the zeroes. It's disconcerting to have focus skip over them.
   */
  int invp=0; for (;invp<16;invp++) {
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    int x=invp&3,y=invp>>2;
    x*=NS_sys_tilesize+2;
    y*=NS_sys_tilesize+5;
    x+=156+(NS_sys_tilesize>>1);
    y+=10+(NS_sys_tilesize>>1);
    if (!(widget=widget_tile_spawn(LAYER->menu,x,y,RID_image_item,itemid,0,0,0))) return -1;
    widget->id=KITCHEN_ID_INVA+invp;
    widget->w+=2;
    widget->h+=2;
  }
   
  /* "Ready" and "Advice" buttons.
   */
  if (!(widget=widget_decal_spawn_string(LAYER->menu,110,15,0,RID_strings_ui,29,100,0xffffffff,0,0))) return -1;
  widget->id=KITCHEN_ID_READY;
  if (!(widget=widget_decal_spawn_string(LAYER->menu,110,33,0,RID_strings_ui,30,100,0xffffffff,0,0))) return -1;
  widget->id=KITCHEN_ID_ADVICE;
  
  menu_focus_id(LAYER->menu,g.session->invp-KITCHEN_ID_INVA); // (invp) is allowed to be an empty slot. In that case, we don't care where focus begins.
  egg_play_song(RID_song_watcher_of_pots,0,1);
  
  return 0;
}

/* Input.
 */
 
static void _kitchen_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _kitchen_update(struct layer *layer,double elapsed) {

  g.kitchen->clock+=elapsed;
  g.session->kitchentime+=elapsed;

  menu_update(LAYER->menu,elapsed);
  
  struct widget *nfocus=LAYER->menu->focus.widget;
  if (nfocus==LAYER->focus) {
  } else if (nfocus) {
    int invp=nfocus->id-KITCHEN_ID_INVA;
    if ((invp>=0)&&(invp<INVENTORY_SIZE)) {
      LAYER->focus=nfocus;
      LAYER->details_dirty=1;
    } else {
      LAYER->focus=0;
      LAYER->details_w=0;
    }
  } else {
    LAYER->focus=0;
    LAYER->details_w=0;
  }
}

/* Rebuild the item-details texture.
 */
 
static void kitchen_render_details(uint8_t *rgba,int dstw,int dsth,int stride,uint8_t itemid,const struct item *item) {
  const int xmargin=2;
  const int lineh=10;
  int availw=dstw-(xmargin<<1);
  int dsty=1;
  char text[1024];
  int textc=sauces_generate_item_description(text,sizeof(text),itemid,ITEM_DESC_COLOR);
  if ((textc>0)&&(textc<=sizeof(text))) {
    struct font_line linev[32];
    int linec=font_break_lines(linev,sizeof(linev)/sizeof(linev[0]),g.font,text,textc,availw);
    if (linec>sizeof(linev)/sizeof(linev[0])) linec=sizeof(linev)/sizeof(linev[0]);
    uint32_t color=sauces_item_description_colorv[0];
    struct font_line *line=linev;
    int i=linec;
    for (;i-->0;line++) {
      if ((line->c>=2)&&(line->v[0]==0x7f)) {
        int ix=line->v[1]-'0';
        if ((ix>=0)&&(ix<10)) color=sauces_item_description_colorv[ix];
        line->v+=2;
        line->c-=2;
      }
      font_render_string(rgba,dstw,dsth,stride,xmargin,dsty,g.font,line->v,line->c,color);
      dsty+=lineh;
    }
  }
}
 
static void kitchen_rebuild_details(struct layer *layer) {
  LAYER->details_w=0; // "no texture", and we can give up at any time.
  if (!LAYER->focus) return;
  int invp=LAYER->focus->id-KITCHEN_ID_INVA;
  if ((invp<0)||(invp>=INVENTORY_SIZE)) return;
  uint8_t itemid=g.session->inventory[invp];
  if (!itemid) return;
  const struct item *item=itemv+itemid;
  if (!item->flags) return;
  if (!LAYER->details_texid) {
    if ((LAYER->details_texid=egg_texture_new())<1) {
      LAYER->details_texid=0;
      return;
    }
  }
  LAYER->details_w=79;
  LAYER->details_h=91;
  int stride=LAYER->details_w<<2;
  uint8_t *rgba=calloc(stride,LAYER->details_h);
  if (!rgba) { LAYER->details_w=0; return; }
  kitchen_render_details(rgba,LAYER->details_w,LAYER->details_h,stride,itemid,item);
  if (egg_texture_load_raw(LAYER->details_texid,LAYER->details_w,LAYER->details_h,stride,rgba,stride*LAYER->details_h)<0) {
    LAYER->details_w=0;
  }
  free(rgba);
}

/* Render customers.
 */
 
static void kitchen_render_table(struct layer *layer,int y) {
  // Colors should match image:kitchen_large but it's not life-or-death.
  const uint32_t line_color=0x493c29ff;
  const uint32_t surface_color=0xa48760ff;
  const uint32_t face_color=0x786142ff;
  const int surfaceh=12; // Include the top line.
  graf_draw_rect(&g.graf,0,y,FBW,surfaceh,surface_color);
  graf_draw_rect(&g.graf,0,y+surfaceh,FBW,FBH,face_color);
  graf_draw_line(&g.graf,0,y,FBW,y,line_color);
  graf_draw_line(&g.graf,0,y+surfaceh,FBW,y+surfaceh,line_color);
}

static void kitchen_render_customer(struct layer *layer,int x,int y,const struct kcustomer *kcustomer,int texid) {
  int srcx=1,srcy=69+41*(kcustomer->race-1);
  graf_draw_decal(&g.graf,texid,x,y,srcx,srcy,40,40,0);
}
 
static void kitchen_render_customers(struct layer *layer,int texid) {
  const int cellsize=40;
  const int ytop=FBH-cellsize*2; // Three rows but the middle row is staggered so effectively two.
  const int tabletop=27; // Distance from cell top to table line. Must agree with image:kitchen_bits.
  
  /* First draw the three tables.
   * These don't need to occlude anything except each other; diners are drawn with transparency where the table goes.
   */
  kitchen_render_table(layer,ytop+tabletop);
  kitchen_render_table(layer,ytop+(cellsize>>1)+tabletop);
  kitchen_render_table(layer,ytop+cellsize+tabletop);
  
  /* Customers are in three rows: 8,7,8. The middle row starts half a cell in.
   */
  struct kcustomer *kcustomer=g.kitchen->kcustomerv;
  int x,i;
  for (x=0,i=8;i-->0;kcustomer++,x+=cellsize) {
    if (!kcustomer->race) continue;
    kitchen_render_customer(layer,x,ytop,kcustomer,texid);
  }
  for (x=cellsize>>1,i=7;i-->0;kcustomer++,x+=cellsize) {
    if (!kcustomer->race) continue;
    kitchen_render_customer(layer,x,ytop+(cellsize>>1),kcustomer,texid);
  }
  for (x=0,i=8;i-->0;kcustomer++,x+=cellsize) {
    if (!kcustomer->race) continue;
    kitchen_render_customer(layer,x,ytop+cellsize,kcustomer,texid);
  }
}

/* Compose the text for the front of the cauldron, as tiles from image:kitchen_large.
 */
 
static void kitchen_write_status_tiles(uint8_t *dst,int colc,int rowc,const struct layer *layer) {
  int ingc=0,vegc=0,meatc=0,candyc=0;
  const uint8_t *itemidv=g.session->inventory;
  const uint8_t *selv=g.kitchen->selected;
  int i=INVENTORY_SIZE;
  for (;i-->0;itemidv++,selv++) {
    if (!*selv) continue;
    const struct item *item=itemv+(*itemidv);
    ingc+=item->density;
    switch (item->foodgroup) {
      case NS_foodgroup_veg: vegc++; break;
      case NS_foodgroup_meat: meatc++; break;
      case NS_foodgroup_candy: candyc++; break;
      case NS_foodgroup_sauce: break;
    }
  }
  int bowlc=g.session->customerc;
  int dstp=colc>>1;
  
  #define DECIMAL2(_n) { \
    int n=_n; \
    if (n>99) n=99; \
    if (n>=10) dst[dstp++]=0xf0+(n/10)%10; \
    dst[dstp++]=0xf0+n%10; \
  }
  
  /* Helpings count can go over 100.
   * Everything else, it's fine to clamp at 99, they shouldn't be able to reach that high.
   */
  dst[dstp]=0xfa; // slash, separating ingc from bowlc
  dstp++;
  DECIMAL2(bowlc)
  dstp=colc>>1;
  dstp--;
  dst[dstp]=0xf0+ingc%10;
  int limit=10,div=1;
  while (ingc>=limit) {
    dstp--;
    div=limit;
    limit*=10;
    dst[dstp]=0xf0+(ingc/div)%10;
  }
  
  /* The natural ingredient counts by food group realistically will always be one digit.
   * But it is possible to exceed 10 so we allow it.
   */
  dstp=colc+2;
  dst[dstp++]=0xfc; // leaf
  DECIMAL2(vegc)
  dstp=colc+6;
  dst[dstp++]=0xfd; // pig
  DECIMAL2(meatc)
  dstp=colc+10;
  dst[dstp++]=0xfe; // candy
  DECIMAL2(candyc)
  
  #undef DECIMAL3
}

/* Render, main.
 */
 
static void _kitchen_render(struct layer *layer) {
  if (!g.kitchen) return;

  /* Background base, one image filling the framebuffer.
   */
  int texid=texcache_get_image(&g.texcache,RID_image_kitchen_bg);
  graf_draw_decal(&g.graf,texid,0,0,0,0,FBW,FBH,0);
  graf_flush(&g.graf);//XXX We shouldn't have to flush here but bg was getting evicted prematurely. That needs to be fixed in Egg, and the graf/texcache API is going to change a little.
  texid=texcache_get_image(&g.texcache,RID_image_kitchen_bits);
  
  /* Little Sister sits near the left edge of the cauldron.
   */
  //TODO
  
  /* Bubbles and bobbing items floating in the stew.
   */
  //TODO
  
  /* Highlighted item details, right side.
   */
  if (LAYER->details_dirty) {
    LAYER->details_dirty=0;
    kitchen_rebuild_details(layer);
  }
  if (LAYER->details_w) {
    int dstx=237;
    int dsty=5;
    graf_draw_decal(&g.graf,LAYER->details_texid,dstx,dsty,0,0,LAYER->details_w,LAYER->details_h,0);
  }
  
  /* Menu handles the pantry items, and "Cook" and "Advice" buttons.
   */
  menu_render(LAYER->menu);
  
  /* Clock in the top left corner.
   */
  hourglass_render(1,1,LAYER->hourglass,g.kitchen->clock/g.kitchen->clock_limit);
  
  /* Word bubble, if Little Sister is talking.
   */
  //TODO
  
  /* Customers.
   */
  kitchen_render_customers(layer,texid);
  
  /* Current stew content, on the front of the cauldron.
   * Doing this immediately after customers, since it uses image:kitchen_large.
   * It works out to be a 14x2 grid.
   */
  {
    texid=texcache_get_image(&g.texcache,RID_image_kitchen_large);
    uint8_t map[14*2];
    memset(map,0xfb,sizeof(map)); // tile 0xfb deliberately empty.
    kitchen_write_status_tiles(map,14,2,layer);
    graf_draw_tile_buffer(&g.graf,texid,23,83,map,14,2,14);
  }
  
  /* Checkmarks on the chosen items.
   */
  texid=texcache_get_image(&g.texcache,RID_image_item);
  int i=LAYER->menu->widgetc;
  while (i-->0) {
    struct widget *widget=LAYER->menu->widgetv[i];
    int invp=widget->id-KITCHEN_ID_INVA;
    if ((invp<0)||(invp>=INVENTORY_SIZE)) continue;
    if (!(g.kitchen->selected[invp])) continue;
    int16_t dstx=widget->x+(widget->w>>1)+4;
    int16_t dsty=widget->y+(widget->h>>1)+4;
    graf_draw_tile(&g.graf,texid,dstx,dsty,0xfe,0);
  }
}

/* Type definition.
 */
 
const struct layer_type layer_type_kitchen={
  .name="kitchen",
  .objlen=sizeof(struct layer_kitchen),
  .del=_kitchen_del,
  .init=_kitchen_init,
  .input=_kitchen_input,
  .update=_kitchen_update,
  .render=_kitchen_render,
};
