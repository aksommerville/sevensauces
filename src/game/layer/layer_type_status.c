/* layer_type_status.c
 * Always paired with a 'world' layer, and directly above it.
 * Shows time remaining, selected item, etc.
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "game/session/session.h"
#include "layer.h"

struct layer_status {
  struct layer hdr;
  const char *dayname; // This layer only lasts one day, no sense looking it up each render.
  int daynamec;
  struct invsum invsum;
  uint8_t inv[INVENTORY_SIZE]; // For observing changes to invsum.
};

#define LAYER ((struct layer_status*)layer)

static void _status_del(struct layer *layer) {
}

static int _status_init(struct layer *layer) {
  if (!g.session||!g.world) return -1;
  layer->opaque=0;
  LAYER->daynamec=strings_get(&LAYER->dayname,RID_strings_ui,22+g.session->day%7);
  return 0;
}

static void status_draw_text(struct layer *layer,int16_t dstx,int16_t dsty,int texid,const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  dstx+=4;
  dsty+=4;
  for (;srcc-->0;src++,dstx+=8) {
    if (*src==0x20) continue;
    graf_draw_tile(&g.graf,texid,dstx,dsty,*src,0);
  }
}

static void status_draw_clock(struct layer *layer,int16_t dstx,int16_t dsty,int texid) {
  char msg[16];
  int msgc=0;
  if (msgc>sizeof(msg)-LAYER->daynamec) return;
  memcpy(msg+msgc,LAYER->dayname,LAYER->daynamec);
  msgc+=LAYER->daynamec;
  if (msgc>sizeof(msg)-5) return; // space, one-digit minute, colon, two-digit second
  int sec=(int)g.world->clock;
  if (sec<0) sec=0;
  int min=sec/60; sec%=60;
  if (min>9) { min=9; sec=99; }
  msg[msgc++]=' ';
  msg[msgc++]='0'+min;
  msg[msgc++]=':';
  msg[msgc++]='0'+sec/10;
  msg[msgc++]='0'+sec%10;
  status_draw_text(layer,dstx,dsty,texid,msg,msgc);
}

static void status_draw_tiny_int(struct layer *layer,int texid,int16_t dstx,int16_t dsty,uint8_t intro,int v) {
  if (v<0) v=0;
  dstx+=1;
  dsty+=2;
  graf_draw_tile(&g.graf,texid,dstx,dsty,intro,0); dstx+=6; // Indicators are wider than glyphs
  if (v>=100000) { graf_draw_tile(&g.graf,texid,dstx,dsty,(v/100000)%10,0); dstx+=4; }
  if (v>=10000 ) { graf_draw_tile(&g.graf,texid,dstx,dsty,(v/10000 )%10,0); dstx+=4; }
  if (v>=1000  ) { graf_draw_tile(&g.graf,texid,dstx,dsty,(v/1000  )%10,0); dstx+=4; }
  if (v>=100   ) { graf_draw_tile(&g.graf,texid,dstx,dsty,(v/100   )%10,0); dstx+=4; }
  if (v>=10    ) { graf_draw_tile(&g.graf,texid,dstx,dsty,(v/10    )%10,0); dstx+=4; }
  graf_draw_tile(&g.graf,texid,dstx,dsty,v%10,0);
}

static void status_draw_inventory_summary(struct layer *layer,int texid,int16_t dstx,int16_t dsty) {
  if (memcmp(LAYER->inv,g.session->inventory,INVENTORY_SIZE)) {
    session_summarize_inventory(&LAYER->invsum,g.session);
    memcpy(LAYER->inv,g.session->inventory,INVENTORY_SIZE);
  }
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0a,LAYER->invsum.invc); dsty+=6;
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0b,LAYER->invsum.vegc); dsty+=6;
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0c,LAYER->invsum.meatc); dsty+=6;
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0d,LAYER->invsum.candyc); dsty+=6;
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0e,g.session->customerc); dsty+=6;
  status_draw_tiny_int(layer,texid,dstx,dsty,0x0f,g.session->gold); dsty+=6;
}

static void _status_render(struct layer *layer) {

  // In the final seconds of diurnal phase, flash the screen as a warning.
  if (g.world->clock<10.0) {
    const double dutycycle=0.50;
    int peakalpha=0x80;
    double whole,fract;
    fract=modf(g.world->clock,&whole);
    if (fract>1.0-dutycycle) {
      int alpha=(int)(((fract-(1.0-dutycycle))*peakalpha)/dutycycle);
      if (alpha>0) {
        graf_set_alpha(&g.graf,alpha);
        graf_draw_rect(&g.graf,0,0,FBW,FBH,0xff800000|alpha);
        graf_set_alpha(&g.graf,0xff);
      }
    }
  }

  // First do as much as we can from uitiles, should be most of it.
  int texid=texcache_get_image(&g.texcache,RID_image_uitiles);
  status_draw_clock(layer,1,1,texid);
  graf_draw_decal(&g.graf,texid,1,9,0,8*8,8*3,8*3,0); // bg for armed item -- defer the item icon, it's from a different image
  status_draw_inventory_summary(layer,texid,4,34);
  
  // Then other images.
  texid=texcache_get_image(&g.texcache,RID_image_item);
  graf_draw_tile(&g.graf,texid,13,21,g.session->inventory[g.session->invp],0);
}

const struct layer_type layer_type_status={
  .name="status",
  .objlen=sizeof(struct layer_status),
  .del=_status_del,
  .init=_status_init,
  .render=_status_render,
};
