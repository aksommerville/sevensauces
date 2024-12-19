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
  struct layer *layer_focus; // OPTIONAL, WEAK.
  struct session *session; // OPTIONAL. Absent at Hello.
  struct world *world; // OPTIONAL. Present only during the diurnal phase.
  struct kitchen *kitchen; // OPTIONAL. Present only during the nocturnal phase.
  int pvinput;
} g;

int sauces_res_get(void *dstpp,int tid,int rid);

/* Recreates (g.session) and pushes the appropriate layer.
 */
int sauces_begin_session();

void sauces_end_session();

/* Pops the world layer, creates the kitchen model, and pushes a kitchen layer.
 * This is wildly unsafe during a world update.
 * If a sprite controller or something wants to end day, instead force the clock to zero and let layer handle it.
 */
int sauces_end_day();
int sauces_end_night();

int sauces_format_item_string(char *dst,int dsta,int fmt_rid,int fmt_ix,uint8_t itemid);

#endif
