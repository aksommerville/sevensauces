/* layer_type_fishing.c
 * Minigame that triggers after you hook a fish, where it falls from the sky and you have to catch it.
 */
 
#include "game/sauces.h"
#include "layer.h"

#define DOT_SPEED (NS_sys_tilesize*6.0)

struct layer_fishing {
  struct layer hdr;
  double animclock;
  int instate; // -1,0,1
  double dotx,doty; // pixels, center
  uint8_t dotxform;
  double dotanimclock;
  int dotframe;
  double fishx,fishy;
  double fishdx;
  double fishybase,fishyrange;
  uint8_t fishxform;
  double duration;
  double termtime;
  double t;
  int outcome; // -1,0,1=lost,playing,won
  int ready; // Goes nonzero at the top of the fish's arc.
  void (*cb)(int outcome,void *userdata);
  void *userdata;
};

#define LAYER ((struct layer_fishing*)layer)

/* Cleanup.
 */
 
static void _fishing_del(struct layer *layer) {
}

/* Init.
 */
 
static int _fishing_init(struct layer *layer) {
  layer->opaque=1;
  
  // Dot always starts in the middle.
  LAYER->dotx=FBW>>1;
  LAYER->doty=FBH-NS_sys_tilesize*3.0;
  
  /* Fish starts at a random place on one half of the screen, and jumps to a random place on the other half.
   * Its entire routine is decided here.
   */
  LAYER->duration=3.0; // TODO Should this be random?
  LAYER->termtime=LAYER->duration+1.0;
  LAYER->fishy=FBH-NS_sys_tilesize-(NS_sys_tilesize>>1);
  LAYER->fishybase=LAYER->fishy;
  LAYER->fishyrange=FBH-NS_sys_tilesize*4;
  LAYER->fishx=rand()%(FBW>>1);
  double targetx=rand()%(FBW>>1);
  if (rand()&1) {
    LAYER->fishx+=FBW>>1;
    LAYER->fishxform=EGG_XFORM_XREV;
  } else {
    targetx+=FBW>>1;
  }
  LAYER->fishdx=(targetx-LAYER->fishx)/LAYER->duration;
   
  return 0;
}

/* Input.
 */
 
static void _fishing_input(struct layer *layer,int input,int pvinput) {
  if (BTNDOWN(WEST)) { layer_pop(layer); return; }//XXX TEMP B to exit
  if (LAYER->outcome) return;
  if (BTNDOWN(LEFT)) {
    LAYER->instate=-1;
    LAYER->dotxform=0;
    LAYER->dotanimclock=0.0;
    LAYER->dotframe=0;
  } else if (BTNUP(LEFT)&&(LAYER->instate<0)) {
    LAYER->instate=0;
  }
  if (BTNDOWN(RIGHT)) {
    LAYER->instate=1;
    LAYER->dotxform=EGG_XFORM_XREV;
    LAYER->dotanimclock=0.0;
    LAYER->dotframe=0;
  } else if (BTNUP(RIGHT)&&(LAYER->instate>0)) {
    LAYER->instate=0;
  }
}

/* Update.
 */
 
static void _fishing_update(struct layer *layer,double elapsed) {
  if ((LAYER->t+=elapsed)>=LAYER->termtime) {
    if (LAYER->cb) LAYER->cb(LAYER->outcome,LAYER->userdata);
    layer_pop(layer);
    return;
  }
  if ((LAYER->t>=LAYER->duration)&&!LAYER->outcome) {
    LAYER->outcome=-1;
    LAYER->instate=0;
  }
  
  /* Fish moves at a fixed rate horizontally, and we calculate y from scratch each frame.
   * This keeps running even if an outcome is established: If positive, we won't be rendered, whatever.
   */
  LAYER->fishx+=LAYER->fishdx*elapsed;
  double t=(LAYER->t*2.0/LAYER->duration)-1.0;
  if ((t>0.0)&&!LAYER->ready) {
    LAYER->ready=1;
    if (LAYER->fishdx>0.0) {
      LAYER->fishxform=EGG_XFORM_SWAP|EGG_XFORM_YREV;
    } else {
      LAYER->fishxform=EGG_XFORM_SWAP;
    }
  }
  t*=t;
  t=1.0-t;
  LAYER->fishy=LAYER->fishybase-t*LAYER->fishyrange;
  
  if ((LAYER->animclock-=elapsed)<0.0) {
    LAYER->animclock+=1.000;
  }
  if ((LAYER->dotanimclock-=elapsed)<=0.0) { // Dot's animation clock keeps running, whether she's animating or not.
    LAYER->dotanimclock+=0.200;
    LAYER->dotframe^=1;
  }
  if (LAYER->instate) {
    LAYER->dotx+=LAYER->instate*DOT_SPEED*elapsed;
    if (LAYER->dotx<NS_sys_tilesize) LAYER->dotx=NS_sys_tilesize;
    else if (LAYER->dotx>FBW-NS_sys_tilesize) LAYER->dotx=FBW-NS_sys_tilesize;
  }
  
  if (!LAYER->outcome&&LAYER->ready) {
    double dx=LAYER->fishx-LAYER->dotx;
    double dy=LAYER->fishy-LAYER->doty;
    double d2=dx*dx+dy*dy;
    double radius2=NS_sys_tilesize*NS_sys_tilesize;
    if (d2<radius2) {
      LAYER->outcome=1;
      LAYER->instate=0;
    }
  }
}

/* Render.
 */
 
static void _fishing_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x73bcedff);
  int texid=texcache_get_image(&g.texcache,RID_image_fishing);
  
  /* Rows 3 and 4 of the tilesheet are the bottom two rows of the output.
   * It's a single-tile pattern, but we went ahead and duplicated it 16 times at the source.
   * Animate the rows independently, back and forth.
   */
  int animphase=LAYER->animclock*16;
  if (animphase>=8) animphase=15-animphase;
  graf_draw_decal(&g.graf,texid,-animphase,FBH-NS_sys_tilesize*2,0,NS_sys_tilesize*3,NS_sys_tilesize*16,NS_sys_tilesize,0);
  graf_draw_decal(&g.graf,texid,-animphase+NS_sys_tilesize*16,FBH-NS_sys_tilesize*2,0,NS_sys_tilesize*3,NS_sys_tilesize*16,NS_sys_tilesize,0);
  graf_draw_decal(&g.graf,texid,animphase-NS_sys_tilesize,FBH-NS_sys_tilesize,0,NS_sys_tilesize*4,NS_sys_tilesize*16,NS_sys_tilesize,0);
  graf_draw_decal(&g.graf,texid,animphase-NS_sys_tilesize+NS_sys_tilesize*16,FBH-NS_sys_tilesize,0,NS_sys_tilesize*4,NS_sys_tilesize*16,NS_sys_tilesize,0);
  
  /* Dot is 4 tiles. Draw it as a decal.
   * 4 frames: idle, walk, victory, failure. All oriented left, except victory doesn't matter.
   */
  int16_t dstx=(int16_t)LAYER->dotx-NS_sys_tilesize;
  int16_t dsty=(int16_t)LAYER->doty-NS_sys_tilesize;
  int dotframe=0;
  if (LAYER->outcome<0) dotframe=3;
  else if (LAYER->outcome>0) dotframe=2;
  else if (LAYER->instate) dotframe=LAYER->dotframe;
  graf_draw_decal(&g.graf,texid,dstx,dsty,NS_sys_tilesize*2*dotframe,NS_sys_tilesize,NS_sys_tilesize*2,NS_sys_tilesize*2,LAYER->dotxform);
  
  /* Fish is one tile.
   * Don't draw him if the outcome is positive -- he's drawn into Dot's victory frame.
   */
  if (LAYER->outcome<1) {
    graf_draw_tile(&g.graf,texid,(int16_t)LAYER->fishx,(int16_t)LAYER->fishy,0x00,LAYER->fishxform);
  }
}

/* Type definition.
 */
 
const struct layer_type layer_type_fishing={
  .name="fishing",
  .objlen=sizeof(struct layer_fishing),
  .del=_fishing_del,
  .init=_fishing_init,
  .input=_fishing_input,
  .update=_fishing_update,
  .render=_fishing_render,
};

/* Setup.
 */
 
int layer_fishing_setup(struct layer *layer,void (*cb)(int outcome,void *userdata),void *userdata) {
  if (!layer||(layer->type!=&layer_type_fishing)) return -1;
  LAYER->cb=cb;
  LAYER->userdata=userdata;
  return 0;
}
