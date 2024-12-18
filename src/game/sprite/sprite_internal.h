#ifndef SPRITE_INTERNAL_H
#define SPRITE_INTERNAL_H

#include "game/sauces.h"
#include "opt/rom/rom.h"
#include "sprite.h"

struct sprites {
  struct sprite **v;
  int c,a;
  struct sprite **deathrow; // WEAK, also listed in (v).
  int deathrowc,deathrowa;
  struct sprite *hero; // WEAK
};

#endif
