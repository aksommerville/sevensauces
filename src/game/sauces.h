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
  struct font *font6;
  struct font *font7;
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

void sauces_dialogue(int dialogueid);

/* Generate the long text description for pause menu and kitchen, optionally with color.
 * The default colors assume a dark background.
 */
int sauces_generate_item_description(char *dst,int dsta,uint8_t itemid,uint8_t flags);
#define ITEM_DESC_COLOR 0x01 /* We may start lines with 0x7f followed by '0'..'9' to indicate color. Only immediately after an LF. */
extern const uint32_t sauces_item_description_colorv[10];

/* Generate a recipe in the global language, based on session's customers and inventory.
 */
int sauces_generate_recipe(char *dst,int dsta);

/* Generate a decorative sprite and sound effect.
 */
void sauces_splash(double x,double y);

#endif
