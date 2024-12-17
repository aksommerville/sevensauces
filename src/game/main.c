#include "sauces.h"
#include "layer/layer.h"

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
  
  srand_auto();
  
  if (!layer_spawn(&layer_type_hello)) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {

  struct layer *focus=layer_stack_get_focus();
  if (!focus) { // There must be a focus layer at all times.
    fprintf(stderr,"No focus layer.\n");
    egg_terminate(1);
    return;
  }
  
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) { // AUX3 to quit, always.
      egg_terminate(0);
      return;
    }
    layer_input(focus,input,g.pvinput);
    g.pvinput=input;
    if (!(focus=layer_stack_get_focus())) return; // Receiving input may change the layer stack.
  }
  
  layer_update(focus,elapsed);
  
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
