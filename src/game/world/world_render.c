#include "game/sauces.h"
#include "game/session/map.h"
#include "game/sprite/sprite.h"
#include "world.h"

/* Select camera position.
 */
 
static void world_update_camera(struct world *world) {

  int worldw=world->map->w*NS_sys_tilesize;
  int worldh=world->map->h*NS_sys_tilesize;
  if ((worldw<=FBW)&&(worldh<=FBH)) {
    world->viewx=(worldw>>1)-(FBW>>1);
    world->viewy=(worldh>>1)-(FBH>>1);
    return;
  }

  struct sprite *hero=sprites_get_hero(world->sprites);
  if (!hero) return;
  
  if (worldw<=FBW) {
    world->viewx=(worldw>>1)-(FBW>>1);
  } else {
    world->viewx=(int)(hero->x*NS_sys_tilesize)-(FBW>>1);
    if (world->viewx<0) world->viewx=0;
    else if (world->viewx>worldw-FBW) world->viewx=worldw-FBW;
  }
  if (worldh<FBH) {
    world->viewy=(worldh>>1)-(FBH>>1);
  } else {
    world->viewy=(int)(hero->y*NS_sys_tilesize)-(FBH>>1);
    if (world->viewy<0) world->viewy=0;
    else if (world->viewy>worldh-FBH) world->viewy=worldh-FBH;
  }
}

/* Render map and optional blotter.
 * Always fills the framebuffer.
 */
 
static void world_render_map(struct world *world) {
  if ((world->viewx<0)||(world->viewy<0)) {
    // We can't overshoot the right or bottom edges without also overshooting left or top respectively.
    graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  }
  int cola=world->viewx/NS_sys_tilesize; if (cola<0) cola=0;
  int colz=(world->viewx+FBW-1)/NS_sys_tilesize; if (colz>=world->map->w) colz=world->map->w-1;
  int rowa=world->viewy/NS_sys_tilesize; if (rowa<0) rowa=0;
  int rowz=(world->viewy+FBH-1)/NS_sys_tilesize; if (rowz>=world->map->h) rowz=world->map->h-1;
  if ((cola>colz)||(rowa>rowz)) return;
  int texid=texcache_get_image(&g.texcache,world->map->imageid);
  graf_draw_tile_buffer(&g.graf,texid,
    cola*NS_sys_tilesize-world->viewx+(NS_sys_tilesize>>1),
    rowa*NS_sys_tilesize-world->viewy+(NS_sys_tilesize>>1),
    world->map->v+rowa*world->map->w+cola,
    colz-cola+1,rowz-rowa+1,world->map->w
  );
}

/* Render, main entry point.
 */
 
void world_render(struct world *world) {
  world_update_camera(world);
  world_render_map(world);
  sprites_render(world->sprites);
  //TODO overlay
}
