/* sauces_model.c
 * Interface to key model events, from a global scope.
 */

#include "sauces.h"
#include "game/session/session.h"
#include "game/world/world.h"
#include "game/kitchen/kitchen.h"
#include "game/layer/layer.h"

/* Layer filter: Only those we can keep when dropping the session.
 * Might only be hello.
 */
 
static int filter_layer_nonsession(struct layer *layer) {
  if (layer->type==&layer_type_hello) return 1;
  return 0;
}

/* Begin session.
 */
 
int sauces_begin_session() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) { world_del(g.world); g.world=0; }
  if (g.kitchen) { kitchen_del(g.kitchen); g.kitchen=0; }
  if (g.session) { session_del(g.session); g.session=0; }
  if (!(g.session=session_new())) return -1;
  //TODO model prep
  if (!layer_spawn(&layer_type_beginday)) return -1;
  return 0;
}

/* End session manually.
 */
 
void sauces_end_session() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) { world_del(g.world); g.world=0; }
  if (g.kitchen) { kitchen_del(g.kitchen); g.kitchen=0; }
  if (g.session) { session_del(g.session); g.session=0; }
}

/* End day.
 */
 
int sauces_end_day() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.world) {
    world_commit_to_session(g.world);
    world_del(g.world);
    g.world=0;
  }
  if (!g.session) return -1;
  if (g.kitchen) {
    kitchen_del(g.kitchen);
    g.kitchen=0;
  }
  g.session->lossc=0;
  if (session_may_proceed(g.session)) {
    if (!(g.kitchen=kitchen_new())) return -1;
    if (!layer_spawn(&layer_type_kitchen)) return -1;
  } else {
    //TODO game over
    fprintf(stderr,"*** eod game over ***\n");
    sauces_end_session();
  }
  return 0;
}

/* End night.
 */
 
int sauces_end_night() {
  layer_stack_filter(filter_layer_nonsession);
  if (g.kitchen) {
    //kitchen_commit_to_session(g.kitchen);//TODO ensure we have a sensible venue for committing, somewhere...
    kitchen_del(g.kitchen);
    g.kitchen=0;
  }
  if (session_may_proceed(g.session)) {
    g.session->day++;
    if (g.session->day>=7) {
      fprintf(stderr,"*** end of week ***\n");//TODO wrap-up layer
      fprintf(stderr,"Customers: %d\n",
        g.session->customerc+g.session->custover[0]+g.session->custover[1]+g.session->custover[2]+g.session->custover[3]
      );
    } else {
      if (!layer_spawn(&layer_type_beginday)) return -1;
    }
  } else {
    fprintf(stderr,"*** mid-week game over ***\n");//TODO
    sauces_end_session();
  }
  return 0;
}

/* Long-form item description text.
 */
 
int sauces_generate_item_description(char *dst,int dsta,uint8_t itemid,uint8_t flags) {
  int dstc=0;
  const struct item *item=itemv+itemid;
  if (item->flags) {
    #define APPEND(src,srcc) { if (dstc<=dsta-(srcc)) { memcpy(dst+dstc,src,srcc); dstc+=srcc; } }
    #define APPENDLN(src,srcc) { if (dstc<dsta-(srcc)) { memcpy(dst+dstc,src,srcc); dstc+=srcc; dst[dstc++]=0x0a; } }
    #define COLOR(ix) if (flags&ITEM_DESC_COLOR) { if (dstc<=dsta-2) { dst[dstc++]=0x7f; dst[dstc++]='0'+(ix); } }
    const struct item *item=itemv+itemid;
    const char *src=0;
    int srcc=strings_get(&src,RID_strings_item_name,itemid); // Item name.
    if (srcc>0) {
      COLOR(1)
      APPENDLN(src,srcc)
    }
    int show_density=0;
    switch (item->foodgroup) {
      case NS_foodgroup_veg: show_density=1; COLOR(3) break;
      case NS_foodgroup_meat: show_density=1; COLOR(4) break;
      case NS_foodgroup_candy: show_density=1; COLOR(5) break;
      case NS_foodgroup_inedible: COLOR(6) break;
      case NS_foodgroup_poison: COLOR(7) break;
      case NS_foodgroup_sauce: show_density=1; COLOR(8) break;
    }
    const int foodgroup_strings_base=0;
    if ((srcc=strings_get(&src,RID_strings_item_errata,foodgroup_strings_base+item->foodgroup))>0) {
      APPEND(src,srcc)
    }
    if (show_density) {
      char tmp[32];
      int tmpc=snprintf(tmp,sizeof(tmp)," x %d",item->density);
      if ((tmpc>0)&&(tmpc<sizeof(tmp))) {
        APPEND(tmp,tmpc)
      }
    }
    APPEND("\n",1)
    COLOR(2)
    if ((srcc=strings_get(&src,RID_strings_item_desc,itemid))>0) { // Item description.
      APPENDLN(src,srcc)
    }
    #undef APPEND
    #undef APPENDLN
    #undef COLOR
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

const uint32_t sauces_item_description_colorv[10]={
  0xffffffff, // Default.
  0xffff00ff, // Item name.
  0xa0c0e0ff, // Description.
  0x00ff00ff, // Vegetable.
  0xffc040ff, // Meat.
  0x00ffffff, // Candy.
  0xa0a0a0ff, // Inedible.
  0xff4030ff, // Poison.
  0xffc0ffff, // Sauce.
  0, // Reserved.
};
