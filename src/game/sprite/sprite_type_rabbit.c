/* sprite_type_rabbit.c
 */
 
#include "game/sauces.h"
#include "game/session/session.h"
#include "game/session/map.h"
#include "game/world/world.h"
#include "sprite.h"

#define RABBIT_STAGE_WAIT 0
#define RABBIT_STAGE_HOP 1
#define RABBIT_STAGE_DAZE 2

#define RABBIT_WAIT_TIME_MIN 0.750
#define RABBIT_WAIT_TIME_MAX 1.500
#define RABBIT_PANIC_WAIT_TIME 0.250
#define RABBIT_HOP_TIME_MIN 0.500
#define RABBIT_HOP_TIME_MAX 0.750
#define RABBIT_SPEED_MIN 4.0
#define RABBIT_SPEED_MAX 6.0
#define RABBIT_PANIC_SPEED 12.0
#define RABBIT_PANIC_HOP_TIME 0.300
#define RABBIT_PANIC_DISTANCE 9.0 /* Pretty much can't see a rabbit without it panicking. Widest screen radius is 10. */
#define RABBIT_RECENTER_MARGIN 5.0 /* Within so many cells of the world edge, force hop direction inward. */
#define RABBIT_ARROW_DISTANCE 0.750
#define RABBIT_DAZE_TIME 1.000
#define RABBIT_CATCH_DISTANCE 0.750
#define RABBIT_CATCH_TIME 0.250 /* Must remain in range for multiple consecutive frames. */

struct sprite_rabbit {
  struct sprite hdr;
  int stage;
  double stageclock;
  int panic; // Nonzero when the hero is near.
  double lasthop; // radians, direction of last hop, initially random.
  double hopdx,hopdy;
  uint8_t tileid0;
  double catchclock;
};

#define SPRITE ((struct sprite_rabbit*)sprite)

// We only reassess panic at the start of the WAIT and HOP stages.
static void rabbit_check_panic(struct sprite *sprite) {
  SPRITE->panic=0;
  struct sprite *hero=sprites_get_hero(sprite->owner);
  if (!hero) return;
  double dx=sprite->x-hero->x;
  double dy=sprite->y-hero->y;
  double d2=dx*dx+dy*dy;
  if (d2>RABBIT_PANIC_DISTANCE*RABBIT_PANIC_DISTANCE) return;
  SPRITE->panic=1;
}

static void rabbit_begin_WAIT(struct sprite *sprite) {
  SPRITE->stage=RABBIT_STAGE_WAIT;
  rabbit_check_panic(sprite);
  if (SPRITE->panic) {
    SPRITE->stageclock=RABBIT_PANIC_WAIT_TIME;
  } else {
    SPRITE->stageclock=RABBIT_WAIT_TIME_MIN+((rand()&0x7fff)*(RABBIT_WAIT_TIME_MAX-RABBIT_WAIT_TIME_MIN))/32767.0;
  }
  sprite->tileid=SPRITE->tileid0;
}

static void rabbit_begin_HOP(struct sprite *sprite) {
  SPRITE->stage=RABBIT_STAGE_HOP;
  rabbit_check_panic(sprite);
  
  // In a panic, the new direction is always exactly opposite the hero, with fixed parameters.
  if (SPRITE->panic) {
    struct sprite *hero=sprites_get_hero(sprite->owner);
    if (!hero) {
      rabbit_begin_WAIT(sprite);
      return;
    }
    double dx=sprite->x-hero->x;
    double dy=sprite->y-hero->y;
    if ((dx>=-0.001)&&(dx<=0.001)&&(dy>=-0.001)&&(dy<=0.001)) dx=dy=0.001; // Stay a safe distance from zero.
    double dist=sqrt(dx*dx+dy*dy);
    dx/=dist;
    dy/=dist;
    SPRITE->hopdx=dx*RABBIT_PANIC_SPEED;
    SPRITE->hopdy=dy*RABBIT_PANIC_SPEED;
    SPRITE->lasthop=atan2(dy,dx);
    SPRITE->stageclock=RABBIT_PANIC_HOP_TIME;
    
  } else {
    // Normally, the new direction will stay close to the most recent, no more than a quarter-turn from it, with centered most likely.
    double variance=(rand()%200)/100.0-1.0;
    variance*=variance;
    variance*=M_PI*0.5;
    double t=SPRITE->lasthop+variance;
    if (t>M_PI) t-=M_PI*2.0;
    else if (t<-M_PI) t+=M_PI*2.0;
    double speed=RABBIT_SPEED_MIN+((rand()&0x7fff)*(RABBIT_SPEED_MAX-RABBIT_SPEED_MIN))/32767.0;
    SPRITE->hopdx=cos(t)*speed;
    SPRITE->hopdy=sin(t)*speed;
    SPRITE->lasthop=t;
    SPRITE->stageclock=RABBIT_HOP_TIME_MIN+((rand()&0x7fff)*(RABBIT_HOP_TIME_MAX-RABBIT_HOP_TIME_MIN))/32767.0;
  }
  
  // Never hop out to the world edge. Reverse direction near the margins, even if we jump right into the hero.
  if ((sprite->x<RABBIT_RECENTER_MARGIN)&&(SPRITE->hopdx<0.0)) SPRITE->hopdx*=-1.0;
  else if ((sprite->x>g.world->map->w-RABBIT_RECENTER_MARGIN)&&(SPRITE->hopdx>0.0)) SPRITE->hopdx*=-1.0;
  if ((sprite->y<RABBIT_RECENTER_MARGIN)&&(SPRITE->hopdy<0.0)) SPRITE->hopdy*=-1.0;
  else if ((sprite->y>g.world->map->h-RABBIT_RECENTER_MARGIN)&&(SPRITE->hopdy>0.0)) SPRITE->hopdy*=-1.0;
  //TODO We badly need correction similar to the world-edge bit above, but for walls within the map. As is, it's stupid easy to trap rabbits in a corner.
  
  if (SPRITE->hopdx<0.0) sprite->xform=0; // Natural direction is left.
  else sprite->xform=EGG_XFORM_XREV;
  sprite->tileid=SPRITE->tileid0+1;
}

static void rabbit_update_HOP(struct sprite *sprite,double elapsed) {
  sprite->x+=SPRITE->hopdx*elapsed;
  sprite->y+=SPRITE->hopdy*elapsed;
  sprite_rectify_physics(sprite);
}

static void rabbit_begin_DAZE(struct sprite *sprite) {
  SPRITE->stage=RABBIT_STAGE_DAZE;
  SPRITE->stageclock=RABBIT_DAZE_TIME;
  sprite->tileid=SPRITE->tileid0+2;
  struct sprite *daze=sprite_new(&sprite_type_daze,sprite->owner,sprite->x,sprite->y-0.5,0,0);
  if (daze) {
    sprite_daze_setup(daze,sprite);
  }
}

static void rabbit_check_arrows(struct sprite *sprite) {
  double distance;
  struct sprite *arrow=sprites_find_nearest(&distance,sprite->owner,sprite->x,sprite->y,&sprite_type_arrow,0,0,0);
  if (!arrow) return;
  if (distance>RABBIT_ARROW_DISTANCE) return;
  sprite_kill_soon(arrow);
  //TODO sound effect
  rabbit_begin_DAZE(sprite);
}

static void rabbit_catch(struct sprite *sprite) {
  int invp=session_get_free_inventory_slot(g.session);
  if (invp>=0) {
    //TODO sound effect
    //TODO Additional fanfare? "You got a rabbit!"
    g.session->inventory[invp]=NS_item_rabbit;
    sprite_kill_soon(sprite);
  } else {
    //TODO Present a modal like "Drop an item to make room?"
  }
}

// Rabbits can be caught in any stage. But it will be really hard to do without dazing them.
static void rabbit_update_catch(struct sprite *sprite,double elapsed) {
  int danger=0;
  struct sprite *hero=sprites_get_hero(sprite->owner);
  if (hero) {
    double dx=hero->x-sprite->x;
    double dy=hero->y-sprite->y;
    double d2=dx*dx+dy*dy;
    if (d2<=RABBIT_CATCH_DISTANCE*RABBIT_CATCH_DISTANCE) {
      danger=1;
    }
  }
  if (danger) {
    SPRITE->catchclock+=elapsed;
    if (SPRITE->catchclock>=RABBIT_CATCH_TIME) {
      rabbit_catch(sprite);
    }
  } else {
    SPRITE->catchclock=0.0;
  }
}

static int _rabbit_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->lasthop=(rand()%628)/100.0;
  SPRITE->stage=RABBIT_STAGE_HOP; // Start in an expired HOP stage, so the first update puts us in WAIT.
  SPRITE->stageclock=0.0; // Don't enter WAIT right now, since we're not fully attached yet.
  return 0;
}

static void _rabbit_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->stageclock-=elapsed)<=0.0) {
    switch (SPRITE->stage) {
      case RABBIT_STAGE_WAIT: rabbit_begin_HOP(sprite); break;
      case RABBIT_STAGE_HOP: rabbit_begin_WAIT(sprite); break;
      case RABBIT_STAGE_DAZE: rabbit_begin_WAIT(sprite); break;
    }
  }
  switch (SPRITE->stage) {
    case RABBIT_STAGE_WAIT: rabbit_check_arrows(sprite); break;
    case RABBIT_STAGE_HOP: rabbit_update_HOP(sprite,elapsed); rabbit_check_arrows(sprite); break;
  }
  rabbit_update_catch(sprite,elapsed);
}

const struct sprite_type sprite_type_rabbit={
  .name="rabbit",
  .objlen=sizeof(struct sprite_rabbit),
  .init=_rabbit_init,
  .update=_rabbit_update,
};
