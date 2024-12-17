#ifndef SAUCES_H
#define SAUCES_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

struct layer;
struct session;
struct world;
struct kitchen;

// Must agree with metadata:1, we assert at init.
#define FBW 320
#define FBH 180

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  struct layer **layerv;
  int layerc,layera;
  struct session *session; // OPTIONAL. Absent at Hello.
  struct world *world; // OPTIONAL. Present only during the diurnal phase.
  struct kitchen *kitchen; // OPTIONAL. Present only during the nocturnal phase.
  int pvinput;
} g;

#endif
