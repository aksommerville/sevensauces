#include "game/sauces.h"
#include "layer.h"

/* Delete.
 */
 
void layer_del(struct layer *layer) {
  if (!layer) return;
  if (layer->type->del) layer->type->del(layer);
  free(layer);
}

/* New.
 */
 
struct layer *layer_new(const struct layer_type *type) {
  if (!type) return 0;
  struct layer *layer=calloc(1,type->objlen);
  if (!layer) return 0;
  layer->type=type;
  if (type->init&&(type->init(layer)<0)) {
    layer_del(layer);
    return 0;
  }
  return layer;
}

/* Get layers from stack.
 */
 
struct layer *layer_stack_get_focus() {
  return g.layer_focus;
}

int layer_stack_get_first_render() {
  int p=g.layerc;
  while (p-->0) {
    struct layer *layer=g.layerv[p];
    if (layer->defunct) continue;
    if (layer->opaque) return p;
  }
  return -1;
}

int layer_is_resident(const struct layer *layer) {
  if (!layer) return 0;
  int i=g.layerc;
  while (i-->0) if (g.layerv[i]==layer) return 1;
  return 0;
}

static struct layer *layer_stack_find_focus() {
  int p=g.layerc;
  while (p-->0) {
    struct layer *layer=g.layerv[p];
    if (layer->defunct) continue;
    if (layer->type->input) return layer;
  }
  return 0;
}

static void layer_stack_reexamine_focus() {
  struct layer *nfocus=layer_stack_find_focus();
  if (nfocus==g.layer_focus) return;
  if (layer_is_resident(g.layer_focus)) {
    if (g.layer_focus->type->input) g.layer_focus->type->input(g.layer_focus,0,g.pvinput);
  }
  g.layer_focus=nfocus;
  // We don't have a sensible "you got focus" event, and maybe don't want one.
  // Sending the initial input state like this is inappropriate; layers would react to the event that spawned them.
  // Also, new layers will receive focus before any external setup, if that's a thing that happens.
  //if (g.pvinput&&nfocus&&nfocus->type->input) nfocus->type->input(nfocus,g.pvinput,0);
}

/* Reap defunct layers.
 */
 
void layer_stack_reap() {
  int i=g.layerc;
  struct layer **p=g.layerv+i-1;
  for (;i-->0;p--) {
    struct layer *layer=*p;
    if (layer->defunct) {
      g.layerc--;
      memmove(g.layerv+i,g.layerv+i+1,sizeof(void*)*(g.layerc-i));
      layer_del(layer);
    }
  }
}

/* Filter layers.
 */
 
void layer_stack_filter(int (*filter)(struct layer *layer)) {
  int i=g.layerc,changed=0;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (!filter(layer)) {
      layer->defunct=1;
      changed=1;
    }
  }
  if (changed) layer_stack_reexamine_focus();
}

/* Pop layer: Only mark it as defunct.
 */

void layer_pop(struct layer *layer) {
  if (!layer) return;
  layer->defunct=1;
  layer_stack_reexamine_focus();
}

/* Push layer.
 */
 
int layer_push(struct layer *layer) {
  if (!layer) return -1;
  
  // Would be disastrous if you double-push a layer, so fail if they try to.
  int i=g.layerc;
  while (i-->0) {
    if (g.layerv[i]==layer) return -1;
  }
  
  if (g.layerc>=g.layera) {
    int na=g.layera+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(g.layerv,sizeof(void*)*na);
    if (!nv) return -1;
    g.layerv=nv;
    g.layera=na;
  }
  
  g.layerv[g.layerc++]=layer;
  layer_stack_reexamine_focus();
  return 0;
}

/* Spawn layer.
 */

struct layer *layer_spawn(const struct layer_type *type) {
  struct layer *layer=layer_new(type);
  if (!layer) return 0;
  if (layer_push(layer)<0) {
    layer_del(layer);
    return 0;
  }
  return layer;
}
