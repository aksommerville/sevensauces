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
    kitchen_del(g.kitchen);
    g.kitchen=0;
  }
  if (session_may_proceed(g.session)) {
    g.session->day++;
    if (g.session->day>=7) {
      fprintf(stderr,"*** end of week *** igc=%.03f\n",g.session->worldtime+g.session->kitchentime);//TODO wrap-up layer
      fprintf(stderr,"Customers: %d\n",
        g.session->customerc+g.session->custover[0]+g.session->custover[1]+g.session->custover[2]+g.session->custover[3]
      );
    } else {
      if (!layer_spawn(&layer_type_beginday)) return -1;
    }
  } else {
    fprintf(stderr,"*** mid-week game over *** igc=%.03f\n",g.session->worldtime+g.session->kitchentime);//TODO
    sauces_end_session();
  }
  return 0;
}

/* Dialogue glue.
 */
 
static void sauces_show_daily_special(int day) {
  if ((day<0)||(day>6)) return;
  struct layer *layer=layer_spawn(&layer_type_message);
  if (!layer) return;
  // For now, I'm using the same message that EOD shows as the bonus notification. Might want different phrasing.
  layer_message_setup_string(layer,RID_strings_ui,35+day);
}

static void sauces_show_party() {
  int manc=0,rabbitc=0,octopusc=0,werewolfc=0,i=g.session->customerc;
  const struct customer *customer=g.session->customerv;
  for (;i-->0;customer++) {
    switch (customer->race) {
      case NS_race_man: manc++; break;
      case NS_race_rabbit: rabbitc++; break;
      case NS_race_octopus: octopusc++; break;
      case NS_race_werewolf: werewolfc++; break;
      default: manc++; // Others count as Man. eg Princess. But also unknown -- better to call them something than nothing.
    }
  }
  struct strings_insertion insv[]={
    {'i',{.i=g.session->customerc}},
    {'i',{.i=manc}},
    {'i',{.i=rabbitc}},
    {'i',{.i=octopusc}},
    {'i',{.i=werewolfc}},
  };
  char msg[1024];
  int msgc=strings_format(msg,sizeof(msg),RID_strings_ui,56,insv,sizeof(insv)/sizeof(insv[0]));
  if ((msgc<1)||(msgc>sizeof(msg))) return;
  struct layer *layer=layer_spawn(&layer_type_message);
  if (!layer) return;
  layer_message_setup_raw(layer,msg,msgc);
}

static void sauces_show_recipe() {
  char msg[1024];
  int msgc=sauces_generate_recipe(msg,sizeof(msg));
  if ((msgc<1)||(msgc>sizeof(msg))) return;
  struct layer *layer=layer_spawn(&layer_type_message);
  if (!layer) return;
  layer_message_setup_raw(layer,msg,msgc);
}

/* Begin dialogue sequence.
 */
 
void sauces_dialogue(int dialogueid) {
  if (!g.session) return;
  switch (dialogueid) {
    case NS_dialogue_dailyspecial: sauces_show_daily_special(g.session->day); break;
    case NS_dialogue_party: sauces_show_party(); break;
    case NS_dialogue_recipe: sauces_show_recipe(); break;
    default: fprintf(stderr,"Unimplemented dialogue %d\n",dialogueid);
  }
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

/* Generate recipe model.
 * (dst) must be 32 bytes. We fill it in with (itemid0,quantity0,itemid1,quantity1,...), and return 0..16.
 * We'll recommend something that satisfies all three of the bonus rules.
 * Where decisions need to be made, lean on (day,customerc,nonce) to break ties, so we decide the same thing every time.
 * Nothing random.
 * TODO That can be reconsidered after testing. Should we deliberately miss the bonuses, and add something like "but maybe you'll want to make some adjustments."?
 * TODO This seems stable now. One full test run, followed every recommendation and got a perfect 25. Need to continue verifying of course.
 */

static const struct item *choose_item(const struct item **v,int c,int density) {
  int candidatec=0,i;
  for (i=c;i-->0;) if (v[i]->density==density) candidatec++;
  if (!candidatec) return 0;
  int choice=(g.session->nonce+g.session->day+g.session->customerc)%candidatec;
  for (i=c;i-->0;) {
    if (v[i]->density!=density) continue;
    if (!choice--) return v[i];
  }
  return 0;
}

static const struct item *choose_something_else(const struct item **v,int c,const struct item *not) {
  int candidatec=0,i;
  for (i=c;i-->0;) {
    if (v[i]==not) continue;
    if (v[i]->density!=not->density) continue;
    if (v[i]->foodgroup!=not->foodgroup) continue;
    candidatec++;
  }
  if (!candidatec) return not;
  int choice=(g.session->nonce+g.session->day+g.session->customerc)%candidatec;
  for (i=c;i-->0;) {
    if (v[i]==not) continue;
    if (v[i]->density!=not->density) continue;
    if (v[i]->foodgroup!=not->foodgroup) continue;
    if (!choice--) return v[i];
  }
  return not;
}
 
static int sauces_generate_recipe_model(uint8_t *dst) {
  int dstc=0; // bytes, not entries
  
  /* Collect all the ingredients by foodgroup.
   * We're going full generic with this.
   * In theory, the sizes should be 256, but I know there won't be more than 32 of any group because I don't want to draw that many tiles.
   * Note that this is actually constant. Could preindex it at build time if we felt like it.
   */
  #define FGLIMIT 32
  const struct item *meatv[FGLIMIT],*vegv[FGLIMIT],*candyv[FGLIMIT],*saucev[FGLIMIT];
  int meatc=0,vegc=0,candyc=0,saucec=0;
  const struct item *item=itemv;
  int i=256;
  for (;i-->0;item++) {
    switch (item->foodgroup) {
      case NS_foodgroup_meat: if (meatc<FGLIMIT) meatv[meatc++]=item; break;
      case NS_foodgroup_veg: if (vegc<FGLIMIT) vegv[vegc++]=item; break;
      case NS_foodgroup_candy: if (candyc<FGLIMIT) candyv[candyc++]=item; break;
      case NS_foodgroup_sauce: if (saucec<FGLIMIT) saucev[saucec++]=item; break;
    }
  }
  if (!meatc||!vegc||!candyc||!saucec) return 0;
  #undef FGLIMIT
  
  /* Which race is in the majority?
   * Ties are a real thing, we can recommend any of the tied races.
   */
  int manc=0,rabbitc=0,octopusc=0,werewolfc=0;
  const struct customer *customer=g.session->customerv;
  for (i=g.session->customerc;i-->0;customer++) {
    switch (customer->race) {
      case NS_race_man: case NS_race_princess: manc++; break; // Princesses count as Human for tax purposes.
      case NS_race_rabbit: rabbitc++; break;
      case NS_race_octopus: octopusc++; break;
      case NS_race_werewolf: werewolfc++; break;
    }
  }
  int racev[4];
  int racec=0;
  if ((manc>=werewolfc)&&(manc>=octopusc)&&(manc>=rabbitc)) racev[racec++]=NS_race_man;
  if ((rabbitc>=werewolfc)&&(rabbitc>=octopusc)&&(rabbitc>=manc)) racev[racec++]=NS_race_rabbit;
  if ((octopusc>=werewolfc)&&(octopusc>=rabbitc)&&(octopusc>=manc)) racev[racec++]=NS_race_octopus;
  if ((werewolfc>=octopusc)&&(werewolfc>=rabbitc)&&(werewolfc>=manc)) racev[racec++]=NS_race_werewolf;
  int race=(racec>=1)?racev[g.session->nonce%racec]:NS_race_man;
  
  /* Decide how many helpings we want total.
   * Usually that's (customerc), but on Thursdays it's double.
   */
  int helpingc=g.session->customerc;
  if (g.session->day==3) helpingc<<=1;
  
  /* How many items do we expect to need?
   * Initially assume we'll be using all 2-point ingredients and no sauce, ie ceil(helpingc/2).
   * Could use 5-pointers for the most optimistic view, but 2 is fine: It can't require more than 12 ingredients.
   * Plus, the 2-pointers are way more attainable than the 5-pointers.
   * But on Saturday, restrict to no more than 4.
   */
  int expect_itemc=(helpingc+1)/2;
  if (g.session->day==5) {
    if (expect_itemc>4) expect_itemc=4;
  }
  
  /* Divide that item count among the food groups, including sauce, to satisfy the race rule and the Sunday sauce rule.
   * It's fine to fudge a little one way or the other. (expect_itemc) assumed 2 points per ingredient but we might use other densities in reality.
   */
  int item_meatc=0,item_vegc=0,item_candyc=0,item_saucec=0;
  if (g.session->day==6) item_saucec=1;
  switch (race) {
    case NS_race_man: {
        item_meatc=item_vegc=item_candyc=(expect_itemc-item_saucec+2)/3;
      } break;
    case NS_race_rabbit:
    case NS_race_octopus:
    case NS_race_werewolf: {
        item_vegc=item_meatc=item_candyc=(expect_itemc-item_saucec)/3;
        if (manc) {
          if (!item_vegc) item_vegc=1;
          if (!item_meatc) item_meatc=1;
          if (!item_candyc) item_candyc=1;
        } else {
          if (!item_vegc&&rabbitc) item_vegc=1;
          if (!item_meatc&&werewolfc) item_meatc=1;
          if (!item_candyc&&octopusc) item_candyc=1;
        }
        switch (race) {
          case NS_race_rabbit: item_vegc++; break;
          case NS_race_octopus: item_candyc++; break;
          case NS_race_werewolf: item_meatc++; break;
        }
      } break;
  }
  
  /* If it's Twofer Tuesday, bump those desired item counts up so they're all even.
   */
  if (g.session->day==1) {
    if (item_saucec&1) item_saucec++;
    if (item_meatc&1) item_meatc++;
    if (item_vegc&1) item_vegc++;
    if (item_candyc&1) item_candyc++;
    // And mind that that might have upset the race rule, so increase again if needed.
    switch (race) {
      case NS_race_rabbit: if (item_vegc==item_meatc) item_vegc+=2; break;
      case NS_race_octopus: if (item_candyc==item_vegc) item_candyc+=2; break;
      case NS_race_werewolf: if (item_meatc==item_candyc) item_meatc+=2; break;
    }
  }
  
  /* Confirm we have no more than 16 ingredients planned.
   * Pretty sure it's not possible to be >16 at this point, but no harm checking.
   */
  if (item_vegc+item_meatc+item_candyc+item_saucec>16) return 0;
  
  /* Pick items willy-nilly, the desired count per food group.
   * Track how many helpings we've added.
   */
  const struct item *dstitemv[16];
  int dstitemc=0;
  int dsthelpingc=0;
  for (i=item_saucec;i-->0;) { // All sauces are equivalent.
    item=saucev[(g.session->nonce+g.session->day)%saucec];
    dstitemv[dstitemc++]=item;
    dsthelpingc+=item->density;
  }
  if (item_meatc) {
    int target_density=(helpingc-dsthelpingc+item_meatc+item_vegc+item_candyc-1)/(item_meatc+item_vegc+item_candyc);
    if (target_density<1) target_density=1;
    else if (target_density>5) target_density=5;
    for (i=item_meatc;i-->0;) {
      if (!(item=choose_item(meatv,meatc,target_density))) return 0;
      dstitemv[dstitemc++]=item;
      dsthelpingc+=item->density;
    }
  }
  if (item_vegc) {
    int target_density=(helpingc-dsthelpingc+item_vegc+item_candyc-1)/(item_vegc+item_candyc);
    if (target_density<1) target_density=1;
    else if (target_density>5) target_density=5;
    for (i=item_vegc;i-->0;) {
      if (!(item=choose_item(vegv,vegc,target_density))) return 0;
      dstitemv[dstitemc++]=item;
      dsthelpingc+=item->density;
    }
  }
  if (item_candyc) {
    int target_density=(helpingc-dsthelpingc+item_candyc-1)/item_candyc;
    if (target_density<1) target_density=1;
    else if (target_density>5) target_density=5;
    for (i=item_candyc;i-->0;) {
      if (!(item=choose_item(candyv,candyc,target_density))) return 0;
      dstitemv[dstitemc++]=item;
      dsthelpingc+=item->density;
    }
  }
  
  /* If we're below the target helping count (not possible?), add a sauce.
   * One sauce can feed the whole party.
   */
  if (dsthelpingc<helpingc) {
    if (g.session->day==1) { // ...or on Tuesday, add two sauces.
      if (dstitemc>14) return 0;
      item=saucev[(g.session->nonce+g.session->day)%saucec];
      dstitemv[dstitemc++]=item;
      dstitemv[dstitemc++]=item;
      dsthelpingc+=item->density<<1;
    } else {
      if (dstitemc>15) return 0;
      item=saucev[(g.session->nonce+g.session->day)%saucec];
      dstitemv[dstitemc++]=item;
      dsthelpingc+=item->density;
    }
  }
  
  /* On Monday, all ingredients must be unique.
   * It should always be possible to find a second item of a given foodgroup and density,
   * since Monday has no other rules in play and always just 4 customers.
   * This algorithm might break when there's three or more of something, but again, Monday, can't happen.
   */
  if (g.session->day==0) {
    int ai=dstitemc; while (ai-->1) {
      const struct item *aitem=dstitemv[ai];
      int bi=ai; while (bi-->0) {
        const struct item *bitem=dstitemv[bi];
        if (aitem==bitem) {
          switch (aitem->foodgroup) {
            case NS_foodgroup_veg: dstitemv[bi]=choose_something_else(vegv,vegc,aitem); break;
            case NS_foodgroup_meat: dstitemv[bi]=choose_something_else(meatv,meatc,aitem); break;
            case NS_foodgroup_candy: dstitemv[bi]=choose_something_else(candyv,candyc,aitem); break;
          }
        }
      }
    }
  }
  
  /* On Tuesday, all ingredients must be in pairs.
   * Run down the list, and duplicate each ingredient we find into slots occupied by the same foodgroup and density.
   */
  if (g.session->day==1) {
    int ai=dstitemc; while (ai-->1) {
      const struct item *aitem=dstitemv[ai];
      int bi=ai; while (bi-->0) {
        const struct item *bitem=dstitemv[bi];
        if ((bitem->foodgroup==aitem->foodgroup)&&(bitem->density==aitem->density)) {
          dstitemv[bi]=aitem;
        }
      }
    }
  }
  
  /* On Wednesday, we must hit the helpings target exactly, but sauce counts special.
   * If we're over, reduce densities until we fit.
   * Note that this can't break any race rules: Those operate only on count, and we're operating only on density.
   */
  if ((g.session->day==2)&&(dsthelpingc>helpingc)) {
    int sauce_helpingc=0;
    for (i=dstitemc;i-->0;) {
      if (dstitemv[i]->foodgroup==NS_foodgroup_sauce) sauce_helpingc+=dstitemv[i]->density;
    }
    int dropc=dsthelpingc-sauce_helpingc-helpingc;
    int i=0;
    for (;(i<dstitemc)&&(dropc>0);i++) {
      if (dstitemv[i]->foodgroup==NS_foodgroup_sauce) continue;
      int ndensity=dstitemv[i]->density-dropc;
      if (ndensity<1) ndensity=1;
      if (ndensity==dstitemv[i]->density) continue;
      item=0;
      switch (dstitemv[i]->foodgroup) {
        case NS_foodgroup_meat: item=choose_item(meatv,meatc,ndensity); break;
        case NS_foodgroup_veg: item=choose_item(vegv,vegc,ndensity); break;
        case NS_foodgroup_candy: item=choose_item(candyv,candyc,ndensity); break;
      }
      if (!item) continue;
      dropc-=dstitemv[i]->density-item->density;
      dstitemv[i]=item;
    }
  }
  
  /* Sort items by address, then encode as (itemid,count,...)
   */
  int lo=0,hi=dstitemc-1,d=1;
  while (lo<hi) {
    int first,last,i,done=1;
    if (d==1) { first=hi; last=lo; d=-1; }
    else { first=lo; last=hi; d=1; }
    for (i=first;i!=last;i+=d) {
      int cmp;
           if (dstitemv[i]<dstitemv[i+d]) cmp=-1;
      else if (dstitemv[i]>dstitemv[i+d]) cmp=1;
      else cmp=0;
      if (cmp==d) {
        done=0;
        const struct item *tmp=dstitemv[i];
        dstitemv[i]=dstitemv[i+d];
        dstitemv[i+d]=tmp;
      }
    }
    if (done) break;
  }
  for (i=0;i<dstitemc;) {
    int q=1;
    while ((i+q<dstitemc)&&(dstitemv[i]==dstitemv[i+q])) q++;
    dst[dstc++]=(dstitemv[i]-itemv);
    dst[dstc++]=q;
    i+=q;
  }
  return dstc>>1; // >>1 because (dstc) is in bytes and we return entries.
}

/* Generate recipe text.
 */
 
int sauces_generate_recipe(char *dst,int dsta) {

  /* If there's no customers, say something entirely different.
   * This very much is possible, and it's five-alarm urgent when it happens.
   */
  if (g.session->customerc<1) {
    const char *src;
    int srcc=strings_get(&src,RID_strings_ui,57);
    if (srcc<=dsta) memcpy(dst,src,srcc);
    return srcc;
  }

  /* Generate the recipe as an array of (itemid,quantity,...).
   * That's the hundreds of messy lines just above.
   */
  uint8_t model[32];
  int modelc=sauces_generate_recipe_model(model);
  if ((modelc<1)||(modelc>INVENTORY_SIZE)) return 0;
  
  /* A little lead message, then lines of (indent,quantity,itemname), then a little valediction.
   */
  int dstc=0;
  const char *src;
  int srcc=strings_get(&src,RID_strings_ui,58);
  if (dstc>=dsta-srcc) return 0;
  memcpy(dst+dstc,src,srcc); dstc+=srcc;
  dst[dstc++]=0x0a;
  int modelp=0;
  for (;modelp<modelc;modelp++) {
    if (dstc>dsta-6) return 0;
    uint8_t itemid=model[modelp<<1];
    uint8_t quantity=model[(modelp<<1)+1];
    dst[dstc++]=0x20;
    dst[dstc++]=0x20;
    if (quantity>=100) dst[dstc++]='0'+quantity/100;
    if (quantity>=10) dst[dstc++]='0'+(quantity/10)%10;
    dst[dstc++]='0'+quantity%10;
    dst[dstc++]=0x20;
    srcc=strings_get(&src,RID_strings_item_name,itemid);
    if (dstc>=dsta-srcc) return 0;
    memcpy(dst+dstc,src,srcc); dstc+=srcc;
    dst[dstc++]=0x0a;
  }
  srcc=strings_get(&src,RID_strings_ui,59);
  if (dstc>dsta-srcc) return 0;
  memcpy(dst+dstc,src,srcc); dstc+=srcc;
  return dstc;
}
