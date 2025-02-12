/* menu.h
 * Menu is a crude UI generalization that can be used by a layer to offload some routine work.
 * Basically, any layer with a cursor should use menu instead of managing input and render by itself.
 * Unlike most general UI frameworks, there is no nesting here. Just "menu" with one set of "widget" in it.
 */
 
#ifndef MENU_H
#define MENU_H

struct menu;
struct widget;
struct widget_type;
struct item;

/* Menu: Top level interface.
 *************************************************************************/

struct menu {
// Public:
  int animated; // True by default. Set false if you will not be calling menu_update().
  void (*cb_cancel)(struct menu *menu);
  void (*cb_activate)(struct menu *menu,struct widget *widget); // Called only if (widget) has no callback. (widget) may be null.
  void *userdata;
// Private:
  struct widget **widgetv;
  int widgetc,widgeta;
  struct {
    int x0,y0,w0,h0; // Old bounds.
    int x1,y1,w1,h1; // New bounds.
    double t; // <1.0 if animating.
    struct widget *widget; // WEAK, always in (widgetv) too. Changes immediately when animation begins.
  } focus;
  int dirty; // If true, we'll reassess focus and render order before the next event or render.
};

void menu_del(struct menu *menu);
struct menu *menu_new();

void menu_update(struct menu *menu,double elapsed);

/* You can trigger input events directly, or provide current and previous state, as layers get it.
 */
void menu_input(struct menu *menu,int input,int pvinput);
void menu_activate(struct menu *menu); // SOUTH
void menu_cancel(struct menu *menu); // WEST
void menu_move(struct menu *menu,int dx,int dy); // LEFT|RIGHT|UP|DOWN

/* If you have elements that need to go above main content but below the cursor (eg shop),
 * render widgets, then your stuff, then cursor.
 * Most consumers can use plain render.
 */
void menu_render(struct menu *menu);
void menu_render_widgets(struct menu *menu);
void menu_render_cursor(struct menu *menu);

void menu_set_focus(struct menu *menu,struct widget *widget);
void menu_focus_id(struct menu *menu,int id);
void menu_force_focus_bounds(struct menu *menu); // Call if you move widgets. Terminates animation and resets cursor position.

/* Adding or removing a widget hands off ownership.
 * "delete" deletes the widget.
 */
int menu_add_widget(struct menu *menu,struct widget *widget);
int menu_remove_widget(struct menu *menu,struct widget *widget);
int menu_delete_widget_at(struct menu *menu,int p);
int menu_delete_widget_id(struct menu *menu,int wid);

struct widget *menu_widget_by_id(const struct menu *menu,int id);

/* Widget: Component of a menu.
 *************************************************************************/
 
struct widget {
  const struct widget_type *type;
  struct menu *menu; // WEAK
  int x,y,w,h; // Outer bounds in framebuffer pixels.
  int accept_focus;
  int imageid; // Nonzero if widget is rendered from one image resource, so menu can sort to avoid texture changes.
  int id; // For owner's use, zero by default.
  void (*cb)(struct widget *widget); // Override's menu's (cb_activate) if set.
  void *userdata;
};

struct widget_type {
  const char *name;
  int objlen;
  void (*del)(struct widget *widget);
  int (*init)(struct widget *widget);
  void (*focus)(struct widget *widget,int focus);
  void (*render)(struct widget *widget);
};

void widget_del(struct widget *widget);
struct widget *widget_new(const struct widget_type *type);

struct widget *widget_spawn(struct menu *menu,const struct widget_type *type);

static void widget_set_bounds(struct widget *widget,int x,int y,int w,int h) {
  if (!widget) return;
  widget->x=x;
  widget->y=y;
  widget->w=w;
  widget->h=h;
}

/* Widget types.
 ***************************************************************/
 
extern const struct widget_type widget_type_blotter;
extern const struct widget_type widget_type_tile;
extern const struct widget_type widget_type_decal;
extern const struct widget_type widget_type_sale;

int widget_blotter_setup(struct widget *widget,uint32_t rgba);
int widget_tile_setup(struct widget *widget,int imageid,uint8_t tileid,uint8_t xform);
int widget_decal_setup_image(struct widget *widget,int imageid,int x,int y,int w,int h); // (0,0,0,0) to use the entire image.
int widget_decal_setup_text(struct widget *widget,const char *src,int srcc,int wlimit,uint32_t rgba); // (wlimit) zero for single-line
int widget_decal_setup_string(struct widget *widget,int rid,int ix,int wlimit,uint32_t rgba); // ''
void widget_decal_align(struct widget *widget,uint16_t align); // Centered (0) by default. Provide dpad button ids to change.
void widget_decal_blink(struct widget *widget,int repc); // For highlighting. <0 to blink forever, 0 to stop blinking, or >0 blink so many times

struct widget *widget_blotter_spawn(struct menu *menu,int x,int y,int w,int h,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_tile_spawn(struct menu *menu,int x,int y,int imageid,uint8_t tileid,uint8_t xform,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_decal_spawn_image(struct menu *menu,int x,int y,uint16_t align,int imageid,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_decal_spawn_partial(struct menu *menu,int x,int y,uint16_t align,int imageid,int srcx,int srcy,int srcw,int srch,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_decal_spawn_text(struct menu *menu,int x,int y,uint16_t align,const char *src,int srcc,int wlimit,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_decal_spawn_string(struct menu *menu,int x,int y,uint16_t align,int rid,int ix,int wlimit,uint32_t rgba,void (*cb)(struct widget *widget),void *userdata);
struct widget *widget_sale_spawn(struct menu *menu,int x,int y,const struct item *item,int price); // (price<0) if player is buying, >0 if she's selling

#endif
