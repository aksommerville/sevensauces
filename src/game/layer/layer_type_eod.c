/* layer_type_eod.c
 * Shows up immediately after night (kitchen), shows user a summary of the meal's results.
 */
 
/*
Initially show:
 - "Daily Summary: DAY"
 - kcbeforev, all
 - "Happy:", "Sad:", "Dead:", all zero.
 - "Tomorrow's Customers:" but no content.
Animate outcome icons above each in kcbeforev, update counters and Tomorrow icons as we go.
Then show each bonus message, blinking, with the new Tomorrow icon also blinking.
*/
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/kitchen/kitchen.h"
#include "layer.h"

#define EOD_TILE_LIMIT 72 /* 23 customers + 23 reactions + 26 tomorrow customers */
#define EOD_DECAL_LIMIT 11 /* headline, happy, sad, dead, c happy, c sad, c dead, bonus 1, bonus 2, bonus 3, tomorrow */

#define EOD_INITIAL_DELAY 1.000
#define EOD_REACT_INTERVAL 0.250
#define EOD_BONUS_INTERVAL 0.500
#define EOD_FAST_INTERVAL 0.033

#define EOD_VISIBILITY_HIDE 0
#define EOD_VISIBILITY_SHOW 1

struct layer_eod {
  struct layer hdr;
  struct stew stew;
  
  double clock; // Counts down to next event.
  int reactc; // First phase. Counts up to (kcbeforec), producing a reaction tile and if happy a tomorrow tile.
  int happyc,sadc,deadc; // Currently displayed values.
  int tomorrowc;
  int bonusp; // 0..(stew.kcaddc), how many bonuses exposed.
  int ready;
  
  int turbo; // Nonzero if skipping animation.
  
  /* Use egg's tile vertex, and do any other bookkeeping we need external to it.
   * This lets us cut out a lot of middleman.
   * The first (stew.kcbeforec) tiles are the top ribbon. After that it's mix and match.
   */
  struct egg_draw_tile tilev[EOD_TILE_LIMIT];
  int tilec;
  
  /* Decals are text; they're all their own texture.
   * So no need to use the egg-native vertex or anything.
   */
  struct eod_decal {
    int texid;
    int x,y,w,h;
    uint32_t rgba; // Only relevant if you replace text.
    int bonusp; // For locating later.
    int visibility;
  } decalv[EOD_DECAL_LIMIT];
  int decalc;
};

#define LAYER ((struct layer_eod*)layer)

/* Cleanup.
 */
 
static void _eod_del(struct layer *layer) {
  struct eod_decal *decal=LAYER->decalv;
  int i=LAYER->decalc;
  for (;i-->0;decal++) {
    egg_texture_del(decal->texid);
  }
}

/* Add one tile.
 */
 
static struct egg_draw_tile *eod_add_tile(struct layer *layer) {
  if (LAYER->tilec>=EOD_TILE_LIMIT) return 0;
  struct egg_draw_tile *tile=LAYER->tilev+LAYER->tilec++;
  return tile;
}

/* Add a tile for each "before" customer, in a single row near the top.
 */
 
static int eod_add_before_tiles(struct layer *layer) {
  const int xstride=14;
  int w=LAYER->stew.kcbeforec*xstride;
  int x=(FBW>>1)-(w>>1)+(xstride>>1);
  if (x<7) x=7;
  const struct kcustomer *kcustomer=LAYER->stew.kcbeforev;
  int i=LAYER->stew.kcbeforec;
  for (;i-->0;kcustomer++,x+=xstride) {
    struct egg_draw_tile *vtx=eod_add_tile(layer);
    if (!vtx) return -1;
    vtx->dstx=x;
    vtx->dsty=45;
    vtx->tileid=kcustomer->race*0x10;
    vtx->xform=0;
  }
  return 0;
}

/* Generate new decal with optional text from strings:ui.
 */
 
static struct eod_decal *eod_generate_decal(struct layer *layer,int strix,uint32_t rgba) {
  if (LAYER->decalc>=EOD_DECAL_LIMIT) return 0;
  struct eod_decal *decal=LAYER->decalv+LAYER->decalc++;
  decal->rgba=rgba;
  decal->bonusp=-1;
  decal->visibility=EOD_VISIBILITY_SHOW;
  if (strix>0) {
    if ((decal->texid=font_texres_oneline(g.font,RID_strings_ui,strix,FBW,rgba))<1) return 0;
    egg_texture_get_status(&decal->w,&decal->h,decal->texid);
  }
  return decal;
}

/* Replace existing decal by concatenating two string resources.
 */
 
static int eod_render_concat(struct eod_decal *decal,struct layer *layer,int pfxp,int sfxp) {
  const char *pfx,*sfx;
  int pfxc=strings_get(&pfx,RID_strings_ui,pfxp);
  int sfxc=strings_get(&sfx,RID_strings_ui,sfxp);
  char tmp[256];
  int tmpc=pfxc+sfxc;
  if (tmpc>sizeof(tmp)) return -1;
  memcpy(tmp,pfx,pfxc);
  memcpy(tmp+pfxc,sfx,sfxc);
  if (decal->texid>0) egg_texture_del(decal->texid);
  decal->texid=font_tex_oneline(g.font,tmp,tmpc,FBW,decal->rgba);
  egg_texture_get_status(&decal->w,&decal->h,decal->texid);
  return 0;
}

/* Replace existing decal with a decimal integer.
 */
 
static int eod_render_decimal(struct eod_decal *decal,struct layer *layer,int v) {
  if (v<0) v=0;
  char tmp[16];
  int tmpc=0,limit=1000000000;
  while (limit>=10) {
    if (v>=limit) tmp[tmpc++]='0'+(v/limit)%10;
    limit/=10;
  }
  tmp[tmpc++]='0'+v%10;
  int texid=font_tex_oneline(g.font,tmp,tmpc,FBW,decal->rgba);
  if (texid<1) return -1;
  if (decal->texid>0) egg_texture_del(decal->texid);
  decal->texid=texid;
  egg_texture_get_status(&decal->w,&decal->h,decal->texid);
  return 0;
}

/* Generate and place all the decals, whether initially visible or not.
 *  - "Daily Summary: DAY" (43+6..12)
 *  - "Happy:" (45)
 *  - "Sad:" (46)
 *  - "Dead:" (47)
 *  - Counts for happy, sad, dead. All zero initially.
 *  - Bonus 1, 2, and 3.
 *  - "Tomorrow's customers:" (44)
 * Only the happy/sad/dead counts will change.
 */
 
static int eod_generate_decals(struct layer *layer) {
  struct eod_decal *decal;
  
  /* The counts go in the first 3 slots, so they're easy to find when we need to change them.
   * Order isn't important for any other purpose.
   */
  if (!(decal=eod_generate_decal(layer,0,0xffffffff))) return -1;
  eod_render_decimal(decal,layer,0);
  decal->x=(FBW>>1)+5;
  decal->y=63;
  if (!(decal=eod_generate_decal(layer,0,0xffffffff))) return -1;
  eod_render_decimal(decal,layer,0);
  decal->x=(FBW>>1)+5;
  decal->y=73;
  if (!(decal=eod_generate_decal(layer,0,0xffffffff))) return -1;
  eod_render_decimal(decal,layer,0);
  decal->x=(FBW>>1)+5;
  decal->y=83;
  
  if (!(decal=eod_generate_decal(layer,0,0xffffffff))) return -1;
  eod_render_concat(decal,layer,43,6+g.session->day);
  decal->x=(FBW>>1)-(decal->w>>1);
  decal->y=9;
  
  if (!(decal=eod_generate_decal(layer,45,0xffffffff))) return -1;
  decal->x=(FBW>>1)-decal->w;
  decal->y=63;
  
  if (!(decal=eod_generate_decal(layer,46,0xffffffff))) return -1;
  decal->x=(FBW>>1)-decal->w;
  decal->y=73;
  
  if (!(decal=eod_generate_decal(layer,47,0xffffffff))) return -1;
  decal->x=(FBW>>1)-decal->w;
  decal->y=83;
  
  if (!(decal=eod_generate_decal(layer,LAYER->stew.addreasonv[0],0xffff00ff))) return -1;
  decal->x=(FBW>>1)-(decal->w>>1);
  decal->y=93;
  decal->visibility=EOD_VISIBILITY_HIDE;
  decal->bonusp=0;
  
  if (!(decal=eod_generate_decal(layer,LAYER->stew.addreasonv[1],0xffff00ff))) return -1;
  decal->x=(FBW>>1)-(decal->w>>1);
  decal->y=103;
  decal->visibility=EOD_VISIBILITY_HIDE;
  decal->bonusp=1;
  
  if (!(decal=eod_generate_decal(layer,LAYER->stew.addreasonv[2],0xffff00ff))) return -1;
  decal->x=(FBW>>1)-(decal->w>>1);
  decal->y=113;
  decal->visibility=EOD_VISIBILITY_HIDE;
  decal->bonusp=2;
  
  if (!(decal=eod_generate_decal(layer,44,0xffffffff))) return -1;
  decal->x=11;
  decal->y=138;
  
  return 0;
}

/* Init.
 */
 
static int _eod_init(struct layer *layer) {
  if (!g.session||!g.kitchen) return -1;
  egg_play_song(0,0,1);
  kitchen_cook(&LAYER->stew,g.kitchen);
  kitchen_commit_stew(&LAYER->stew);
  eod_add_before_tiles(layer);
  eod_generate_decals(layer);
  LAYER->clock=EOD_INITIAL_DELAY;
  return 0;
}

/* Input.
 */
 
static void _eod_input(struct layer *layer,int input,int pvinput) {
  if (LAYER->ready) {
    if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
      sauces_end_night();
    }
  } else {
    if (input&EGG_BTN_SOUTH) {
      LAYER->turbo=1;
      LAYER->clock=0.0;
    } else LAYER->turbo=0;
  }
}

/* Add one reaction and all knock-on effects.
 */
 
static void eod_react(struct layer *layer) {
  if (LAYER->reactc>=LAYER->stew.kcbeforec) return;
  struct kcustomer *kcustomer=LAYER->stew.kcbeforev+LAYER->reactc;
  struct egg_draw_tile *basetile=LAYER->tilev+LAYER->reactc;
  LAYER->reactc++;
  struct egg_draw_tile *tile;
  int *count=0;
  struct eod_decal *count_decal=0;
  if (tile=eod_add_tile(layer)) {
    tile->dstx=basetile->dstx;
    tile->dsty=basetile->dsty-NS_sys_tilesize;
    tile->xform=0;
    switch (kcustomer->outcome) {
      case KITCHEN_OUTCOME_DEAD: tile->tileid=0x03; count=&LAYER->deadc; count_decal=LAYER->decalv+2; break;
      case KITCHEN_OUTCOME_UNFED: tile->tileid=0x02; count=&LAYER->sadc; count_decal=LAYER->decalv+1; break;
      case KITCHEN_OUTCOME_SAD: tile->tileid=0x02; count=&LAYER->sadc; count_decal=LAYER->decalv+1; break;
      case KITCHEN_OUTCOME_HAPPY: tile->tileid=0x01; count=&LAYER->happyc; count_decal=LAYER->decalv+0; break;
    }
  }
  if (kcustomer->outcome==KITCHEN_OUTCOME_HAPPY) {
    if (tile=eod_add_tile(layer)) {
      int col=LAYER->tomorrowc%13;
      int row=LAYER->tomorrowc/13;
      tile->dstx=124+col*14;
      tile->dsty=142+row*16;
      tile->tileid=basetile->tileid;
      tile->xform=0;
      LAYER->tomorrowc++;
    }
  }
  if (count) {
    (*count)++;
    if (count_decal) {
      eod_render_decimal(count_decal,layer,*count);
    }
  }
}

/* Make the next bonus visible.
 * The decals already exist.
 */
 
static void eod_add_next_bonus(struct layer *layer) {
  if (LAYER->bonusp>=LAYER->stew.kcaddc) return;
  struct eod_decal *decal=0,*q=LAYER->decalv;
  int i=LAYER->decalc;
  for (;i-->0;q++) {
    if (q->bonusp==LAYER->bonusp) {
      decal=q;
      break;
    }
  }
  int race=LAYER->stew.kcaddv[LAYER->bonusp].race;
  LAYER->bonusp++;
  if (decal) {
    decal->visibility=EOD_VISIBILITY_SHOW;
  }
  struct egg_draw_tile *tile=eod_add_tile(layer);
  if (tile) {
    int col=LAYER->tomorrowc%13;
    int row=LAYER->tomorrowc/13;
    tile->dstx=124+col*14;
    tile->dsty=142+row*16;
    tile->tileid=0x10*race;
    tile->xform=0;
    LAYER->tomorrowc++;
  }
}

/* Take the next animation-driven action and bump the clock.
 */
 
static void eod_advance_animation(struct layer *layer) {
  double next_interval=0.0;
  
  if (LAYER->reactc<LAYER->stew.kcbeforec) {
    eod_react(layer);
    if (LAYER->reactc<LAYER->stew.kcbeforec) next_interval=EOD_REACT_INTERVAL;
    else next_interval=EOD_BONUS_INTERVAL;
    
  } else if (LAYER->bonusp<LAYER->stew.kcaddc) {
    eod_add_next_bonus(layer);
    next_interval=EOD_BONUS_INTERVAL;
    
  } else {
    LAYER->ready=1;
  }
  
  if (LAYER->turbo) next_interval=EOD_FAST_INTERVAL;
  LAYER->clock+=next_interval;
}

/* Update.
 */
 
static void _eod_update(struct layer *layer,double elapsed) {
  if (!LAYER->ready) {
    if ((LAYER->clock-=elapsed)<=0.0) {
      eod_advance_animation(layer);
    }
  }
}

/* Render.
 */
 
static void _eod_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x002040ff);
  
  struct eod_decal *decal=LAYER->decalv;
  int i=LAYER->decalc;
  for (;i-->0;decal++) {
    if (decal->visibility==EOD_VISIBILITY_HIDE) continue;
    graf_draw_decal(&g.graf,decal->texid,decal->x,decal->y,0,0,decal->w,decal->h,0);
  }
  graf_flush(&g.graf);
  
  egg_draw_tile(1,texcache_get_image(&g.texcache,RID_image_eod_tiles),LAYER->tilev,LAYER->tilec);
}

/* Type definition.
 */
 
const struct layer_type layer_type_eod={
  .name="eod",
  .objlen=sizeof(struct layer_eod),
  .del=_eod_del,
  .init=_eod_init,
  .input=_eod_input,
  .update=_eod_update,
  .render=_eod_render,
};
