#include "game/sauces.h"
#include "game/kitchen/kitchen.h"

#define BUBBLEC 10

struct bobbler {
  int tr,tg,tb; // Tint for the base color.
  struct bubble {
    int x,y; // Relative to the stew.
    int ttl;
  } bubblev[BUBBLEC];
  struct ingredient {
    double x,y; // Top-left, before bobble adjustment, relative to the stew.
    double dx,dy; // Pixel/frame.
    double bobblet; // Bobble phase in radians. Amplitude and frequency are constant for all.
    uint8_t itemid; // If there are two of the same itemid, we don't track which is which (no point).
    uint8_t scratch; // For transient use in matching.
  } ingredientv[INVENTORY_SIZE];
  int ingredientc;
  int gravityp;
};

/* Delete.
 */

void bobbler_del(struct bobbler *bobbler) {
  if (!bobbler) return;
  free(bobbler);
}

/* Initialize one bubble from scratch.
 */
 
static void bubble_init(struct bubble *bubble) {
  bubble->x=6+rand()%131;
  bubble->y=4+rand()%20;
  bubble->ttl=50;
}

/* New.
 */
 
struct bobbler *bobbler_new() {
  struct bobbler *bobbler=calloc(1,sizeof(struct bobbler));
  if (!bobbler) return 0;
  
  // Empty stew has a perfect middle gray tint, yuck.
  bobbler->tr=0x80;
  bobbler->tg=0x80;
  bobbler->tb=0x80;
  
  struct bubble *bubble=bobbler->bubblev;
  int i=BUBBLEC; for (;i-->0;bubble++) {
    bubble_init(bubble);
    bubble->ttl=(bubble->ttl*(rand()&0xff))>>8;
  }
  
  return bobbler;
}

/* The stew's base color is derived from the ingredients' food groups.
 * We pay out changes over a few frames.
 */
 
static uint32_t bobbler_choose_tint(struct bobbler *bobbler) {
  uint8_t rtarget,gtarget,btarget;
  
  int vegc=0,meatc=0,candyc=0,saucec=0,poisonc=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!g.kitchen->selected[invp]) continue;
    const struct item *item=itemv+g.session->inventory[invp];
    switch (item->foodgroup) {
      case NS_foodgroup_veg: vegc++; break;
      case NS_foodgroup_meat: meatc++; break;
      case NS_foodgroup_candy: candyc++; break;
      case NS_foodgroup_sauce: saucec++; break;
      case NS_foodgroup_poison: poisonc++; break;
    }
  }
  
  // Any poison at all, we go shocking red.
  if (poisonc) {
    rtarget=0xed;
    gtarget=0x13;
    btarget=0x48;
    
  // If no ingredients (or only inedible ones), middle gray.
  // Or if there's only sauce, bright yellow.
  } else if (!vegc&&!meatc&&!candyc) {
    if (saucec) {
      rtarget=0xdc;
      gtarget=0xd1;
      btarget=0x22;
    } else {
      rtarget=0x80;
      gtarget=0x80;
      btarget=0x80;
    }
    
  // (balanced,veg,meat,candy)=(blue,green,pink,cyan), and go much brighter if there's sauce.
  } else if ((vegc==meatc)&&(meatc==candyc)) {
    if (saucec) { // balanced+sauce
      rtarget=0xce;
      gtarget=0xb0;
      btarget=0x53;
    } else { // balanced dry
      rtarget=0x94;
      gtarget=0x86;
      btarget=0x58;
    }
  } else if ((vegc>meatc)&&(vegc>candyc)) {
    if (saucec) { // veg+sauce
      rtarget=0x20;
      gtarget=0xee;
      btarget=0x1b;
    } else { // veg dry
      rtarget=0x0f;
      gtarget=0x85;
      btarget=0x0c;
    }
  } else if ((meatc>vegc)&&(meatc>candyc)) {
    if (saucec) { // meat+sauce
      rtarget=0xb9;
      gtarget=0x79;
      btarget=0x22;
    } else { // meat dry
      rtarget=0x7e;
      gtarget=0x4d;
      btarget=0x25;
    }
  } else if ((candyc>vegc)&&(candyc>meatc)) {
    if (saucec) { // candy+sauce
      rtarget=0x46;
      gtarget=0x9e;
      btarget=0xd1;
    } else { // candy dry
      rtarget=0x19;
      gtarget=0x61;
      btarget=0x8a;
    }
    
  // No valid majority.
  } else {
    if (saucec) {
      rtarget=0xa0;
      gtarget=0x76;
      btarget=0xc0;
    } else {
      rtarget=0x6a;
      gtarget=0x55;
      btarget=0x7a;
    }
  }
  
  /* Approach the target color.
   */
  const int adjust_speed=2;
  if (bobbler->tr<rtarget) {
    if ((bobbler->tr+=adjust_speed)>rtarget) bobbler->tr=rtarget;
  } else if (bobbler->tr>rtarget) {
    if ((bobbler->tr-=adjust_speed)<rtarget) bobbler->tr=rtarget;
  }
  if (bobbler->tg<gtarget) {
    if ((bobbler->tg+=adjust_speed)>gtarget) bobbler->tg=gtarget;
  } else if (bobbler->tg>gtarget) {
    if ((bobbler->tg-=adjust_speed)<gtarget) bobbler->tg=gtarget;
  }
  if (bobbler->tb<btarget) {
    if ((bobbler->tb+=adjust_speed)>btarget) bobbler->tb=btarget;
  } else if (bobbler->tb>btarget) {
    if ((bobbler->tb-=adjust_speed)<btarget) bobbler->tb=btarget;
  }
  
  return (bobbler->tr<<24)|(bobbler->tg<<16)|(bobbler->tb<<8)|0xff;
}

/* Add and remove ingredients to match the global kitchen's selections.
 * When adding, they get random position, velocity, and bobble phase.
 */
 
static void bobbler_match_ingredients(struct bobbler *bobbler) {
  int i,invp;
  struct ingredient *ingredient;
  for (i=bobbler->ingredientc,ingredient=bobbler->ingredientv;i-->0;ingredient++) ingredient->scratch=0;
  // Add ingredients for anything in the kitchen we don't have, and scratch the ones we do.
  for (invp=INVENTORY_SIZE;invp-->0;) {
    if (!g.kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    int got=0;
    for (i=bobbler->ingredientc,ingredient=bobbler->ingredientv;i-->0;ingredient++) {
      if (ingredient->scratch) continue;
      if (ingredient->itemid==itemid) {
        ingredient->scratch=1;
        got=1;
        break;
      }
    }
    if (got) continue;
    if (bobbler->ingredientc>=INVENTORY_SIZE) continue; // Need to add, but we need to remove something first. Shouldn't be possible but whatever.
    ingredient=bobbler->ingredientv+bobbler->ingredientc++;
    ingredient->x=4+rand()%116;
    ingredient->y=-8+rand()%15;
    ingredient->dx=(((rand()&0xffff)/65535.0)-0.5)*0.050;
    ingredient->dy=(((rand()&0xffff)/65535.0)-0.5)*0.020;
    ingredient->bobblet=((rand()&0xffff)/65535.0)*M_PI*2.0;
    ingredient->itemid=itemid;
    ingredient->scratch=1;
  }
  // And now remove any we didn't scratch.
  for (i=bobbler->ingredientc,ingredient=bobbler->ingredientv+bobbler->ingredientc-1;i-->0;ingredient--) {
    if (!ingredient->scratch) {
      bobbler->ingredientc--;
      memmove(ingredient,ingredient+1,sizeof(struct ingredient)*(bobbler->ingredientc-i));
    }
  }
}

/* Adjust velocity, position, and bobble phase for each ingredient.
 * When they're all updated, sort by render order.
 */
 
static void bobbler_update_ingredients(struct bobbler *bobbler) {
  const double speedlimit=0.010;
  int i=bobbler->ingredientc;
  struct ingredient *ingredient=bobbler->ingredientv;
  for (;i-->0;ingredient++) {
    ingredient->x+=ingredient->dx;
    if ((ingredient->x<4.0)&&(ingredient->dx<0.0)) ingredient->dx*=-1.0;
    else if ((ingredient->x>120.0)&&(ingredient->dx>0.0)) ingredient->dx*=-1.0;
    ingredient->y+=ingredient->dy;
    if ((ingredient->y<-8.0)&&(ingredient->dy<0.0)) ingredient->dy*=-1.0;
    else if ((ingredient->y>7.0)&&(ingredient->dy>0.0)) ingredient->dy*=-1.0;
    ingredient->dx+=((rand()&0xffff)-32768.0)/32768.0*0.001;
    ingredient->dy+=((rand()&0xffff)-32768.0)/32768.0*0.001;
    if (ingredient->dx<-speedlimit) ingredient->dx=-speedlimit;
    else if (ingredient->dx>speedlimit) ingredient->dx=speedlimit;
    if (ingredient->dy<-speedlimit) ingredient->dy=-speedlimit;
    else if (ingredient->dy>speedlimit) ingredient->dy=speedlimit;
    if ((ingredient->bobblet+=0.100)>=M_PI) ingredient->bobblet-=M_PI*2.0;
  }
  for (i=bobbler->ingredientc-1,ingredient=bobbler->ingredientv;i-->0;ingredient++) {
    if (ingredient[0].y>ingredient[1].y) {
      struct ingredient tmp=ingredient[0];
      ingredient[0]=ingredient[1];
      ingredient[1]=tmp;
    }
  }
}

/* Each frame, apply a little extra anti-gravity to one ingredient, cycling thru them over time.
 * When it's the only ingredient, mosey centerward.
 * When there's more than one, bias away from the nearest neighbors.
 */
 
static void bobbler_extra_anti_gravity(struct bobbler *bobbler) {
  if (bobbler->ingredientc<1) return;
  if (bobbler->gravityp>=bobbler->ingredientc) bobbler->gravityp=0;
  struct ingredient *ingredient=bobbler->ingredientv+bobbler->gravityp++;
  double gravx=0.0,gravy=0.0;
  if (bobbler->ingredientc==1) {
    if ((ingredient->x>=60.0)&&(ingredient->x<=64.0)&&(ingredient->y>=0.0)&&(ingredient->y<4.0)) return;
    gravx=1.0/(62.0-ingredient->x);
    gravy=1.0/(2.0-ingredient->y);
  } else {
    struct ingredient *other=bobbler->ingredientv;
    int i=bobbler->ingredientc;
    for (;i-->0;other++) {
      double dx=ingredient->x-other->x;
      double dy=ingredient->y-other->y;
      // Ignore anything right on top of me, because rounding could make it fly haywire.
      // This also causes us to ignore ourself, which is necessary.
      if ((dx>-1.0)&&(dx<1.0)&&(dy>-1.0)&&(dy<1.0)) continue;
      gravx+=1.0/dx; // Should we square these denominators?
      gravy+=1.0/dy;
    }
  }
  double ddx=gravx*0.010;
  double ddy=gravy*0.010;
  const double thresh=0.010;
  if (ddx<-thresh) ddx=-thresh; else if (ddx>thresh) ddx=thresh;
  if (ddy<-thresh) ddy=-thresh; else if (ddy>thresh) ddy=thresh;
  ingredient->dx+=ddx;
  ingredient->dy+=ddy;
}

/* Render.
 */
 
void bobbler_render_surface(int dstx,int dsty,struct bobbler *bobbler) {
  if (!bobbler) return;
  
  /* Base color.
   */
  int texid=texcache_get_image(&g.texcache,RID_image_kitchen_bits);
  uint32_t tint=bobbler_choose_tint(bobbler);
  graf_set_tint(&g.graf,tint);
  graf_draw_decal(&g.graf,texid,dstx,dsty,1,40,143,28,0);
  
  /* Update and render bubbles, and replace them if their TTL expires.
   */
  uint32_t bubbletint=((tint>>1)&0x7f7f7f00)|0xff;
  graf_set_tint(&g.graf,bubbletint);
  struct bubble *bubble=bobbler->bubblev;
  int i=BUBBLEC; for (;i-->0;bubble++) {
    if (bubble->ttl-->0) {
      int frame=4-bubble->ttl/10;
      if (frame>4) frame=4;
      graf_draw_decal(&g.graf,texid,dstx+bubble->x,dsty+bubble->y,138+frame*7,34,6,5,0);
    } else {
      bubble_init(bubble);
    }
  }
  graf_set_tint(&g.graf,0);
}

void bobbler_render_overlay(int dstx,int dsty,struct bobbler *bobbler) {
  if (!bobbler) return;
  
  /* Update ingredients.
   * Or if there's none, we can terminate here.
   */
  bobbler_match_ingredients(bobbler);
  if (bobbler->ingredientc<1) return;
  bobbler_extra_anti_gravity(bobbler);
  bobbler_update_ingredients(bobbler);
  
  /* Render ingredients.
   */
  int texid=texcache_get_image(&g.texcache,RID_image_item),i;
  struct ingredient *ingredient=bobbler->ingredientv;
  for (i=bobbler->ingredientc;i-->0;ingredient++) {
    double bobbley=sin(ingredient->bobblet)*2.0;
    int ix=dstx+(int)ingredient->x+(NS_sys_tilesize>>1);
    int iy=dsty+(int)(ingredient->y+bobbley)+(NS_sys_tilesize>>1);
    int h=NS_sys_tilesize-(int)(bobbley+0.5)-2;
    int srcx=(ingredient->itemid&0x0f)*NS_sys_tilesize;
    int srcy=(ingredient->itemid>>4)*NS_sys_tilesize;
    graf_draw_decal(&g.graf,texid,ix,iy,srcx,srcy,NS_sys_tilesize,h,0);
    int lx=ix,lw=NS_sys_tilesize;
    if (bobbley>0.0) { lx-=1; lw+=1; }
  }
}
