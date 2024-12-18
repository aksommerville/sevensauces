#include "game/sauces.h"
#include "game/session/map.h"
#include "game/world/world.h"
#include "sprite.h"

/* Detect and escape map collisions.
 */
 
int sprite_rectify_physics(struct sprite *sprite) {
  int changed=0;
  
  /* First, force sprite into the map's boundaries.
   */
  if (sprite->x<0.5) { sprite->x=0.5; changed=1; }
  else if (sprite->x>g.world->map->w-0.5) { sprite->x=g.world->map->w-0.5; changed=1; }
  if (sprite->y<0.5) { sprite->y=0.5; changed=1; }
  else if (sprite->y>g.world->map->h-0.5) { sprite->y=g.world->map->h-0.5; changed=1; }
  
  /* Since every sprite has a radius 0.5, there can't be more than 4 cells involved.
   * Can we cheese it and make things easier?
   */
  int col=(int)sprite->x,row=(int)sprite->y;
  int lefty=0,uppy=0;
  double dummy;
  if ((col>0)&&(modf(sprite->x,&dummy)<0.5)) { col--; lefty=1; }
  if ((row>0)&&(modf(sprite->y,&dummy)<0.5)) { row--; uppy=1; }
  if (col>g.world->map->w-2) col=g.world->map->w-2;
  if (row>g.world->map->h-2) row=g.world->map->h-2;
  const uint8_t *cellv=g.world->map->v+row*g.world->map->w+col;
  uint8_t solidmask=(1<<NS_physics_solid)|(1<<NS_physics_water);
  uint8_t hits=( // 8,4,2,1=NW,NE,SW,SE
    (((1<<g.world->physics[cellv[0]])&solidmask)?8:0)|
    (((1<<g.world->physics[cellv[1]])&solidmask)?4:0)|
    (((1<<g.world->physics[cellv[g.world->map->w]])&solidmask)?2:0)|
    (((1<<g.world->physics[cellv[g.world->map->w+1]])&solidmask)?1:0)|
  0);
  
  /* There are 16 possible dispositions for the earth beneath us.
   * Handle each case separately, and they're all pretty simple.
   */
  double dx,dy;
  switch (hits) {
  
    case 0x0: return changed; // Open space.
    case 0xf: return changed; // Stuck in a wall. Nothing we can do about it, so just let them go.
  
    /* Concave corners are the easiest case: There is exactly one legal place to stand.
     * This is cool, because usually concave is the hardest case.
     * We're going to call it changed even if she's already in the legal position, rather than checking.
     */
    case 0xe: sprite->x=col+1.5; sprite->y=row+1.5; return 1;
    case 0xd: sprite->x=col+0.5; sprite->y=row+1.5; return 1;
    case 0xb: sprite->x=col+1.5; sprite->y=row+0.5; return 1;
    case 0x7: sprite->x=col+0.5; sprite->y=row+0.5; return 1;
    
    /* The two checkerboard cases are just like concaves, but two legal positions instead of one.
     */
    case 0x6: dx=sprite->x-col; if (dx>=1.0) { sprite->x=col+1.5; sprite->y=row+1.5; } else { sprite->x=col+0.5; sprite->y=row+0.5; } return 1;
    case 0x9: dx=sprite->x-col; if (dx>=1.0) { sprite->x=col+1.5; sprite->y=row+0.5; } else { sprite->x=col+0.5; sprite->y=row+1.5; } return 1;
    
    /* Cardinal cases: force one axis.
     */
    case 0xa: sprite->x=col+1.5; return 1;
    case 0x5: sprite->x=col+0.5; return 1;
    case 0xc: sprite->y=row+1.5; return 1;
    case 0x3: sprite->y=row+0.5; return 1;
    
    /* The four remaining are the convex corners.
     * These are the interesting cases, because the sprite's curvature comes into play.
     * If sprite's center hasn't reached the corner yet, repeat the cardinal case.
     */
    case 0x8: if ((dx=sprite->x-col)<1.0) { sprite->y=row+1.5; return 1; } if ((dy=sprite->y-row)<1.0) { sprite->x=col+1.5; return 1; } goto _convex_;
    case 0x4: if ((dx=sprite->x-col)>1.0) { sprite->y=row+1.5; return 1; } if ((dy=sprite->y-row)<1.0) { sprite->x=col+0.5; return 1; } goto _convex_;
    case 0x2: if ((dx=sprite->x-col)<1.0) { sprite->y=row+0.5; return 1; } if ((dy=sprite->y-row)>1.0) { sprite->x=col+1.5; return 1; } goto _convex_;
    case 0x1: if ((dx=sprite->x-col)>1.0) { sprite->y=row+0.5; return 1; } if ((dy=sprite->y-row)>1.0) { sprite->x=col+0.5; return 1; } goto _convex_;
    _convex_: {
        dx-=1.0; dy-=1.0; // Now from the center.
        if ((dx>-0.001)&&(dx<0.001)&&(dy>-0.001)&&(dy<0.001)) dx=dy=0.001; // I don't want to know what happens when the corner pierces your heart exactly.
        double d2=dx*dx+dy*dy;
        if (d2>=0.25) return changed;
        // The corner breaches the sprite. A consequence of the cardinalization above is that we can correct against just this point, we don't care which corner it is.
        double d=sqrt(d2);
        dx/=d;
        dy/=d;
        sprite->x+=dx*(0.5-d);
        sprite->y+=dy*(0.5-d);
        return 1;
      }
    
  }
  return changed;
}
