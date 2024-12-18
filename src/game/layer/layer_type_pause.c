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

struct layer_pause {
  struct layer hdr;
  struct menu *menu;
  int16_t dstx,dsty,dstw,dsth; // Full bounds of blotter.
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
      case 16: break; // "Resume", ie do nothing extra.
      case 17: sauces_end_session(); return; // Beware this deletes layers immediately.
      default: if ((LAYER->setup==SETUP_GLOBAL)&&g.session&&(widget->id>=0)&&(widget->id<16)) {
          g.session->invp=widget->id;
        }
    }
  }
  layer_pop(layer);
}

/* Recalculate bounds based on menu.
 */
 
static void pause_auto_bounds(struct layer *layer) {
  if (LAYER->menu->widgetc<1) {
    layer_pop(layer);
    return;
  }
  int l=LAYER->menu->widgetv[0]->x;
  int t=LAYER->menu->widgetv[0]->y;
  int r=l+LAYER->menu->widgetv[0]->w;
  int b=t+LAYER->menu->widgetv[0]->h;
  int i=LAYER->menu->widgetc;
  struct widget *prompt=0;
  while (i-->1) {
    struct widget *widget=LAYER->menu->widgetv[i];
    if (widget->id<0) { // Negative ID is for the prompt. We move it to fit the rest.
      prompt=widget;
      continue;
    }
    if (widget->x<l) l=widget->x;
    if (widget->y<t) t=widget->y;
    int q;
    if ((q=widget->x+widget->w)>r) r=q;
    if ((q=widget->y+widget->h)>b) b=q;
  }
  if (prompt) {
    prompt->x=((l+r)>>1)-(prompt->w>>1);
    prompt->y=t-4-prompt->h;
  }
  const int margin=4;
  l-=margin;
  t-=margin;
  r+=margin;
  b+=margin;
  LAYER->dstw=r-l;
  LAYER->dsth=b-t;
  // We don't actually care where (l,t) landed now that we have the size; we want to stay centered always.
  int nx=(FBW>>1)-(LAYER->dstw>>1);
  int ny=(FBH>>1)-(LAYER->dsth>>1);
  int dx=nx-LAYER->dstx,dy=ny-LAYER->dsty;
  LAYER->dstx=nx;
  LAYER->dsty=ny;
  if (dx||dy) {
    for (i=LAYER->menu->widgetc;i-->0;) {
      struct widget *widget=LAYER->menu->widgetv[i];
      widget->x+=dx;
      widget->y+=dy;
    }
  }
  menu_force_focus_bounds(LAYER->menu);
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
  
  // There should be additional buttons depending on setup, but we always start with 16 inventory buttons.
  const int cellsize=NS_sys_tilesize+2;
  int x0=(FBW>>1)-(cellsize<<1)+(cellsize>>1);
  int y0=(FBH>>1)-(cellsize<<1)+(cellsize>>1);
  int row=0,y=y0,invp=0; for (;row<4;row++,y+=cellsize) {
    int col=0,x=x0; for (;col<4;col++,x+=cellsize,invp++) {
      struct widget *widget=widget_tile_spawn(LAYER->menu,x,y,RID_image_item,g.session->inventory[invp],0,0,0);
      if (!widget) return -1;
      widget->w=cellsize;
      widget->h=cellsize;
      widget->id=invp;
      if (g.session&&(invp==g.session->invp)) menu_set_focus(LAYER->menu,widget);
    }
  }
  LAYER->dstx=x0-(cellsize>>1);
  LAYER->dsty=y0-(cellsize>>1);
  
  //TODO I think some extra read-only chrome would be appropriate. Item name and description...
  //...actually yes, it's very important: User needs an easy way to see her items' densities. They're not obvious.
  
  pause_auto_bounds(layer);
  return 0;
}

/* Input.
 */
 
static void _pause_input(struct layer *layer,int input,int pvinput) {
  menu_input(LAYER->menu,input,pvinput);
}

static void _pause_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

/* Render.
 */
 
static void _pause_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000c0);
  graf_draw_rect(&g.graf,LAYER->dstx,LAYER->dsty,LAYER->dstw,LAYER->dsth,0x406080e0);
  menu_render(LAYER->menu);
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
  int x=LAYER->dstx+LAYER->dstw;
  int y=LAYER->dsty+4;
  struct widget *widget;
  if (!(widget=widget_decal_spawn_string(LAYER->menu,x,y,EGG_BTN_LEFT|EGG_BTN_UP,RID_strings_ui,15,FBW,0xffffffff,0,0))) return -1;
  y+=widget->h;
  widget->id=16;
  if (!(widget=widget_decal_spawn_string(LAYER->menu,x,y,EGG_BTN_LEFT|EGG_BTN_UP,RID_strings_ui,16,FBW,0xffffffff,0,0))) return -1;
  y+=widget->h;
  widget->id=17;
  pause_auto_bounds(layer);
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
    struct widget *button=widget_decal_spawn_text(LAYER->menu,LAYER->dstx+LAYER->dstw,LAYER->dsty+4,EGG_BTN_LEFT|EGG_BTN_UP,cancel,cancelc,FBW,0xffffffff,0,0);
    if (!button) return -1;
    button->id=16;
    button->accept_focus=1;
    promptlimit+=button->w;
  }
  if (!prompt) promptc=0; else if (promptc<0) { promptc=0; while (prompt[promptc]) promptc++; }
  if (promptc) {
    struct widget *label=widget_decal_spawn_text(LAYER->menu,LAYER->dstx+(LAYER->dstw>>1),LAYER->dsty,EGG_BTN_DOWN,prompt,promptc,promptlimit,0xffffffff,0,0);
    if (!label) return -1;
    label->id=-1;
    label->accept_focus=0;
  }
  LAYER->cb=cb;
  LAYER->userdata=userdata;
  pause_auto_bounds(layer);
  return 0;
}
