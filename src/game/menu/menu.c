#include "game/sauces.h"
#include "game/layer/layer.h" /* for the button macros */
#include "menu.h"

// It takes a fixed amount of time to move the cursor, regardless of how far it's moving.
#define CURSOR_RATE 8.0 /* Hz, ie 1/s */

/* Delete.
 */
 
void menu_del(struct menu *menu) {
  if (!menu) return;
  if (menu->widgetv) {
    while (menu->widgetc-->0) {
      menu->widgetv[menu->widgetc]->menu=0;
      widget_del(menu->widgetv[menu->widgetc]);
    }
    free(menu->widgetv);
  }
  free(menu);
}

/* New.
 */
 
struct menu *menu_new() {
  struct menu *menu=calloc(1,sizeof(struct menu));
  if (!menu) return 0;
  menu->animated=1;
  menu->focus.t=1.0;
  menu->dirty=1;
  return menu;
}

/* Get animated cursor position.
 */
 
static void menu_get_cursor(int *x,int *y,int *w,int *h,const struct menu *menu) {
  if (menu->focus.t<=0.0) {
    *x=menu->focus.x0;
    *y=menu->focus.y0;
    *w=menu->focus.w0;
    *h=menu->focus.h0;
  } else if (menu->focus.t>=1.0) {
    *x=menu->focus.x1;
    *y=menu->focus.y1;
    *w=menu->focus.w1;
    *h=menu->focus.h1;
  } else {
    double w0=1.0-menu->focus.t;
    *x=menu->focus.x0*w0+menu->focus.x1*menu->focus.t;
    *y=menu->focus.y0*w0+menu->focus.y1*menu->focus.t;
    *w=menu->focus.w0*w0+menu->focus.w1*menu->focus.t;
    *h=menu->focus.h0*w0+menu->focus.h1*menu->focus.t;
  }
}

/* Terminate animation and reset focus bounds.
 */

void menu_force_focus_bounds(struct menu *menu) {
  if (!menu->focus.widget) return;
  menu->focus.t=1.0;
  menu->focus.x1=menu->focus.widget->x;
  menu->focus.y1=menu->focus.widget->y;
  menu->focus.w1=menu->focus.widget->w;
  menu->focus.h1=menu->focus.widget->h;
}

/* Set focus.
 */
 
void menu_set_focus(struct menu *menu,struct widget *widget) {
  if (widget==menu->focus.widget) return;
  if (menu->focus.widget&&menu->focus.widget->type->focus) {
    menu->focus.widget->type->focus(widget,0);
  }
  int cx,cy,cw,ch;
  menu_get_cursor(&cx,&cy,&cw,&ch,menu);
  if (widget) {
    menu->focus.widget=widget;
    menu->focus.x1=menu->focus.widget->x;
    menu->focus.y1=menu->focus.widget->y;
    menu->focus.w1=menu->focus.widget->w;
    menu->focus.h1=menu->focus.widget->h;
    if (cw&&ch&&menu->animated) {
      menu->focus.x0=cx;
      menu->focus.y0=cy;
      menu->focus.w0=cw;
      menu->focus.h0=ch;
      menu->focus.t=0.0;
    } else {
      menu->focus.t=1.0;
    }
  } else {
    menu->focus.widget=0;
    menu->focus.w1=0;
    menu->focus.h1=0;
    menu->focus.t=0.0;
  }
  if (widget&&widget->type->focus) {
    widget->type->focus(widget,1);
  }
}

/* Find and focus the default widget.
 */
 
static void menu_focus_default(struct menu *menu) {
  int i=0; for (;i<menu->widgetc;i++) {
    struct widget *widget=menu->widgetv[i];
    if (widget->accept_focus) {
      menu_set_focus(menu,widget);
      break;
    }
  }
}

/* React to changed widget list, if it changed.
 */
 
static void menu_clean(struct menu *menu) {
  if (!menu->dirty) return;
  menu->dirty=0;
  
  // If we have a focus, confirm that it still exists and update bounds.
  if (menu->focus.widget) {
    int ok=0,i=menu->widgetc;
    while (i-->0) if (menu->widgetv[i]==menu->focus.widget) { ok=1; break; }
    if (!ok) {
      // Drop without calling the focus hook, since the widget is presumably deleted.
      menu->focus.widget=0;
      menu->focus.t=1.0;
      menu->focus.w1=0;
      menu->focus.h1=0;
    } else {
      menu->focus.x1=menu->focus.widget->x;
      menu->focus.y1=menu->focus.widget->y;
      menu->focus.w1=menu->focus.widget->w;
      menu->focus.h1=menu->focus.widget->h;
    }
  }
  
  // If we do not have a focus (possibly having been dropped in the prior step), check for a default.
  if (!menu->focus.widget) {
    menu_focus_default(menu);
  }
  
  //TODO Opportunity here to sort by (imageid) to reduce texture churn. Need a way to explicitly mark certain order constraints.
}

/* Update.
 */

void menu_update(struct menu *menu,double elapsed) {
  menu_clean(menu);
  if (menu->focus.t<1.0) {
    if ((menu->focus.t+=elapsed*CURSOR_RATE)>=1.0) {
      menu->focus.t=1.0;
    }
  }
  //TODO Will widgets need update?
}

/* Which of these two is nearer the reference point, for the given direction of motion? (-1,0,1)
 * We compare the *center* of the source to the *edge* of the destination, so comparing
 * a widget to itself does yield a sensible high answer, and requires no further intervention.
 */
 
static int menu_pick_neighbor(const struct widget *a,const struct widget *b,int refx,int refy,int dx,int dy) {
  int adist,bdist;
  if (dx<0) {
    if ((adist=refx-a->w-a->x)<0) adist+=FBW;
    if ((bdist=refx-b->w-b->x)<0) bdist+=FBW;
  } else if (dx>0) {
    if ((adist=a->x-refx)<0) adist+=FBW;
    if ((bdist=b->x-refx)<0) bdist+=FBW;
  } else if (dy<0) {
    if ((adist=refy-a->h-a->y)<0) adist+=FBH;
    if ((bdist=refy-b->h-b->y)<0) bdist+=FBH;
  } else {
    if ((adist=a->y-refy)<0) adist+=FBH;
    if ((bdist=b->y-refy)<0) bdist+=FBH;
  }
  if (dx) {
    int amid=a->y+(a->h>>1);
    int bmid=b->y+(b->h>>1);
    int d;
    if ((d=amid-refy)<0) d=-d; adist+=d<<1;
    if ((d=bmid-refy)<0) d=-d; bdist+=d<<1;
  } else {
    int amid=a->x+(a->w>>1);
    int bmid=b->x+(b->w>>1);
    int d;
    if ((d=amid-refx)<0) d=-d; adist+=d<<1;
    if ((d=bmid-refx)<0) d=-d; bdist+=d<<1;
  }
  if (adist<bdist) return -1;
  if (adist>bdist) return 1;
  return 0;
}

/* Find the neighbor for focus purposes, from a given widget and direction.
 * We do not guarantee that every widget will be reachable, but in a sane layout they should be.
 * Beware of layouts that place one outlier with no nearby widgets on either axis, those might be unreachable.
 * I've experimented with purely random layouts and it works out pretty good.
 */
 
static struct widget *menu_find_neighbor(const struct menu *menu,struct widget *from,int dx,int dy) {
  if (!menu||!from) return 0;
  if (!dx&&!dy) return from;
  int midx=from->x+(from->w>>1);
  int midy=from->y+(from->h>>1);
  struct widget *best=0;
  int i=menu->widgetc;
  while (i-->0) {
    struct widget *q=menu->widgetv[i];
    if (!q->accept_focus) continue;
    if (!best) best=q;
    else if (menu_pick_neighbor(q,best,midx,midy,dx,dy)<0) best=q;
  }
  if (!best) return from;
  return best;
}

/* Receive input.
 */

void menu_input(struct menu *menu,int input,int pvinput) {
  if (BTNDOWN(LEFT)) menu_move(menu,-1,0);
  if (BTNDOWN(RIGHT)) menu_move(menu,1,0);
  if (BTNDOWN(UP)) menu_move(menu,0,-1);
  if (BTNDOWN(DOWN)) menu_move(menu,0,1);
  // Allow that activate or cancel might cause us to delete, and get out immediately when we do it.
  // You can't activate and cancel in the same frame.
  if (BTNDOWN(SOUTH)) { menu_activate(menu); return; }
  if (BTNDOWN(WEST)) { menu_cancel(menu); return; }
}

void menu_activate(struct menu *menu) {
  menu_clean(menu);
  menu->focus.t=1.0; // If animation was in progress, snap it forward. Important if the callback is going to push a new layer, but always sensible.
  if (menu->focus.widget&&menu->focus.widget->cb) {
    menu->focus.widget->cb(menu->focus.widget);
  } else if (menu->cb_activate) {
    menu->cb_activate(menu,menu->focus.widget);
  }
}

void menu_cancel(struct menu *menu) {
  menu_clean(menu);
  if (menu->cb_cancel) menu->cb_cancel(menu);
}

void menu_move(struct menu *menu,int dx,int dy) {
  menu_clean(menu);
  egg_play_sound(RID_sound_uimotion);
  if (!menu->focus.widget) {
    menu_focus_default(menu);
  } else {
    struct widget *next=menu_find_neighbor(menu,menu->focus.widget,dx,dy);
    if (next) menu_set_focus(menu,next);
  }
}

/* Render cursor.
 */
 
static void menu_render_cursor(struct menu *menu,int x,int y,int w,int h) {
  graf_draw_rect(&g.graf,x,y,w,1,0xffff00ff);
  graf_draw_rect(&g.graf,x,y,1,h,0xffff00ff);
  graf_draw_rect(&g.graf,x+w-1,y,1,h,0xffff00ff);
  graf_draw_rect(&g.graf,x,y+h-1,w,1,0xffff00ff);
}

/* Render.
 */

void menu_render(struct menu *menu) {
  menu_clean(menu);
  int i=0; for (;i<menu->widgetc;i++) {
    struct widget *widget=menu->widgetv[i];
    if (widget->type->render) widget->type->render(widget);
  }
  int cx,cy,cw,ch;
  menu_get_cursor(&cx,&cy,&cw,&ch,menu);
  if ((cw>0)&&(ch>0)) {
    menu_render_cursor(menu,cx,cy,cw,ch);
  }
}

/* Add widget.
 */

int menu_add_widget(struct menu *menu,struct widget *widget) {
  if (!menu||!widget) return -1;
  int i=menu->widgetc;
  while (i-->0) if (menu->widgetv[i]==widget) return 0;
  if (widget->menu) return -1;
  if (menu->widgetc>=menu->widgeta) {
    int na=menu->widgeta+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(menu->widgetv,sizeof(void*)*na);
    if (!nv) return -1;
    menu->widgetv=nv;
    menu->widgeta=na;
  }
  menu->widgetv[menu->widgetc++]=widget;
  menu->dirty=1;
  widget->menu=menu;
  return 0;
}

/* Remove widget.
 * Handoff, do not delete.
 */
 
int menu_remove_widget(struct menu *menu,struct widget *widget) {
  if (!menu||!widget) return -1;
  if (widget->menu!=menu) return -1;
  int i=menu->widgetc;
  while (i-->0) {
    if (menu->widgetv[i]==widget) {
      menu->widgetc--;
      memmove(menu->widgetv+i,menu->widgetv+i+1,sizeof(void*)*(menu->widgetc-i));
      widget->menu=0;
      menu->dirty=1;
      return 0;
    }
  }
  return -1;
}

/* Generic widget API.
 * Implementing here because it's really no big deal.
 */
 
void widget_del(struct widget *widget) {
  if (!widget) return;
  if (widget->menu) fprintf(stderr,"!!!WARNING!!! Deleting widget %p with menu=%p, should be null by this point.\n",widget,widget->menu);
  if (widget->type->del) widget->type->del(widget);
  free(widget);
}

struct widget *widget_new(const struct widget_type *type) {
  if (!type) return 0;
  struct widget *widget=calloc(1,type->objlen);
  if (!widget) return 0;
  widget->type=type;
  widget->accept_focus=1;
  if (type->init&&(type->init(widget)<0)) {
    widget_del(widget);
    return 0;
  }
  return widget;
}

struct widget *widget_spawn(struct menu *menu,const struct widget_type *type) {
  struct widget *widget=widget_new(type);
  if (!widget) return 0;
  if (menu_add_widget(menu,widget)<0) {
    widget_del(widget);
    return 0;
  }
  return widget;
}
