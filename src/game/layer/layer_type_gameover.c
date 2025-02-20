#include "game/sauces.h"
#include "game/session/session.h"
#include "layer.h"

#define GAMEOVER_LABEL_LIMIT 4

struct layer_gameover {
  struct layer hdr;
  struct label {
    int texid;
    int x,y,w,h;
  } labelv[GAMEOVER_LABEL_LIMIT];
  int labelc;
};

#define LAYER ((struct layer_gameover*)layer)

/* Cleanup.
 */
 
static void _gameover_del(struct layer *layer) {
}

/* Prep labels.
 */
 
static struct label *gameover_label_new(struct layer *layer) {
  if (LAYER->labelc>=GAMEOVER_LABEL_LIMIT) return 0;
  struct label *label=LAYER->labelv+LAYER->labelc++;
  memset(label,0,sizeof(struct label));
  return label;
}

static int gameover_label_ii(struct label *label,int strix,int a,int b) {
  if (label->texid) egg_texture_del(label->texid);
  char tmp[256];
  struct strings_insertion insv[]={
    {'i',.i=a},
    {'i',.i=b},
  };
  int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_ui,strix,insv,2);
  label->texid=font_tex_oneline(g.font,tmp,tmpc,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  label->x=(FBW>>1)-(label->w>>1);
  return 0;
}

static int gameover_label_time(struct label *label,int strix,double f) {
  if (label->texid) egg_texture_del(label->texid);
  char repr[9];
  int ms=(int)(f*1000.0);
  int s=ms/1000; ms%=1000;
  int min=s/60; s%=60;
  if (min>99) { min=s=99; ms=999; }
  repr[0]='0'+min/10;
  repr[1]='0'+min%10;
  repr[2]=':';
  repr[3]='0'+s/10;
  repr[4]='0'+s%10;
  repr[5]='.';
  repr[6]='0'+ms/100;
  repr[7]='0'+(ms/10)%10;
  repr[8]='0'+ms%10;
  char tmp[256];
  struct strings_insertion ins={'s',.s={.v=repr,.c=sizeof(repr)}};
  int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_ui,strix,&ins,1);
  label->texid=font_tex_oneline(g.font,tmp,tmpc,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  label->x=(FBW>>1)-(label->w>>1);
  return 0;
}

/* Init.
 */
 
static int _gameover_init(struct layer *layer) {
  if (!g.session) return -1;
  struct label *label;
  
  //TODO We can do much better with data reporting.
  // Also, I'd like some amount of narrative here, and credits.
  
  if (!(label=gameover_label_new(layer))) return -1;
  gameover_label_ii(label,72,0,0); // "game over"
  
  if (!(label=gameover_label_new(layer))) return -1;
  int custc=g.session->customerc;
  int i=4; while (i-->0) custc+=g.session->custover[i];
  gameover_label_ii(label,73,custc,26);
  
  if (!(label=gameover_label_new(layer))) return -1;
  gameover_label_time(label,74,g.session->worldtime+g.session->kitchentime);
  
  if (!(label=gameover_label_new(layer))) return -1;
  gameover_label_ii(label,75,g.session->gold,0);
  
  int ystride=LAYER->labelv[0].h+4;
  int totalh=LAYER->labelc*ystride;
  int y=(FBH>>1)-(totalh>>1);
  for (label=LAYER->labelv,i=LAYER->labelc;i-->0;label++,y+=ystride) label->y=y;
  
  return 0;
}

/* Input.
 */
 
static void _gameover_input(struct layer *layer,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    layer->defunct=1;
    sauces_end_session();
  }
}

/* Update.
 */
 
static void _gameover_update(struct layer *layer,double elapsed) {
}

/* Render.
 */
 
static void _gameover_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x600810ff);
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) {
    graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
  }
}

/* Type definition.
 */
 
const struct layer_type layer_type_gameover={
  .name="gameover",
  .objlen=sizeof(struct layer_gameover),
  .del=_gameover_del,
  .init=_gameover_init,
  .input=_gameover_input,
  .update=_gameover_update,
  .render=_gameover_render,
};
