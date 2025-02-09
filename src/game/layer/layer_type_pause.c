/* layer_type_pause.c
 * Summoned on demand during diurnal play.
 * We're the regular pause menu, and also any "pick an item" case.
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/menu/menu.h"
#include "layer.h"

#define SETUP_GLOBAL 1
#define SETUP_QUERY 2

// Widget IDs.
#define PAUSE_ID_INVA 0
#define PAUSE_ID_INVZ 15
#define PAUSE_ID_PROMPT 16
#define PAUSE_ID_RESUME 17
#define PAUSE_ID_MENU 18

#define CELLSIZE (NS_sys_tilesize+2)

struct layer_pause {
  struct layer hdr;
  struct menu *menu;
  int16_t dstx,dsty,dstw,dsth; // Full bounds of blotter.
  int16_t descx,descy,descw,desch; // Item description, absolute framebuffer coords.
  int desc_dirty; // Nonzero if bounds or selection changed. Gets freshened during render, if needed.
  struct widget *focus_item; // WEAK. Tracks (menu->focus.widget), or null if it's not inventory.
  int desc_texid;
  int setup;
  void (*cb)(int invp,void *userdata);
  void *userdata;
};

#define LAYER ((struct layer_pause*)layer)

/* Cleanup.
 */
 
static void _pause_del(struct layer *layer) {
  if (LAYER->cb) {
    LAYER->cb(-1,LAYER->userdata);
    LAYER->cb=0;
  }
  menu_del(LAYER->menu);
  if (LAYER->desc_texid) egg_texture_del(LAYER->desc_texid);
}

/* Menu callbacks.
 */
 
static void pause_cb_cancel(struct menu *menu) {
  struct layer *layer=menu->userdata;
  if (LAYER->cb) {
    LAYER->cb(-1,LAYER->userdata);
    LAYER->cb=0;
  }
  layer_pop(layer);
}

static void pause_cb_activate(struct menu *menu,struct widget *widget) {
  struct layer *layer=menu->userdata;
  if (widget) {
    if (LAYER->cb) {
      LAYER->cb(widget->id,LAYER->userdata);
      LAYER->cb=0;
    }
    switch (widget->id) {
      case PAUSE_ID_RESUME: break; // "Resume", ie do nothing extra.
      case PAUSE_ID_MENU: sauces_end_session(); return; // Beware this deletes layers immediately.
      default: if ((LAYER->setup==SETUP_GLOBAL)&&g.session&&(widget->id>=0)&&(widget->id<16)) {
          g.session->invp=widget->id;
        }
    }
  }
  layer_pop(layer);
}

/* Pack.
 */
 
static void pause_pack(struct layer *layer) {
  
  /* Initially assume we have the 4x4 grid, then expand per present widgets.
   */
  const int outer_margin=2;
  const int prompt_margin=0;
  const int right_margin=2;
  const int rightw_min=NS_sys_tilesize*5; // Allow lots of right space, for the item description.
  LAYER->dstw=CELLSIZE*4;
  LAYER->dsth=CELLSIZE*4;
  int rightw=rightw_min,toph=0;
  struct widget *prompt=0;
  int i=LAYER->menu->widgetc;
  while (i-->0) {
    struct widget *widget=LAYER->menu->widgetv[i];
    switch (widget->id) {
      case PAUSE_ID_PROMPT: prompt=widget; if (widget->h>toph) toph=widget->h; break;
      case PAUSE_ID_RESUME: if (widget->w>rightw) rightw=widget->w; break;
      case PAUSE_ID_MENU: if (widget->w>rightw) rightw=widget->w; break;
    }
  }
  if (toph) toph+=prompt_margin;
  LAYER->dsth+=toph;
  if (rightw) rightw+=right_margin;
  LAYER->dstw+=rightw;
  if (prompt) {
    int minw=prompt->w+(outer_margin<<1);
    if (LAYER->dstw<minw) LAYER->dstw=minw;
  }
  LAYER->dstw+=outer_margin<<1;
  LAYER->dsth+=outer_margin<<1;
  LAYER->dstx=(FBW>>1)-(LAYER->dstw>>1);
  LAYER->dsty=(FBH>>1)-(LAYER->dsth>>1);
  
  /* Place widgets.
   */
  int righty=LAYER->dsty+outer_margin+toph;
  for (i=0;i<LAYER->menu->widgetc;i++) {
    struct widget *widget=LAYER->menu->widgetv[i];
    if ((widget->id>=PAUSE_ID_INVA)&&(widget->id<=PAUSE_ID_INVZ)) {
      int invp=widget->id-PAUSE_ID_INVA;
      int col=invp&3;
      int row=invp>>2;
      widget->x=LAYER->dstx+outer_margin+col*CELLSIZE;
      widget->y=LAYER->dsty+outer_margin+toph+row*CELLSIZE;
    } else switch (widget->id) {
      case PAUSE_ID_PROMPT: {
          widget->x=LAYER->dstx+outer_margin;
          widget->y=LAYER->dsty+outer_margin;
          widget->w=LAYER->dstw-(outer_margin<<1);
        } break;
      case PAUSE_ID_RESUME:
      case PAUSE_ID_MENU: {
          widget->x=LAYER->dstx+outer_margin+CELLSIZE*4+right_margin;
          widget->y=righty;
          righty+=widget->h;
        } break;
    }
  }
  
  /* The description of highlighted item in the lower right is not a widget.
   * Consume all available lower-right space for it.
   * These bounds can go negative. If it's too small, it won't be rendered.
   */
  LAYER->descx=LAYER->dstx+outer_margin+CELLSIZE*4+right_margin+3; // The string widgets have a little built-in margin too.
  LAYER->descy=righty;
  LAYER->descw=LAYER->dstx+LAYER->dstw-outer_margin-LAYER->descx;
  LAYER->desch=LAYER->dsty+LAYER->dsth-outer_margin-LAYER->descy;
  LAYER->desc_dirty=1;
}

/* Init.
 */
 
static int _pause_init(struct layer *layer) {
  if (!g.session) return -1;
  layer->opaque=0;
  if (!(LAYER->menu=menu_new())) return -1;
  LAYER->menu->cb_cancel=pause_cb_cancel;
  LAYER->menu->cb_activate=pause_cb_activate;
  LAYER->menu->userdata=layer;
  
  /* Generate the 16 inventory widgets.
   * Prompt and the text buttons are created by ctors, after this.
   */
  int i=PAUSE_ID_INVA;
  for (;i<=PAUSE_ID_INVZ;i++) {
    struct widget *widget=widget_tile_spawn(LAYER->menu,0,0,RID_image_item,g.session->inventory[i-PAUSE_ID_INVA],0,0,0);
    if (!widget) return -1;
    widget->w=CELLSIZE;
    widget->h=CELLSIZE;
    widget->id=i;
    if (g.session&&(i-PAUSE_ID_INVA==g.session->invp)) menu_set_focus(LAYER->menu,widget);
  }
  
  /* Pack, just in case there's no ctor.
   * So this will typically happen twice for each menu. Don't worry, it's cheapish.
   */
  pause_pack(layer);
  
  return 0;
}

/* Input.
 */
 
static void _pause_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _pause_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
  
  // Has the focus changed?
  if (LAYER->focus_item!=LAYER->menu->focus.widget) {
    struct widget *nfocus=LAYER->menu->focus.widget;
    if (!nfocus||(nfocus->id<PAUSE_ID_INVA)||(nfocus->id>PAUSE_ID_INVZ)) nfocus=0;
    if (LAYER->focus_item!=nfocus) {
      LAYER->focus_item=nfocus;
      LAYER->desc_dirty=1;
    }
  }
}

/* Refresh the item description texture.
 * It's always exactly the available output size.
 */
 
static void pause_generate_item_desc_texture(struct layer *layer) {
  if (!LAYER->desc_texid) {
    if ((LAYER->desc_texid=egg_texture_new())<=0) {
      LAYER->desc_texid=0;
      return;
    }
  }
  int itemid=-1;
  if (g.session&&LAYER->focus_item) {
    if ((LAYER->focus_item->id>=PAUSE_ID_INVA)&&(LAYER->focus_item->id<=PAUSE_ID_INVZ)) {
      itemid=g.session->inventory[LAYER->focus_item->id-PAUSE_ID_INVA];
    }
  }
  
  /* Compose the content first as text, and let font break lines.
   * We're operating in a rather small space with unknown content; got to assume some item names will need wrapped.
   * Lines may begin U+7f followed by '0'..'7' to select a color.
   */
  char text[1024];
  int textc=sauces_generate_item_description(text,sizeof(text),itemid,ITEM_DESC_COLOR);
  struct font_line linev[32];
  int linec=font_break_lines(linev,sizeof(linev)/sizeof(linev[0]),g.font6,text,textc,LAYER->descw);
  if (linec>sizeof(linev)/sizeof(linev[0])) linec=sizeof(linev)/sizeof(linev[0]);
  
  uint8_t *rgba=calloc(LAYER->descw<<2,LAYER->desch);
  if (!rgba) return;
  
  int y=0,i=linec;
  uint32_t color=sauces_item_description_colorv[0];
  struct font_line *line=linev;
  for (;i-->0;line++,y+=7) {
    if ((line->c>=2)&&(line->v[0]==0x7f)) {
      int ix=line->v[1]-'0';
      if ((ix>=0)&&(ix<10)) color=sauces_item_description_colorv[ix];
      line->v+=2;
      line->c-=2;
    }
    font_render_string(rgba,LAYER->descw,LAYER->desch,LAYER->descw<<2,0,y,g.font6,line->v,line->c,color);
  }
  
  egg_texture_load_raw(LAYER->desc_texid,LAYER->descw,LAYER->desch,LAYER->descw<<2,rgba,LAYER->descw*LAYER->desch*4);
  free(rgba);
}

/* Render description of selected item, in lower right corner.
 * This is entirely static, so it doesn't interact with menu.
 */
 
static void pause_render_item_desc(struct layer *layer) {
  if (LAYER->descw<1) return;
  if (LAYER->desch<1) return;
  if (LAYER->desc_dirty) {
    LAYER->desc_dirty=0;
    pause_generate_item_desc_texture(layer);
  }
  graf_draw_decal(&g.graf,LAYER->desc_texid,LAYER->descx,LAYER->descy,0,0,LAYER->descw,LAYER->desch,0);
}

/* Render.
 */
 
static void _pause_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000c0);
  graf_draw_rect(&g.graf,LAYER->dstx,LAYER->dsty,LAYER->dstw,LAYER->dsth,0x406080e0);
  menu_render(LAYER->menu);
  pause_render_item_desc(layer);
}

/* Type definition.
 */
 
const struct layer_type layer_type_pause={
  .name="pause",
  .objlen=sizeof(struct layer_pause),
  .del=_pause_del,
  .init=_pause_init,
  .input=_pause_input,
  .update=_pause_update,
  .render=_pause_render,
};

/* Setup.
 */
 
int layer_pause_setup_global(struct layer *layer) {
  if (!layer||(layer->type!=&layer_type_pause)) return -1;
  if (LAYER->setup) return -1;
  LAYER->setup=SETUP_GLOBAL;
  struct widget *widget;
  if (!(widget=widget_decal_spawn_string(LAYER->menu,0,0,EGG_BTN_LEFT|EGG_BTN_UP,RID_strings_ui,15,FBW,0xffffffff,0,0))) return -1;
  widget->id=PAUSE_ID_RESUME;
  if (!(widget=widget_decal_spawn_string(LAYER->menu,0,0,EGG_BTN_LEFT|EGG_BTN_UP,RID_strings_ui,16,FBW,0xffffffff,0,0))) return -1;
  widget->id=PAUSE_ID_MENU;
  pause_pack(layer);
  return 0;
}

int layer_pause_setup_query(
  struct layer *layer,
  const char *prompt,int promptc,
  const char *cancel,int cancelc,
  void (*cb)(int invp,void *userdata),void *userdata
) {
  if (!layer||(layer->type!=&layer_type_pause)) return -1;
  if (LAYER->setup) return -1;
  LAYER->setup=SETUP_QUERY;
  int promptlimit=NS_sys_tilesize*5;
  if (!cancel) cancelc=0; else if (cancelc<0) { cancelc=0; while (cancel[cancelc]) cancelc++; }
  if (cancelc) {
    struct widget *button=widget_decal_spawn_text(LAYER->menu,0,0,EGG_BTN_LEFT|EGG_BTN_UP,cancel,cancelc,FBW,0xffffffff,0,0);
    if (!button) return -1;
    button->id=PAUSE_ID_RESUME;
    promptlimit+=button->w;
  }
  if (!prompt) promptc=0; else if (promptc<0) { promptc=0; while (prompt[promptc]) promptc++; }
  if (promptc) {
    struct widget *label=widget_decal_spawn_text(LAYER->menu,0,0,EGG_BTN_DOWN,prompt,promptc,promptlimit,0xffffffff,0,0);
    if (!label) return -1;
    label->id=PAUSE_ID_PROMPT;
    label->accept_focus=0;
  }
  LAYER->cb=cb;
  LAYER->userdata=userdata;
  pause_pack(layer);
  return 0;
}
