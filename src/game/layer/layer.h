/* layer.h
 * At the top level of the game, everything is organized into a layer stack.
 * Exactly one layer at a time receives input and updates, and one or more render.
 * There must be an opaque layer and a focus layer at all times.
 */
 
#ifndef LAYER_H
#define LAYER_H

struct layer;
struct layer_type;

/* Generic layer and type.
 ****************************************************************/

struct layer {
  const struct layer_type *type;
  int defunct; // Scheduled for removal. No longer participates.
  int opaque;
};

struct layer_type {
  const char *name;
  int objlen;
  void (*del)(struct layer *layer);
  int (*init)(struct layer *layer);
  
  /* Implementing either (input) marks your layers as focusable.
   * If you don't implement, some layer below you will continue to update while you're visible.
   */
  void (*input)(struct layer *layer,int input,int pvinput);
  void (*update)(struct layer *layer,double elapsed);
  
  void (*render)(struct layer *layer);
};

/* These should only be used by the layer stack.
 * See below for friendlier construction and deletion.
 */
void layer_del(struct layer *layer);
struct layer *layer_new(const struct layer_type *type);

static inline void layer_input(struct layer *layer,int input,int pvinput) {
  if (layer->type->input) layer->type->input(layer,input,pvinput);
}
static inline void layer_update(struct layer *layer,double elapsed) {
  if (layer->type->update) layer->type->update(layer,elapsed);
}
static inline void layer_render(struct layer *layer) {
  if (layer->type->render) layer->type->render(layer);
}

/* Global stack.
 *******************************************************************/
 
struct layer *layer_stack_get_focus();
int layer_stack_get_first_render(); // => Position in (g.layerv) of the topmost opaque layer.
int layer_is_resident(const struct layer *layer);
void layer_stack_reap(); // Drop defunct layers. Main should do this at the end of each update.
void layer_stack_filter(int (*filter)(struct layer *layer));

/* Popping a layer only schedules it for deletion at the end of the update cycle.
 * Pushing hands off ownership to the global stack.
 * Layers are expected to pop themselves eventually.
 * Pushing a layer is rare, code outside the layer stack shouldn't need it at all.
 */
void layer_pop(struct layer *layer);
int layer_push(struct layer *layer);

/* Create a push a new layer of the given type, and return a WEAK reference.
 * A given type might expect other setup params and provide its own ctor; see below.
 */
struct layer *layer_spawn(const struct layer_type *type);

/* Layer types.
 ****************************************************************/
 
extern const struct layer_type layer_type_hello;
extern const struct layer_type layer_type_settings;
extern const struct layer_type layer_type_credits;
extern const struct layer_type layer_type_beginday;
extern const struct layer_type layer_type_world;
extern const struct layer_type layer_type_kitchen;
extern const struct layer_type layer_type_pause;
extern const struct layer_type layer_type_toast;

/* "global" is the regular pause menu. It can change the session's chosen slot, and can abort the session.
 * "query" takes an optional prompt and reports the chosen item to your callback, or <0 if cancelled.
 */
int layer_pause_setup_global(struct layer *layer);
int layer_pause_setup_query(
  struct layer *layer,
  const char *prompt,int promptc,
  const char *cancel,int cancelc,
  void (*cb)(int invp,void *userdata),void *userdata
);

// Toasts can be configured either in framebuffer pixels or world meters. If world, we react to future scrolling.
int layer_toast_setup_fb(struct layer *layer,int x,int y,const char *msg,int msgc,uint32_t rgba);
int layer_toast_setup_world(struct layer *layer,double x,double y,const char *msg,int msgc,uint32_t rgba);

/* Extra helpers for layer implementations.
 *****************************************************************/
 
#define BTNDOWN(tag) ((input&EGG_BTN_##tag)&&!(pvinput&EGG_BTN_##tag))
#define BTNUP(tag) (!(input&EGG_BTN_##tag)&&(pvinput&EGG_BTN_##tag))

#endif
