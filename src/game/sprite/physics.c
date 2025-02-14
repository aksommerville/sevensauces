#include "game/sauces.h"
#include "game/session/map.h"
#include "game/world/world.h"
#include "sprite.h"

/* Rectify collisions against map.
 */
 
static int sprite_rectify_against_map(struct sprite *sprite) {

  // Which cells am I touching?
  double rad=sprite->radius;
  double l=sprite->x-rad;
  double r=sprite->x+rad;
  double t=sprite->y-rad;
  double b=sprite->y+rad;
  int cola=(int)l; if (cola<0) cola=0;
  int colz=(int)r; if (colz>=g.world->map->w) colz=g.world->map->w-1;
  if (cola>colz) return 0;
  int rowa=(int)t; if (rowa<0) rowa=0;
  int rowz=(int)b; if (rowz>=g.world->map->h) rowz=g.world->map->h-1;
  if (rowa>rowz) return 0;
  
  /* List the impassable cells as integer AABBs, merging aggressively.
   * I expect sprites to be no larger than a map cell, so [8] is actually overkill here.
   */
  struct ibox { int x,y,w,h; } iboxv[8];
  int iboxc=0;
  const uint8_t *cellrow=g.world->map->v+rowa*g.world->map->w+cola;
  int row=rowa; for (;row<=rowz;row++,cellrow+=g.world->map->w) {
    const uint8_t *cellp=cellrow;
    int col=cola; for (;col<=colz;col++,cellp++) {
      uint8_t physics=g.world->map->physics[*cellp];
      if ((1<<physics)&sprite->solidmask) {
        int merged=0,i=iboxc;
        while (i-->0) {
               if ((iboxv[i].w==1)&&(iboxv[i].x==col)&&(row==iboxv[i].y-1)) { merged=1; iboxv[i].y--; iboxv[i].h++; break; }
          else if ((iboxv[i].w==1)&&(iboxv[i].x==col)&&(row==iboxv[i].y+iboxv[i].h)) { merged=1; iboxv[i].h++; break; }
          else if ((iboxv[i].h==1)&&(iboxv[i].y==row)&&(col==iboxv[i].x-1)) { merged=1; iboxv[i].x--; iboxv[i].w++; break; }
          else if ((iboxv[i].h==1)&&(iboxv[i].y==row)&&(col==iboxv[i].x+iboxv[i].w)) { merged=1; iboxv[i].w++; }
        }
        if (merged) continue;
        if (iboxc>=8) continue; // Skip cells if we can't record or merge them in.
        iboxv[iboxc++]=(struct ibox){col,row,1,1};
      }
    }
  }
  if (!iboxc) return 0;
  
  /* One more pass of merging, in case there are >1-sized adjacent boxes, eg a foursquare.
   */
  int ai=iboxc; while (ai-->1) {
    struct ibox *ab=iboxv+ai;
    int bi=ai; while (bi-->0) {
      struct ibox *bb=iboxv+bi;
      #define DROPA { iboxc--; memmove(ab,ab+1,sizeof(struct ibox)*(iboxc-ai)); break; }
      if ((ab->x==bb->x)&&(ab->w==bb->w)) {
        if (ab->y+ab->h==bb->y) {
          bb->y-=ab->h;
          bb->h+=ab->h;
          DROPA
        } else if (ab->y==bb->y+bb->h) {
          bb->h+=ab->h;
          DROPA
        }
      }
      if ((ab->y==bb->y)&&(ab->h==bb->h)) {
        if (ab->x+ab->w==bb->x) {
          bb->x-=ab->w;
          bb->w+=ab->w;
          DROPA
        } else if (ab->x==bb->x+bb->w) {
          bb->w+=ab->w;
          DROPA
        }
      }
      #undef DROPA
    }
  }
  
  if (0) {
    char msg[1024];
    int msgc=snprintf(msg,sizeof(msg),"%d collisions (sprite=%s,%f,%f):",iboxc,sprite->type->name,sprite->x,sprite->y);
    const struct ibox *b=iboxv;
    int i=0; for (;i<iboxc;i++,b++) {
      msgc+=snprintf(msg+msgc,sizeof(msg)-msgc," [%d,%d,%d,%d]",b->x,b->y,b->w,b->h);
    }
    fprintf(stderr,"%.*s\n",msgc,msg);
  }
  
  /* The upshot of this aggressive merging is that we should now have 1 or 2 boxes.
   * 1 in most cases, 2 if we're in a corner.
   * In fact, I'm pretty confident >2 will never happen so I'm not going to solve for it.
   * (that might prove wrong if we ever have sprites larger than a tile).
   */
  if (iboxc>2) {
    fprintf(stderr,"%s:%d: iboxc==%d. I didn't think this was possible. Dropping some collisions arbitrarily.\n",__FILE__,__LINE__,iboxc);
    iboxc=2;
  }
  
  /* When there's 2 boxes, they should form a checkerboard or corner.
   * Collect the set of available positions adjacent to both boxes.
   * There's 8, of which either 1 or 2 should be valid. Take whichever is closest to the sprite.
   * (ie the four edges of A combined with the two other-axis edges of B).
   * We try all 8 possibilities. Six or seven of them will be unreasonable, and that's fine.
   */
  if (iboxc==2) {
    const struct ibox *ab=iboxv,*bb=iboxv+1;
    struct candidate { double x,y; } candidatev[12]={
      {ab->x      -rad,bb->y      -rad},
      {ab->x      -rad,bb->y+bb->h+rad},
      {ab->x+ab->w+rad,bb->y      -rad},
      {ab->x+ab->w+rad,bb->y+bb->h+rad},
      {bb->x      -rad,ab->y      -rad},
      {bb->x      -rad,ab->y+ab->h+rad},
      {bb->x+bb->w+rad,ab->y      -rad},
      {bb->x+bb->w+rad,ab->y+ab->h+rad},
    };
    int candidatec=8;
    
    /* And a wee adjustment: If the two boxes have a common edge,
     * also check escapement to that edge, without moving on the other axis.
     * This can come up at doorways, if the sprite is slightly wider than a tile.
     */
    if (ab->x      ==bb->x      ) candidatev[candidatec++]=(struct candidate){ab->x-rad,sprite->y};
    if (ab->x+ab->w==bb->x+bb->w) candidatev[candidatec++]=(struct candidate){ab->x+ab->w+rad,sprite->y};
    if (ab->y      ==bb->y      ) candidatev[candidatec++]=(struct candidate){sprite->x,ab->y-rad};
    if (ab->y+ab->h==bb->y+bb->h) candidatev[candidatec++]=(struct candidate){sprite->x,ab->y+ab->h+rad};
    
    double best=999999.999,bestx=sprite->x,besty=sprite->y;
    int i=candidatec; while (i-->0) {
      double dx=candidatev[i].x-sprite->x;
      double dy=candidatev[i].y-sprite->y;
      double score=dx*dx+dy*dy;
      if (score<best) {
        best=score;
        bestx=candidatev[i].x;
        besty=candidatev[i].y;
      }
    }
    sprite->x=bestx;
    sprite->y=besty;
    return 1;
  }
  
  /* Collided against a single box.
   * This is by far the likeliest case of map collision and also the easiest.
   * Calculate the escapement in each of the four directions, and escape whichever way is shortest.
   */
  double bl=iboxv[0].x;
  double bt=iboxv[0].y;
  double br=bl+1.0*iboxv[0].w;
  double bb=bt+1.0*iboxv[0].h;
  
  /* Corner case for single-box collision.
   * This is the only case where sprites are not rectangles.
   * It triggers when the sprite's center is outside the box's bounds on both axes.
   * This section can be eliminated, and the only problem would be you get stuck entering narrow entrances sometimes.
   */
  int circular=0;
  double cornerx,cornery;
       if ((sprite->x<bl)&&(sprite->y<bt)) { circular=1; cornerx=bl; cornery=bt; }
  else if ((sprite->x<bl)&&(sprite->y>bb)) { circular=1; cornerx=bl; cornery=bb; }
  else if ((sprite->x>br)&&(sprite->y<bt)) { circular=1; cornerx=br; cornery=bt; }
  else if ((sprite->x>br)&&(sprite->y>bb)) { circular=1; cornerx=br; cornery=bb; }
  if (circular) {
    double dx=sprite->x-cornerx,dy=sprite->y-cornery; // Sign matters, it's the direction of escapement.
    double d=sqrt(dx*dx+dy*dy); // Distance heart to corner.
    double esc=rad-d; // Escape distance.
    if (esc<=0.0) return 0; // In the circular case, it's possible it wasn't a collision after all.
    sprite->x+=esc*dx/d;
    sprite->y+=esc*dy/d;
    return 1;
  }
   
  /* Single-box collision, escape on a single axis.
   * Far and away the likeliest case of collisions.
   */
  double escl=r-bl;
  double escr=br-l;
  double esct=b-bt;
  double escb=bb-t;
  if ((escl<=escr)&&(escl<=esct)&&(escl<=escb)) sprite->x-=escl;
  else if ((escr<=esct)&&(escr<=escb)) sprite->x+=escr;
  else if (esct<=escb) sprite->y-=esct;
  else sprite->y+=escb;
  return 1;
}

/* Detect and escape map collisions.
 */
 
int sprite_rectify_physics(struct sprite *sprite,double border_radius) {
  int changed=0;
  
  /* First, force sprite into the map's boundaries.
   */
  if (border_radius<sprite->radius) border_radius=sprite->radius;
  if (sprite->x<border_radius) { sprite->x=border_radius; changed=1; }
  else if (sprite->x>g.world->map->w-border_radius) { sprite->x=g.world->map->w-border_radius; changed=1; }
  if (sprite->y<border_radius) { sprite->y=border_radius; changed=1; }
  else if (sprite->y>g.world->map->h-border_radius) { sprite->y=g.world->map->h-border_radius; changed=1; }
  
  /* Then if we're doing map collisions, see above.
   */
  if (sprite->solidmask) {
    if (sprite_rectify_against_map(sprite)) changed=1;
  }
  return changed;
}
