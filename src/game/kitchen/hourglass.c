/* hourglass.c
 * For displaying the time remaining in night, we could use decimal digits or a sliding bar,
 * but where's the fun in that?
 */

#include "game/sauces.h"
#include "kitchen.h"

#define HGW 24
#define HGH 32
#define HG_SRCX 138 /* The overlay shell, in image:kitchen_bits */
#define HG_SRCY 1
#define HG_SRCX_WARNING 188
#define HG_SRCX_KAPUT 213
#define HG_CTLX 163 /* Control image that shows us the sand color and available flow space. */
#define HG_CTLY 1

#define HG_WALLCOLOR 0x00010000 /* Transparent but nonzero. */

#define HG_ENDTIME 0.950 /* The last particle leaves the top chamber a little before time is actually expired. */

struct hourglass {
  uint32_t *bits;
  int texid;
  uint32_t sandcolor;
  uint32_t stablecolor; // One point off from sand. Indicates a sand particle that can't move further. */
  double clock; // 0..1
  int sandc;
  double interval; // How long between grains leaving the top chamber?
};

/* Delete.
 */
 
void hourglass_del(struct hourglass *hg) {
  if (!hg) return;
  if (hg->bits) free(hg->bits);
  if (hg->texid>0) egg_texture_del(hg->texid);
  free(hg);
}

/* Init, fallible.
 */
 
static int hg_init(struct hourglass *hg) {
  
  /* Allocate our bits, and make a texture of it.
   */
  if (!(hg->bits=calloc(HGW<<2,HGH))) return -1;
  if ((hg->texid=egg_texture_new())<1) return -1;
  if (egg_texture_load_raw(hg->texid,HGW,HGH,HGW<<2,hg->bits,HGW*HGH*4)<0) return -1;
  
  /* Draw the control image into our texture, then copy out its pixels.
   */
  int texid=texcache_get_image(&g.texcache,RID_image_kitchen_bits);
  struct egg_draw_decal vtx={
    .dstx=0,.dsty=0,.srcx=HG_CTLX,.srcy=HG_CTLY,.w=HGW,.h=HGH,
  };
  egg_draw_decal(hg->texid,texid,&vtx,1);
  egg_texture_get_pixels(hg->bits,HGW*HGH*4,hg->texid);
  
  /* Every pixel of the control image is either:
   *  - Straight zeroes: WALL. We will never touch this pixel.
   *  - Straight ones: FREE. Set to zero, and it becomes available for sand to flow into.
   *  - Anything else: SAND. Defines both the color and the initial position of sand particles.
   */
  hg->sandcolor=0xffffffff; // Just in case something goes wrong.
  hg->stablecolor=0xfffeffff;
  uint32_t *p=hg->bits;
  int i=HGW*HGH;
  for (;i-->0;p++) {
    if (*p==0x00000000) *p=HG_WALLCOLOR;
    else if (*p==0xffffffff) *p=0;
    else if (hg->sandc) {
      hg->sandc++;
      *p=hg->sandcolor;
    } else {
      hg->sandc=1;
      hg->sandcolor=*p;
      hg->stablecolor=hg->sandcolor^0x00010000;
    }
  }
  if (!hg->sandc) {
    fprintf(stderr,"%s:%d:WARNING: Hourglass control image has no sand.\n",__FILE__,__LINE__);
    hg->sandc=1;
    hg->bits[((HGH>>1)-1)*HGW+(HGW>>1)]=hg->sandcolor;
  }
  hg->interval=HG_ENDTIME/(hg->sandc+1.0);
  
  return 0;
}

/* New.
 */
 
struct hourglass *hourglass_new() {
  struct hourglass *hg=calloc(1,sizeof(struct hourglass));
  if (!hg) return 0;
  if (hg_init(hg)<0) {
    hourglass_del(hg);
    return 0;
  }
  return hg;
}

/* Move one grain of sand from top to bottom.
 * Returns true if we change anything.
 */
 
static int hg_place(struct hourglass *hg) {
  uint32_t *p=hg->bits+(HGH>>1)*HGW;
  int i=(HGW*HGH)>>1;
  for (;i-->0;p++) {
    if (!*p) {
      *p=hg->sandcolor;
      return 1;
    }
  }
  return 1;
}
 
static int hg_xfer(struct hourglass *hg) {
  int y=HGH>>1;
  uint32_t *p=hg->bits+y*HGW;
  int i=(HGW*HGH)>>1;
  while (i-->0) {
    p--;
    if (*p==hg->sandcolor) {
      if (!hg_place(hg)) return 0;
      *p=0;
      return 1;
    }
  }
  return 0;
}

/* Advance time to (t) and update (bits).
 */
 
static void hg_advance(struct hourglass *hg,double t) {

  /* At fixed intervals, move one grain of sand from the bottom of the top chamber
   * to the top of the bottom chamber.
   */
  double elapsed=t-hg->clock;
  while (elapsed>=hg->interval) {
    hg->clock+=hg->interval;
    elapsed-=hg->interval;
    if (!hg_xfer(hg)) break;
  }
  
  /* Do one pass along the image, bottom to top, moving one grain at a time.
   * This stage isn't time-regulated.
   * Though effectively, it's regulated by the render interval, which should be pretty tight.
   */
  uint32_t *p=hg->bits+HGW*HGH;
  int y=HGH;
  while (y-->1) { // Don't visit the top row.
    if (y==HGH>>1) { // The middle row can only be entered by xfer. Skip it.
      p-=HGW;
      continue;
    }
    int x=HGW;
    while (x-->0) {
      p--;
      if (*p) continue; // Something here, carry on.
      if (p[-HGW]==hg->sandcolor) {
        p[-HGW]=0;
        *p=hg->sandcolor;
      } else if (x&&p[-1]&&(p[-HGW-1]==hg->sandcolor)) {
        p[-HGW-1]=0;
        *p=hg->sandcolor;
      } else if ((x<HGW-1)&&p[1]&&(p[1-HGW]==hg->sandcolor)) {
        p[1-HGW]=0;
        *p=hg->sandcolor;
      // Could stop here and get a perfect 1/1 angle of repose. But I think 1/2 will look more natural:
      } else if ((x>=2)&&p[-1]&&p[-2]&&(p[-HGW-2]==hg->sandcolor)) {
        p[-HGW-2]=0;
        *p=hg->sandcolor;
      } else if ((x<HGW-2)&&p[1]&&p[1]&&(p[2-HGW]==hg->sandcolor)) {
        p[2-HGW]=0;
        *p=hg->sandcolor;
      }
    }
  }
}

/* Update/render.
 */
 
void hourglass_render(int dstx,int dsty,struct hourglass *hg,double t,double remaining) {
  if (!hg) return;
  if ((hg->clock<t)&&(hg->clock<1.0)) {
    hg_advance(hg,t);
    egg_texture_load_raw(hg->texid,HGW,HGH,HGW<<2,hg->bits,HGW*HGH*4);
  }
  graf_draw_decal(&g.graf,hg->texid,dstx,dsty,0,0,HGW,HGH,0);
  int srcx=HG_SRCX;
  if (remaining<0.0) {
    srcx=HG_SRCX_KAPUT;
  } else if (remaining<5.0) {
    if (((int)(remaining*4.0))&1) srcx=HG_SRCX_WARNING;
  }
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_kitchen_bits),dstx,dsty,srcx,HG_SRCY,HGW,HGH,0);
}
