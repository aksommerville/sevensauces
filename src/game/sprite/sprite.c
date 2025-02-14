#include "sprite_internal.h"

/* Delete and unlist.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  
  // Unlist.
  if (sprite->owner) {
    int i=sprite->owner->c;
    while (i-->0) {
      if (sprite->owner->v[i]==sprite) {
        sprite->owner->c--;
        memmove(sprite->owner->v+i,sprite->owner->v+i+1,sizeof(void*)*(sprite->owner->c-i));
        break;
      }
    }
    if (sprite->owner->hero==sprite) sprite->owner->hero=0;
  }
  
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* Run commands only to extract type.
 */
 
static const struct sprite_type *sprite_type_from_res(const struct rom_sprite *rspr) {
  struct rom_command_reader reader={.v=rspr->cmdv,.c=rspr->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_type: switch ((cmd.argv[0]<<8)|cmd.argv[1]) {
          #define _(tag) case NS_spritetype_##tag: return &sprite_type_##tag;
          FOR_EACH_SPRITE_TYPE
          #undef _
          default: fprintf(stderr,"Undefined sprite type %d.\n",(cmd.argv[0]<<8)|cmd.argv[1]); return 0;
        }
    }
  }
  return 0;
}

/* Run commands for generic processing.
 */
 
static int sprite_run_initial_commands(struct sprite *sprite) {
  struct rom_command_reader reader={.v=sprite->cmdv,.c=sprite->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    #define u8(p) (cmd.argv[p])
    #define u16(p) ((cmd.argv[p]<<8)|cmd.argv[(p)+1])
    #define u32(p) ((cmd.argv[p]<<24)|(cmd.argv[(p)+1]<<16)|(cmd.argv[(p)+2]<<8)|cmd.argv[(p)+3])
    switch (cmd.opcode) {
      case CMD_sprite_grapplable: sprite->grapplable=1; break;
      case CMD_sprite_image: sprite->imageid=u16(0); break;
      case CMD_sprite_tile: sprite->tileid=u8(0); sprite->xform=u8(1); break;
    }
    #undef u8
    #undef u16
    #undef u32
  }
  return 0;
}

/* New.
 */

struct sprite *sprite_new(
  const struct sprite_type *type,
  struct sprites *owner,
  double x,double y,
  uint32_t arg,
  int rid
) {

  // Assert owner exists and make room in its list.
  if (!owner) return 0;
  if (owner->c>=owner->a) {
    int na=owner->a+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(owner->v,sizeof(void*)*na);
    if (!nv) return 0;
    owner->v=nv;
    owner->a=na;
  }
  
  /* Acquire resource if we're doing that, and read type from it.
   * Or assert that type was provided.
   */
  struct rom_sprite rspr={0};
  if (rid) {
    if (type) return 0; // Illegal to provide both (rid) and (type).
    const void *serial=0;
    int serialc=sauces_res_get(&serial,EGG_TID_sprite,rid);
    if (rom_sprite_decode(&rspr,serial,serialc)<0) {
      fprintf(stderr,"sprite:%d failed to decode\n",rid);
      return 0;
    }
    type=sprite_type_from_res(&rspr);
  }
  if (!type) return 0;
  
  /* Allocate, initialize simple things, and list in owner.
   */
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->owner=owner;
  sprite->x=x;
  sprite->y=y;
  sprite->arg=arg;
  sprite->rid=rid;
  sprite->cmdv=rspr.cmdv;
  sprite->cmdc=rspr.cmdc;
  owner->v[owner->c++]=sprite;
  if (!owner->hero&&(type==&sprite_type_hero)) owner->hero=sprite;
  sprite->radius=0.5;
  sprite->solidmask=(1<<NS_physics_solid)|(1<<NS_physics_water)|(1<<NS_physics_grapplable);
  
  /* Read the resource for generic initialization.
   * It's fine if it's empty.
   */
  if (sprite_run_initial_commands(sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  
  /* Run type's initializer if present.
   */
  if (type->init&&(type->init(sprite)<0)) {
    sprite_del(sprite);
    return 0;
  }
  
  return sprite;
}

/* Schedule deletion.
 */

void sprite_kill_soon(struct sprite *sprite) {
  if (!sprite||!sprite->owner) return;
  // Would be disastrous if a sprite gets deathrowed twice. Don't let it happen.
  int i=sprite->owner->deathrowc;
  while (i-->0) {
    if (sprite->owner->deathrow[i]==sprite) return;
  }
  if (sprite->owner->deathrowc>=sprite->owner->deathrowa) {
    int na=sprite->owner->deathrowa+16;
    if (na>INT_MAX/sizeof(void*)) return;
    void *nv=realloc(sprite->owner->deathrow,sizeof(void*)*na);
    if (!nv) return;
    sprite->owner->deathrow=nv;
    sprite->owner->deathrowa=na;
  }
  sprite->owner->deathrow[sprite->owner->deathrowc++]=sprite;
}
