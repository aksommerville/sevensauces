#include "sprite_internal.h"
#include "game/layer/layer.h"
#include "game/world/world.h"
#include "game/session/map.h"

/* Delete.
 */
 
void sprites_del(struct sprites *sprites) {
  if (!sprites) return;
  if (sprites->v) {
    while (sprites->c-->0) sprite_del(sprites->v[sprites->c]);
    free(sprites->v);
  }
  if (sprites->deathrow) free(sprites->deathrow);
  free(sprites);
}

/* New.
 */
 
struct sprites *sprites_new() {
  struct sprites *sprites=calloc(1,sizeof(struct sprites));
  if (!sprites) return 0;
  return sprites;
}

/* Trivial accessors.
 */
 
struct sprite *sprites_get_hero(struct sprites *sprites) {
  if (!sprites) return 0;
  return sprites->hero;
}

int sprites_get_all(struct sprite ***dstppp,const struct sprites *sprites) {
  if (!sprites) return 0;
  *dstppp=sprites->v;
  return sprites->c;
}

/* Delete all sprites.
 */

void sprites_drop_all(struct sprites *sprites) {
  sprites->deathrowc=0;
  sprites->hero=0;
  while (sprites->c>0) {
    sprites->c--;
    sprite_del(sprites->v[sprites->c]);
  }
}

/* Test residence.
 */

int sprite_is_resident(const struct sprites *sprites,const struct sprite *sprite) {
  if (!sprites||!sprite) return 0;
  struct sprite **q=sprites->v;
  int i=sprites->c;
  for (;i-->0;q++) if (*q==sprite) return 1;
  return 0;
}

/* Find nearest.
 */
 
struct sprite *sprites_find_nearest(
  double *distance,
  const struct sprites *sprites,
  double x,double y,
  const struct sprite_type *type, // Any if null.
  int rid, // Any if zero.
  int (*match)(struct sprite *closer,struct sprite *prev,void *userdata), // <0 to match (closer) and abort, >0 to match (closer), 0 to keep (prev). (prev) may be null.
  void *userdata
) {
  struct sprite *best=0;
  double bestmd=999999.0; // Manhattan distance
  int i=sprites->c;
  struct sprite **q=sprites->v;
  for (;i-->0;q++) {
    struct sprite *sprite=*q;
    if (type&&(sprite->type!=type)) continue;
    if (rid&&(sprite->rid!=rid)) continue;
    double dx=sprite->x-x; if (dx<0.0) dx=-dx;
    double dy=sprite->y-y; if (dy<0.0) dy=-dy;
    double md=dx+dy;
    if (md>=bestmd) continue;
    if (match) {
      int rsp=match(sprite,best,userdata);
      if (!rsp) continue;
      if (rsp<0) return sprite;
    }
    best=sprite;
    bestmd=md;
  }
  if (!best) return 0;
  if (distance) {
    double dx=best->x-x;
    double dy=best->y-y;
    *distance=sqrt(dx*dx+dy*dy);
  }
  return best;
}

/* List or unlist, and hand off ownership.
 */
 
void sprites_unlist(struct sprites *sprites,struct sprite *sprite) {
  if (!sprites||!sprite) return;
  int i=sprites->c;
  while (i-->0) {
    if (sprites->v[i]==sprite) {
      sprites->c--;
      memmove(sprites->v+i,sprites->v+i+1,sizeof(void*)*(sprites->c-i));
      if (sprite==sprites->hero) sprites->hero=0;
      return;
    }
  }
}

int sprites_list(struct sprites *sprites,struct sprite *sprite) {
  if (!sprites||!sprite) return -1;
  if (sprites!=sprite->owner) return -1;
  int i=sprites->c;
  while (i-->0) if (sprites->v[i]==sprite) return 0;
  if (sprites->c>=sprites->a) {
    int na=sprites->a+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(sprites->v,sizeof(void*)*na);
    if (!nv) return 0;
    sprites->v=nv;
    sprites->a=na;
  }
  sprites->v[sprites->c++]=sprite;
  if (sprite->type==&sprite_type_hero) sprites->hero=sprite;
  return 1;
}

/* Delete any sprites on deathrow.
 * We will call this at times when no iteration or sprite-controller code is expected to be running.
 */
 
static void sprites_execute_deathrow(struct sprites *sprites) {
  while (sprites->deathrowc>0) {
    sprites->deathrowc--;
    sprite_del(sprites->deathrow[sprites->deathrowc]);
  }
}

/* Update.
 */

void sprites_update(struct sprites *sprites,double elapsed) {
  sprites_execute_deathrow(sprites);
  struct layer *layer=layer_stack_get_focus();
  
  // Iterate by index and backward, so we're safe against appends happening along the way.
  // Sprites are encouraged to "kill_soon" instead of "del", removes won't be a problem if they obey that.
  // Insertions could be a problem. But I think we'll only append as new sprites spawn.
  int i=sprites->c;
  while (i-->0) {
    if (i>=sprites->c) continue;
    struct sprite *sprite=sprites->v[i];
    if (sprite->type->update) {
      sprite->type->update(sprite,elapsed);
      // If the focus layer changed during a sprite update -- entirely possible -- get out immediately.
      if (layer_stack_get_focus()!=layer) return;
    }
  }
  
  sprites_execute_deathrow(sprites);
  
  //TODO Generic update processing. Physics?
}

/* Single-pass bubble sort for render order.
 */
 
static int sprites_rendercmp(const struct sprite *a,const struct sprite *b) {
  if (a->layer<b->layer) return -1;
  if (a->layer>b->layer) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  return 0;
}
 
static void sprites_sort_1(struct sprites *sprites) {
  if (sprites->c<2) return;
  int first,last,i,d;
  if (sprites->sortdir==1) {
    first=0;
    last=sprites->c-1;
    d=1;
    sprites->sortdir=-1;
  } else {
    first=sprites->c-1;
    last=0;
    d=-1;
    sprites->sortdir=1;
  }
  for (i=first;i!=last;i+=d) {
    struct sprite *a=sprites->v[i];
    struct sprite *b=sprites->v[i+d];
    int cmp=sprites_rendercmp(a,b);
    if (cmp==d) {
      sprites->v[i]=b;
      sprites->v[i+d]=a;
    }
  }
}

/* Render.
 */
 
void sprites_render(struct sprites *sprites) {
  
  sprites_sort_1(sprites);

  int xlo=-NS_sys_tilesize; // Assume anything whose center is more than a tile offscreen is completely invisible.
  int ylo=-NS_sys_tilesize;
  int xhi=FBW+NS_sys_tilesize;
  int yhi=FBH+NS_sys_tilesize;
  int texid=0,imageid=0;
  int i=0; for (;i<sprites->c;i++) {
    struct sprite *sprite=sprites->v[i];
    int x=(int)(sprite->x*NS_sys_tilesize)-g.world->viewx;
    int y=(int)(sprite->y*NS_sys_tilesize)-g.world->viewy;
    
    if (sprite->type->render) {
      graf_flush(&g.graf); // Flushing is wise, in case the sprite's renderer invalidates texcache.
      sprite->type->render(sprite,x,y);
      texid=imageid=0; // Assume that texcache has gone stale.
      
    } else {
      // Generic culling is only for single-tile sprites. Customer renderers might cover a very broad area (eg grapple).
      if ((x<xlo)||(x>xhi)) continue;
      if ((y<ylo)||(y>yhi)) continue;
    
      if (sprite->imageid!=imageid) {
        imageid=sprite->imageid;
        texid=texcache_get_image(&g.texcache,sprite->imageid);
      }
      graf_draw_tile(&g.graf,texid,x,y,sprite->tileid,sprite->xform);
    }
    
    if (0&&(sprite->type==&sprite_type_arrow)) {//XXX highlight hitboxes
      int x=(int)(sprite->x*NS_sys_tilesize)-g.world->viewx;
      int y=(int)(sprite->y*NS_sys_tilesize)-g.world->viewy;
      int w=(int)(sprite->radius*2.0*NS_sys_tilesize);
      int h=(int)(sprite->radius*2.0*NS_sys_tilesize);
      x-=w>>1;
      y-=h>>1;
      graf_draw_rect(&g.graf,x,y,w,h,0xff000080);
    }
  }
}

/* Find spawn position.
 */
 
void sprites_find_spawn_position(double *x,double *y,const struct sprites *sprites) {
  
  /* First, if the natural position is in bounds, doesn't overlap any impassable map cells,
   * and no more than 50% axiswise overlap of other sprites, keep it.
   */
  double l=(*x)-0.5,r=(*x)+0.5,t=(*y)-0.5,b=(*y)+0.5;
  if ((l>=0.0)&&(t>=0.0)&&(r<g.world->map->w)&&(b<g.world->map->h)) {
    int cola=(int)l,colz=(int)r,rowa=(int)t,rowz=(int)b;
    const uint8_t *prow=g.world->map->v+rowa*g.world->map->w+cola;
    int row=rowa; for (;row<=rowz;row++,prow+=g.world->map->w) {
      const uint8_t *p=prow;
      int col=cola; for (;col<=colz;col++,p++) {
        uint8_t physics=g.world->map->physics[*p];
        switch (physics) {
          case NS_physics_solid:
          case NS_physics_water:
          case NS_physics_harvest:
          case NS_physics_grapplable:
            goto _not_here_;
          // harvest is passable of course, but it's awkward to have sprites on top of one.
        }
      }
    }
    struct sprite **p=sprites->v;
    int i=sprites->c;
    for (;i-->0;p++) {
      struct sprite *sprite=*p;
      if (sprite->x<=l) continue;
      if (sprite->x>=r) continue;
      if (sprite->y<=t) continue;
      if (sprite->y>=b) continue;
      goto _not_here_;
    }
    return; // OK, it's fine exactly where it's at.
   _not_here_:;
  }
  
  /* Drop it aligned on a map cell, up to 2 cells on either axis from the start.
   * First gather the candidate cells.
   */
  struct candidate { int x,y; } candidatev[25];
  int candidatec=0;
  int col=(int)(*x)-2,colc=5;
  int row=(int)(*y)-2,rowc=5;
  if (col<0) { colc+=col; col=0; }
  if (row<0) { rowc+=row; row=0; }
  if (col+colc>g.world->map->w) colc=g.world->map->w-col;
  if (row+rowc>g.world->map->h) rowc=g.world->map->h-row;
  if ((colc<1)||(rowc<1)) return; // hmm, oops?
  const uint8_t *prow=g.world->map->v+row*g.world->map->w+col;
  int rowi=0; for (;rowi<rowc;rowi++,prow+=g.world->map->w) {
    const uint8_t *p=prow;
    int coli=0; for (;coli<colc;coli++,p++) {
      uint8_t physics=g.world->map->physics[*p];
      switch (physics) {
        case NS_physics_solid:
        case NS_physics_water:
        case NS_physics_harvest:
        case NS_physics_grapplable:
          continue;
      }
      candidatev[candidatec++]=(struct candidate){col+coli,row+rowi};
    }
  }
  if (candidatec<1) return;
  
  /* Eliminate any candidates where an existing sprite's center is in that cell.
   */
  struct sprite **p=sprites->v;
  int i=sprites->c;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    int sx=(int)sprite->x;
    if ((sx<col)||(sx>=col+colc)) continue;
    int sy=(int)sprite->y;
    if ((sy<row)||(sy>=row+rowc)) continue;
    int ci=candidatec; while (ci-->0) {
      if (candidatev[ci].x!=sx) continue;
      if (candidatev[ci].y!=sy) continue;
      candidatec--;
      memmove(candidatev+ci,candidatev+ci+1,sizeof(struct candidate)*(candidatec-ci));
    }
  }
  
  /* And take whichever candidate is nearest the original point.
   * It's fine if the candidate list is empty.
   */
  double bestx=*x,besty=*y;
  double bestdist=999.0;
  const struct candidate *cn=candidatev;
  for (i=candidatec;i-->0;cn++) {
    double dx=cn->x-(*x);
    double dy=cn->y-(*y);
    double d=dx*dx+dy*dy;
    if (d<bestdist) {
      bestx=cn->x+0.5;
      besty=cn->y+0.5;
      bestdist=d;
    }
  }
  *x=bestx;
  *y=besty;
}
