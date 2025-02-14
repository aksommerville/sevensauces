/* sprite_type_hero.c
 * Definition of the sprite type, and its direct hooks.
 */

#include "hero_internal.h"

static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */

static int _hero_init(struct sprite *sprite) {
  sprite->imageid=RID_image_sprites1;
  sprite->tileid=0x00;
  SPRITE->facedy=1;
  SPRITE->pressx=SPRITE->pressy=-1;
  return 0;
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  if (SPRITE->item_in_use==NS_item_fishpole) {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      hero_fishpole_ready(sprite);
    }
    return;
  }

  if ((SPRITE->cursorclock-=elapsed)<0.0) {
    SPRITE->cursorclock+=0.200;
    if (++(SPRITE->cursorframe)>=4) SPRITE->cursorframe=0;
  }
  
  if ((SPRITE->animclock-=elapsed)<0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }

  if (SPRITE->walkdx||SPRITE->walkdy) {
    const double walkspeed=6.0; // m/s
    sprite->x+=walkspeed*elapsed*SPRITE->walkdx;
    sprite->y+=walkspeed*elapsed*SPRITE->walkdy;
    if (sprite_rectify_physics(sprite,0.5)) hero_check_press(sprite);
    else hero_no_press(sprite);
  } else hero_no_press(sprite);
  
  hero_update_item(sprite,elapsed);
  if (SPRITE->item_in_use==NS_item_grapple) {
    // Don't interact with the world while grappling. Important if we're being pulled, should be fine when still too.
  } else {
    hero_check_touches(sprite);
    hero_check_cell(sprite);
  }
}

/* Wieldable: When this item is in use, Dot shows her sword face.
 * Returns the nonzero weapon tileid, or zero if not wieldable.
 */
 
static inline int hero_item_is_wieldable(uint8_t itemid) {
  switch (itemid) {
    case NS_item_sword: return 0x13;
    case NS_item_grapple: return 0x23;
    case NS_item_apology: return 0x14;
    case NS_item_loveletter: return 0x15;
  }
  return 0;
}

/* Render.
 */

static void _hero_render(struct sprite *sprite,int16_t x,int16_t y) {
  const struct item *item=itemv+g.session->inventory[g.session->invp];
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  
  // Fishpole deployed and animating? That's a whole different thing.
  if (SPRITE->item_in_use==NS_item_fishpole) {
    int linex=x,liney=y;
    uint8_t tileid,xform=0;
         if (SPRITE->facedx<0) { linex-=NS_sys_tilesize; tileid=0x0c; }
    else if (SPRITE->facedx>0) { linex+=NS_sys_tilesize; tileid=0x0c; xform=EGG_XFORM_XREV; }
    else if (SPRITE->facedy<0) { liney-=NS_sys_tilesize; tileid=0x09; }
    else                       { liney+=NS_sys_tilesize; tileid=0x06; }
    if (SPRITE->fish_outcome&&(SPRITE->animclock<0.500)) { // "oh!"
      graf_draw_tile(&g.graf,texid,x,y,tileid+2,xform);
      int16_t fishy=(int16_t)(liney-(0.500-SPRITE->animclock)*60.0);
      uint8_t fishtile;
           if (SPRITE->animclock>=0.350) fishtile=0x18;
      else if (SPRITE->animclock>=0.200) fishtile=0x1b;
      else if (SPRITE->animclock>=0.050) fishtile=0x18;
      else                               fishtile=0x1e;
      graf_draw_tile(&g.graf,texid,linex,fishy,fishtile,0);
    } else {
      double whole,fract;
      fract=modf(SPRITE->animclock,&whole);
      if (fract>=0.500) tileid++;
      graf_draw_tile(&g.graf,texid,x,y,tileid,xform);
      graf_draw_tile(&g.graf,texid,linex,liney,tileid+0x10,xform);
    }
    return;
  }
  
  if (item->flags&(1<<NS_itemflag_cell)) {
    int col=-100,row=0;
    hero_get_item_cell(&col,&row,sprite);
    int16_t dstx=col*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.world->viewx;
    int16_t dsty=row*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.world->viewy;
    uint8_t xform=0;
    switch (SPRITE->cursorframe) {
      case 1: xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 2: xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    }
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x40,xform);
  }

  // (facedx,facedy) are the authority for our face direction. Update (tileid,xform) accordingly.
       if (SPRITE->facedx<0) { sprite->tileid=0x02; sprite->xform=0; }
  else if (SPRITE->facedx>0) { sprite->tileid=0x02; sprite->xform=EGG_XFORM_XREV; }
  else if (SPRITE->facedy<0) { sprite->tileid=0x01; sprite->xform=0; }
  else                       { sprite->tileid=0x00; sprite->xform=0; }
  
  // Update (tileid) per action and animation, and draw other decorations. tileid currently 0, 1, or 2, depending on facedir.
  uint8_t wieldtile=hero_item_is_wieldable(SPRITE->item_in_use);
  if (wieldtile) {
    sprite->tileid+=0x03;
    int16_t swordx=x+SPRITE->facedx*NS_sys_tilesize;
    int16_t swordy=y+SPRITE->facedy*NS_sys_tilesize;
    uint8_t swordxform=0; // Natural orientation is down, and when horizontal, put the natural left edge at the bottom both ways.
    if (SPRITE->facedx<0) swordxform=EGG_XFORM_SWAP|EGG_XFORM_XREV|EGG_XFORM_YREV;
    else if (SPRITE->facedx>0) swordxform=EGG_XFORM_SWAP|EGG_XFORM_XREV;
    else if (SPRITE->facedy<0) swordxform=EGG_XFORM_XREV|EGG_XFORM_YREV;
    graf_draw_tile(&g.graf,texid,swordx,swordy,wieldtile,swordxform);
    
  } else if (SPRITE->walkdx||SPRITE->walkdy) {
    switch (SPRITE->animframe) {
      case 0: sprite->tileid+=0x10; break;
      case 2: sprite->tileid+=0x20; break;
    }
  }

  graf_draw_tile(&g.graf,texid,x,y,sprite->tileid,sprite->xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
