#include "sauces.h"
#include "layer/layer.h"
#include "opt/rom/rom.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Headers say framebuffer %dx%d but we got %dx%d.\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  
  if (!(g.font6=font_new())) return -1;
  if (font_add_image_resource(g.font6,0x0020,RID_image_font6_0020)<0) return -1;
  
  if (!(g.font7=font_new())) return -1;
  if (font_add_image_resource(g.font7,0x0020,RID_image_font7_0020)<0) return -1;
  //TODO We'll also need Latin-1 and Cyrillic for both fonts. And whatever else we can translate to... Japanese? Telugu?
  
  srand_auto();
  
  if (!layer_spawn(&layer_type_hello)) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {
  
  // Process any changes to input state.
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) { // AUX3 to quit, always.
      egg_terminate(0);
      return;
    }
    struct layer *focus=layer_stack_get_focus();
    if (!focus) { // There must be a focus layer at all times.
      fprintf(stderr,"No focus layer.\n");
      egg_terminate(1);
      return;
    }
    layer_input(focus,input,g.pvinput);
    g.pvinput=input;
  }
  
  // Update all layers down the first that implements (input). Not just the focus layer!
  int i=g.layerc;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (layer->defunct) {
      continue;
    }
    if (layer->type->update) layer->type->update(layer,elapsed);
    if (layer->type->input) break;
  }
  
  layer_stack_reap();
}

void egg_client_render() {
  graf_reset(&g.graf);
  int layerp=layer_stack_get_first_render();
  if (layerp<0) { // There must be an opaque layer at all times.
    fprintf(stderr,"No opaque layer.\n");
    egg_terminate(1);
    return;
  }
  for (;layerp<g.layerc;layerp++) {
    if (g.layerv[layerp]->defunct) continue; // Weird for a defunct layer to still exist at render time, but ok.
    layer_render(g.layerv[layerp]);
  }
  graf_flush(&g.graf);
}

/* Get resource. TODO We probably ought to index these, at least the high-traffic types.
 */
 
int sauces_res_get(void *dstpp,int tid,int rid) {
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return 0;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid<tid) continue;
    if (res->tid>tid) break;
    if (res->rid<rid) continue;
    if (res->rid>rid) break;
    *(const void**)dstpp=res->v;
    return res->c;
  }
  return 0;
}

/* Format string, replacing '%' with an item name.
 */

int sauces_format_item_string(char *dst,int dsta,int fmt_rid,int fmt_ix,uint8_t itemid) {
  const char *name=0,*fmt=0;
  int namec,fmtc;
  if ((namec=strings_get(&name,RID_strings_item_name,itemid))<1) return 0;
  if ((fmtc=strings_get(&fmt,fmt_rid,fmt_ix))<1) return 0;
  int dstc=0;
  for (;fmtc-->0;fmt++) {
    if (*fmt=='%') {
      if (dstc>dsta-namec) return 0;
      memcpy(dst+dstc,name,namec);
      dstc+=namec;
    } else {
      if (dstc>=dsta) return 0;
      dst[dstc++]=*fmt;
    }
  }
  return dstc;
}
