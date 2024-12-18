/* sprite_type_faun.c
 * All wild beasts associated with an item.
 * These can be picked up and stunned, and they'll run from the hero when she's near.
 * They move differently depending on itemid.
 * Arg: u8 itemid, u24 reserved
 */
 
#include "game/sauces.h"
#include "game/world/world.h"
#include "game/session/session.h"
#include "sprite.h"

struct sprite_faun {
  struct sprite hdr;
  double blackout; // Initial period where we will not allow capture.
  double capture_time; // Constant; how long must hero collide in order to catch us?
  double capture_clock; // Volatile, counts down while overlapped.
  double capture_d2; // Constant; Must stay within this distance to capture (squared, so we don't have to root it each time).
  double spook_d2; // Constant; Within this distance, we'll run off.
  double safe_d2; // Constant; >spook_d2, if we get this far away, we relax.
  int spooked;
  double burst_time_lo,burst_time_hi; // Constant; We move in little bursts of a duration in this range.
  double burst_time_spook; // Constant; when spooked, all bursts are this duration. Typically short.
  double burst_clock;
  double speed_lo,speed_hi,speed_spook; // Constant.
  double wait_time_lo,wait_time_hi,wait_time_spook; // Constant.
  double wait_clock;
  double dx,dy;
};

#define SPRITE ((struct sprite_faun*)sprite)

/* Random float between two endpoints.
 */
 
static double frandrange(double lo,double hi) {
  return lo+((rand()&0x7fff)*(hi-lo))/32767.0;
}

/* Init.
 */
 
static int _faun_init(struct sprite *sprite) {
  uint8_t itemid=sprite->arg>>24;
  SPRITE->blackout=2.0;
  SPRITE->capture_clock=0.0;
  SPRITE->spooked=0;
  SPRITE->burst_clock=0.0;
  SPRITE->wait_clock=0.0;
  SPRITE->dx=0.0;
  SPRITE->dy=0.0;
  
  // Set some defaults in case it's not implemented yet, but we'll override below per item.
  // These defaults are pretty good; the spook speed and wait time work out to match the hero's speed almost exactly.
  sprite->imageid=RID_image_item;
  sprite->tileid=itemid;
  SPRITE->capture_time=0.500;
  SPRITE->capture_d2=1.0; // Unsquared while we set up, for clarity.
  SPRITE->spook_d2=5.0; // ''
  SPRITE->safe_d2=8.0; // ''
  SPRITE->burst_time_lo=0.250;
  SPRITE->burst_time_hi=0.750;
  SPRITE->burst_time_spook=0.250;
  SPRITE->speed_lo=2.0;
  SPRITE->speed_hi=5.0;
  SPRITE->speed_spook=8.0; // Hero moves at 6 m/s; this should be faster.
  SPRITE->wait_time_lo=0.125;
  SPRITE->wait_time_hi=0.333;
  SPRITE->wait_time_spook=0.080;
  
  switch (itemid) {
    //TODO Parameters for all known beasts.
    default: fprintf(stderr,"%s:%d:%s: No handler for item 0x%02x\n",__FILE__,__LINE__,__func__,itemid); break;
  }
  
  SPRITE->capture_d2*=SPRITE->capture_d2;
  SPRITE->spook_d2*=SPRITE->spook_d2;
  SPRITE->safe_d2*=SPRITE->safe_d2;
  return 0;
}

/* Capture me. Deathrows the sprite.
 */
 
static void faun_cb_swap(int invp,void *userdata) {
  if ((invp<0)||(invp>=16)) return; // Cancelled, all good.
  struct sprite *sprite=userdata;
  uint8_t dropid=g.session->inventory[invp];
  uint8_t takeid=sprite->arg>>24;
  if (!dropid||!takeid) return; // Shouldn't have gotten this far, but please go no further.
  const struct item *item=itemv+dropid;
  
  // With NS_itemusage_free, we can cheat by swapping IDs and reinitializing.
  if (item->usage==NS_itemusage_free) {
    sprite->arg=(sprite->arg&0x00ffffff)|(dropid<<24);
    sprite->tileid=dropid;
    SPRITE->blackout=2.0;
    g.session->inventory[invp]=takeid;
    return;
  }
  
  // Everything else is effectively NS_itemusage_drop: Create an item sprite and delete me.
  struct sprite *itemspr=sprite_new(0,g.world->sprites,sprite->x,sprite->y,dropid<<24,RID_sprite_item);
  sprite_kill_soon(sprite);
  g.session->inventory[invp]=takeid;
}
 
static void faun_capture(struct sprite *sprite) {
  uint8_t itemid=sprite->arg>>24;
  int result=session_acquire_item(g.session,itemid,faun_cb_swap,sprite);
  if (result>0) {
    sprite_kill_soon(sprite);
  }
}

/* Begin my next burst.
 */
 
static void faun_choose_next_move(struct sprite *sprite,int panic) {
  
  // When spooked, we have a constant speed and duration, and we run exactly opposite the hero.
  if (SPRITE->spooked) {
    SPRITE->burst_clock=SPRITE->burst_time_spook;
    struct sprite *hero=sprites_get_hero(sprite->owner);
    if (hero) {
      double dx=sprite->x-hero->x;
      double dy=sprite->y-hero->y;
      const double near=0.333;
      if (panic||((dx>-near)&&(dx<near)&&(dy>-near)&&(dy<near))) {
        // When very close or panicked, pick a random direction.
        double t=frandrange(-M_PI,M_PI);
        SPRITE->dx=cos(t)*SPRITE->speed_spook;
        SPRITE->dy=sin(t)*SPRITE->speed_spook;
      } else {
        // Most of the time, move exactly opposite the hero.
        double dist=sqrt(dx*dx+dy*dy);
        double nx=dx/dist,ny=dy/dist;
        SPRITE->dx=nx*SPRITE->speed_spook;
        SPRITE->dy=ny*SPRITE->speed_spook;
      }
    }
    
  } else {
    SPRITE->burst_clock=frandrange(SPRITE->burst_time_lo,SPRITE->burst_time_hi);
    double speed=frandrange(SPRITE->speed_lo,SPRITE->speed_hi);
    double t=frandrange(-M_PI,M_PI);
    SPRITE->dx=cos(t)*speed;
    SPRITE->dy=sin(t)*speed;
  }
  // Natural orientation is left.
  if (SPRITE->dx<-0.100) sprite->xform=0;
  else if (SPRITE->dx>0.100) sprite->xform=EGG_XFORM_XREV;
}

/* Begin waiting.
 */
 
static void faun_chill(struct sprite *sprite) {
  if (SPRITE->spooked) {
    SPRITE->wait_clock=SPRITE->wait_time_spook;
  } else {
    SPRITE->wait_clock=frandrange(SPRITE->wait_time_lo,SPRITE->wait_time_hi);
  }
}

/* Hero is close: Get spooked.
 */
 
static void faun_spook(struct sprite *sprite) {
  SPRITE->spooked=1;
  faun_choose_next_move(sprite,0);
}

/* Take one step, during the burst.
 */
 
static void faun_step(struct sprite *sprite,double elapsed) {
  double x0=sprite->x,y0=sprite->y;
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  sprite_rectify_physics(sprite);
  if (SPRITE->spooked) {
    double dx=sprite->x-x0,dy=sprite->y-y0;
    double d=sqrt(dx*dx+dy*dy);
    double effv=d/elapsed;
    if (effv<2.0) {
      faun_choose_next_move(sprite,1);
    }
  }
}

/* Update.
 */
 
static void _faun_update(struct sprite *sprite,double elapsed) {

  /* We need the hero's distance every time, for spooking and capture tracking.
   */
  double hero_d2=999.0;
  struct sprite *hero=sprites_get_hero(sprite->owner);
  if (hero) {
    double dx=hero->x-sprite->x;
    double dy=hero->y-sprite->y;
    hero_d2=dx*dx+dy*dy;
  }
  
  /* Check capture.
   */
  if (SPRITE->blackout>0.0) {
    SPRITE->blackout-=elapsed;
  } else if (hero_d2<SPRITE->capture_d2) {
    if (SPRITE->capture_clock<=0.0) SPRITE->capture_clock=SPRITE->capture_time;
    if (((SPRITE->capture_clock)-=elapsed)<=0.0) {
      faun_capture(sprite);
      return;
    }
  } else {
    SPRITE->capture_clock=0.0;
  }
  
  /* Check spookage.
   */
  if (!SPRITE->spooked&&(hero_d2<SPRITE->spook_d2)) {
    faun_spook(sprite);
  }
  
  /* Wait, move, or if the burst clock expires, reassess.
   */
  if (SPRITE->wait_clock>0.0) {
    if ((SPRITE->wait_clock-=elapsed)<=0.0) {
      faun_choose_next_move(sprite,0);
    }
  } else if (SPRITE->burst_clock>0.0) {
    if ((SPRITE->burst_clock-=elapsed)<=0.0) {
      if (hero_d2>SPRITE->safe_d2) SPRITE->spooked=0;
      faun_chill(sprite);
    }
    faun_step(sprite,elapsed);
  } else {
    faun_choose_next_move(sprite,0);
  }
  
  //TODO animation
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_faun={
  .name="faun",
  .objlen=sizeof(struct sprite_faun),
  .init=_faun_init,
  .update=_faun_update,
};