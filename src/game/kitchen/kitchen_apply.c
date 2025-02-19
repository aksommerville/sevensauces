#include "game/sauces.h"
#include "game/session/session.h"
#include "kitchen.h"

/* Add an "add customer" to a stew.
 * The strings are in strings:ui.
 */
 
static int kitchen_add_bonus(struct stew *stew,int stringix) {
  if (stew->kcaddc>=ADD_CUSTOMER_LIMIT) return -1;
  struct kcustomer *kcustomer=stew->kcaddv+stew->kcaddc;
  stew->addreasonv[stew->kcaddc]=stringix;
  stew->kcaddc++;
  kcustomer->race=1+rand()%4;
  return 0;
}

/* Nonzero if any customer of this race is dissatisfied or dead.
 */
 
static int kitchen_race_unhappy(const struct stew *stew,int race) {
  int c=0;
  const struct kcustomer *kcustomer=stew->kcbeforev;
  int i=stew->kcbeforec;
  for (;i-->0;kcustomer++) {
    if (kcustomer->race!=race) continue;
    switch (kcustomer->outcome) {
      case KITCHEN_OUTCOME_UNFED:
      case KITCHEN_OUTCOME_DEAD:
      case KITCHEN_OUTCOME_SAD:
        return 1;
    }
    c++;
  }
  if (!c) return 1; // "none of these people exist" also counts as "unhappy"
  return 0;
}

/* Business rules.
 * All read-only, returning boolean.
 * These will not be called for empty stews, there's always at least one item.
 */
 
static int kitchen_rb_man(const struct stew *stew) {
  if (kitchen_race_unhappy(stew,NS_race_man)) return 0;
  return (stew->vegc&&(stew->vegc==stew->meatc)&&(stew->meatc==stew->candyc));
}

static int kitchen_rb_rabbit(const struct stew *stew) {
  if (kitchen_race_unhappy(stew,NS_race_rabbit)) return 0;
  return ((stew->vegc>stew->meatc)&&(stew->vegc>stew->candyc));
}

static int kitchen_rb_octopus(const struct stew *stew) {
  if (kitchen_race_unhappy(stew,NS_race_octopus)) return 0;
  return ((stew->candyc>stew->vegc)&&(stew->candyc>stew->meatc));
}

static int kitchen_rb_werewolf(const struct stew *stew) {
  if (kitchen_race_unhappy(stew,NS_race_werewolf)) return 0;
  return ((stew->meatc>stew->vegc)&&(stew->meatc>stew->candyc));
}

static int kitchen_ds_mon(const struct stew *stew) {
  int ai=1; for (;ai<stew->itemc;ai++) {
    int bi=0; for (;bi<ai;bi++) {
      if (stew->itemv[ai]==stew->itemv[bi]) return 0;
    }
  }
  return 1;
}

static int kitchen_ds_tue(const struct stew *stew) {
  const struct item *titemv[INVENTORY_SIZE];
  memcpy(titemv,stew->itemv,sizeof(void*)*stew->itemc);
  int itemc=stew->itemc;
  while (itemc>0) {
    const struct item *item=titemv[--itemc];
    int dupp=-1,i=itemc;
    while (i-->0) {
      if (titemv[i]==item) {
        dupp=i;
        break;
      }
    }
    if (dupp<0) return 0;
    itemc--;
    memmove(titemv+dupp,titemv+dupp+1,sizeof(void*)*(itemc-dupp));
  }
  return 1;
}

static int kitchen_ds_wed(const struct stew *stew) {
  int presauce=stew->helpingc-stew->helpingc_sauce;
  if (presauce>stew->kcbeforec) return 0;
  return (stew->helpingc>=stew->kcbeforec);
}

static int kitchen_ds_thu(const struct stew *stew) {
  return (stew->helpingc>=stew->kcbeforec<<1);
}

static int kitchen_ds_fri(const struct stew *stew) {
  int i=stew->itemc;
  while (i-->0) {
    const struct item *item=stew->itemv[i];
    if (item->density==1) return 0;
  }
  return 1;
}

static int kitchen_ds_sat(const struct stew *stew) {
  return (stew->itemc<=4);
}

static int kitchen_ds_sun(const struct stew *stew) {
  return stew->saucec;
}

/* Apply rules.
 * The item and initial-customer analysis must be complete.
 * We populate (kcbeforev[].outcome,kcaddv(etc),unfedc(etc),addreasonv)
 */
 
static void kitchen_apply_rules(struct stew *stew,const struct kitchen *kitchen) {
  
  /* Decide outcome for each customer.
   */
  struct kcustomer *kcustomer=stew->kcbeforev;
  int i=stew->kcbeforec;
  int served=0;
  for (;i-->0;kcustomer++) {
    
    // Unfed if there's no stew left.
    if (served>=stew->helpingc) {
      kcustomer->outcome=KITCHEN_OUTCOME_UNFED;
      stew->unfedc++;
      continue;
    }
    served++;
    
    // Dead if the stew was poisoned.
    if (stew->poisonc) {
      kcustomer->outcome=KITCHEN_OUTCOME_DEAD;
      stew->deadc++;
      continue;
    }
    
    // If there was secret sauce, normal per-person rules are suspended and they're all happy.
    if (stew->saucec) {
      kcustomer->outcome=KITCHEN_OUTCOME_HAPPY;
      stew->happyc++;
      continue;
    }
    
    // Race rules.
    kcustomer->outcome=KITCHEN_OUTCOME_SAD;
    switch (kcustomer->race) {
      case NS_race_man: if (stew->clock<stew->clock_limit) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_rabbit: if (stew->vegc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_octopus: if (stew->candyc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_werewolf: if (stew->meatc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_princess: kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
    }
    if (kcustomer->outcome==KITCHEN_OUTCOME_HAPPY) {
      stew->happyc++;
    } else {
      stew->sadc++;
    }
  }
  stew->kcrmc=stew->unfedc+stew->deadc+stew->sadc;
  
  /* First bonus, if all customers are satisfied.
   */
  if (stew->happyc==stew->kcbeforec) {
    kitchen_add_bonus(stew,42);
  }
  
  /* Second bonus for race rules per the majority race.
   * Test multiple rules if there's a tie, and only one need pass.
   * A race is disqualified if any of its members is unhappy.
   * (doesn't make sense for the Rabbit Rule to give you a bonus, if you've just poisoned all the Rabbits).
   * The race bonuses are mutually exclusive, so the order we test them doesn't matter.
   */
  if (stew->itemc) {
    int topc=stew->manc;
    if (stew->rabbitc>topc) topc=stew->rabbitc;
    if (stew->octopusc>topc) topc=stew->octopusc;
    if (stew->werewolfc>topc) topc=stew->werewolfc;
    if ((stew->manc>=topc)&&kitchen_rb_man(stew)) kitchen_add_bonus(stew,31);
    else if ((stew->rabbitc>=topc)&&kitchen_rb_rabbit(stew)) kitchen_add_bonus(stew,32);
    else if ((stew->octopusc>=topc)&&kitchen_rb_octopus(stew)) kitchen_add_bonus(stew,33);
    else if ((stew->werewolfc>=topc)&&kitchen_rb_werewolf(stew)) kitchen_add_bonus(stew,34);
  }
  
  /* Third bonus depending on day of the week.
   */
  if (stew->itemc) switch (g.session->day) {
    case 0: if (kitchen_ds_mon(stew)) kitchen_add_bonus(stew,35); break;
    case 1: if (kitchen_ds_tue(stew)) kitchen_add_bonus(stew,36); break;
    case 2: if (kitchen_ds_wed(stew)) kitchen_add_bonus(stew,37); break;
    case 3: if (kitchen_ds_thu(stew)) kitchen_add_bonus(stew,38); break;
    case 4: if (kitchen_ds_fri(stew)) kitchen_add_bonus(stew,39); break;
    case 5: if (kitchen_ds_sat(stew)) kitchen_add_bonus(stew,40); break;
    case 6: if (kitchen_ds_sun(stew)) kitchen_add_bonus(stew,41); break;
  }
}

/* Populate (itemv) and the associated summary fields.
 */
 
static void kitchen_analyze_inventory(struct stew *stew,const struct kitchen *kitchen) {
  int invp=0;
  for (;invp<INVENTORY_SIZE;invp++) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    stew->itemv[stew->itemc]=item;
    stew->invpv[stew->itemc]=invp;
    stew->itemc++;
    stew->helpingc+=item->density;
    switch (item->foodgroup) {
      case NS_foodgroup_veg: stew->vegc++; break;
      case NS_foodgroup_meat: stew->meatc++; break;
      case NS_foodgroup_candy: stew->candyc++; break;
      case NS_foodgroup_sauce: stew->saucec++; stew->helpingc_sauce+=item->density; break;
      case NS_foodgroup_poison: stew->poisonc++; break;
      default: stew->otherc++; break;
    }
  }
}

/* Populate (kcbeforev) and the associated summary fields.
 */
 
static void kitchen_analyze_customers(struct stew *stew,const struct kitchen *kitchen) {
  const struct kcustomer *src=kitchen->kcustomerv;
  int i=SESSION_CUSTOMER_LIMIT;
  for (;i-->0;src++) {
    if (!src->race) continue; // Vacant seat, skip it.
    memcpy(stew->kcbeforev+stew->kcbeforec,src,sizeof(struct kcustomer));
    stew->kcbeforec++;
    switch (src->race) {
      case NS_race_man: stew->manc++; break;
      case NS_race_rabbit: stew->rabbitc++; break;
      case NS_race_octopus: stew->octopusc++; break;
      case NS_race_werewolf: stew->werewolfc++; break;
      case NS_race_princess: stew->manc++; break;
    }
  }
}

/* Assess stew.
 */
 
void kitchen_cook(struct stew *stew,const struct kitchen *kitchen) {
  if (!stew||!kitchen) return;
  memset(stew,0,sizeof(struct stew));
  
  stew->clock=kitchen->clock;
  stew->clock_limit=clock_limit_by_day(g.session->day);
  
  kitchen_analyze_inventory(stew,kitchen);
  kitchen_analyze_customers(stew,kitchen);
  kitchen_apply_rules(stew,kitchen);
}

/* Clock limit.
 */
 
double clock_limit_by_day(int day) {
  switch (day) {
    case 0: return 30.0;
    case 1: return 28.0;
    case 2: return 26.0;
    case 3: return 24.0;
    case 4: return 22.0;
    case 5: return 20.0;
    default:return 18.0;
  }
}

/* Commit finished stew.
 */

void kitchen_commit_stew(const struct stew *stew) {
  int i;

  if (0) { //XXX dump stats
    fprintf(stderr,"%s ------------------\n",__func__);
    fprintf(stderr,"%d ITEMS:\n",stew->itemc);
    for (i=0;i<stew->itemc;i++) {
      const struct item *item=stew->itemv[i];
      const char *name=0;
      int namec=strings_get(&name,RID_strings_item_name,item-itemv);
      fprintf(stderr,"  %.*s (invp=%d)\n",namec,name,stew->invpv[i]);
    }
    fprintf(stderr,"vegc=%d meatc=%d candyc=%d saucec=%d poisonc=%d otherc=%d\n",stew->vegc,stew->meatc,stew->candyc,stew->saucec,stew->poisonc,stew->otherc);
    fprintf(stderr,"%d helpings (%d from sauce) for %d customers.\n",stew->helpingc,stew->helpingc_sauce,stew->kcbeforec);
    const struct kcustomer *kcustomer=stew->kcbeforev;
    for (i=stew->kcbeforec;i-->0;kcustomer++) {
      const char *race="???",*outcome="???";
      switch (kcustomer->race) {
        case NS_race_man: race="man"; break;
        case NS_race_rabbit: race="rabbit"; break;
        case NS_race_octopus: race="octopus"; break;
        case NS_race_werewolf: race="werewolf"; break;
        case NS_race_princess: race="princess"; break;
      }
      switch (kcustomer->outcome) {
        case KITCHEN_OUTCOME_UNFED: outcome="UNFED"; break;
        case KITCHEN_OUTCOME_DEAD: outcome="DEAD"; break;
        case KITCHEN_OUTCOME_SAD: outcome="SAD"; break;
        case KITCHEN_OUTCOME_HAPPY: outcome="HAPPY"; break;
      }
      fprintf(stderr,"  %s %s\n",race,outcome);
    }
    fprintf(stderr,"Add %d customers.\n",stew->kcaddc);
    for (i=0;i<stew->kcaddc;i++) {
      const char *msg=0;
      int msgc=strings_get(&msg,RID_strings_ui,stew->addreasonv[i]);
      fprintf(stderr,"  %.*s\n",msgc,msg);
    }
    fprintf(stderr,"Remove %d customers.\n",stew->kcrmc);
  }
  
  /* Blank inventory for everything used.
   */
  const uint8_t *invp=stew->invpv;
  for (i=stew->itemc;i-->0;invp++) {
    if (*invp>=INVENTORY_SIZE) continue;
    g.session->inventory[*invp]=0;
  }
  
  /* Acquire gold.
   */
  const int profit_per_head=2;
  int profit=(stew->kcbeforec-stew->kcrmc)*profit_per_head;
  if (profit>0) {
    if ((g.session->gold+=profit)>999) g.session->gold+=profit;
  }
  
  /* Kill or lose customers.
   */
  g.session->lossc=0;
  if (stew->kcrmc) {
    const struct kcustomer *kcustomer=stew->kcbeforev;
    for (i=stew->kcbeforec;i-->0;kcustomer++) {
      if (kcustomer->outcome==KITCHEN_OUTCOME_DEAD) {
        session_remove_customer_at(g.session,kcustomer->sesp);
      } else if ((kcustomer->outcome==KITCHEN_OUTCOME_UNFED)||(kcustomer->outcome==KITCHEN_OUTCOME_SAD)) {
        session_lose_customer_at(g.session,kcustomer->sesp);
      }
    }
    session_commit_removals(g.session);
  }
  
  /* Add customers.
   */
  const struct kcustomer *kcustomer=stew->kcaddv;
  for (i=stew->kcaddc;i-->0;kcustomer++) {
    session_add_customer(g.session,kcustomer->race);
  }
  
  //TODO We probably want more session-level bookkeeping, for achievements or just reporting.
}

/* Generate advice.
 * We check for poison and check food groups against the majority-race rule.
 * We do not check the daily specials.
 */
 
int kitchen_assess(int *severity,const struct kitchen *kitchen) {
  *severity=3; // "advise", the usual case.
  if (!kitchen) return 0;
  
  int vegc=0,meatc=0,candyc=0,saucec=0,poisonc=0,inertc=0,helpingc=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    const struct item *item=itemv+g.session->inventory[invp];
    switch (item->foodgroup) {
      case NS_foodgroup_veg: vegc++; break;
      case NS_foodgroup_meat: meatc++; break;
      case NS_foodgroup_candy: candyc++; break;
      case NS_foodgroup_sauce: saucec++; break;
      case NS_foodgroup_poison: poisonc++; break;
      default: inertc++;
    }
    helpingc+=item->density;
  }
  if (poisonc) {
    *severity=1;
    return 61;
  }
  if (inertc) {
    *severity=1;
    return 62;
  }
  if (!helpingc) return 60;
  if (helpingc<g.session->customerc) return 63;
  
  int manc=0,rabbitc=0,octopusc=0,werewolfc=0;
  int i=g.session->customerc;
  const struct customer *customer=g.session->customerv;
  for (;i-->0;customer++) {
    switch (customer->race) {
      case NS_race_man: manc++; break;
      case NS_race_rabbit: rabbitc++; break;
      case NS_race_octopus: octopusc++; break;
      case NS_race_werewolf: werewolfc++; break;
      case NS_race_princess: manc++; break;
    }
  }
  // The majority race rule is not exclusive, ties can break for either party -- that's important.
  int man=(manc>=rabbitc)&&(manc>=octopusc)&&(manc>=werewolfc);
  int rabbit=(rabbitc>=manc)&&(rabbitc>=octopusc)&&(rabbitc>=werewolfc);
  int octopus=(octopusc>=manc)&&(octopusc>=rabbitc)&&(octopusc>=werewolfc);
  int werewolf=(werewolfc>=rabbitc)&&(werewolfc>=manc)&&(werewolfc>=octopusc);
  if ((vegc==meatc)&&(meatc==candyc)&&man) { *severity=2; return 64; }
  if ((vegc>meatc)&&(vegc>candyc)&&rabbit) { *severity=2; return 64; }
  if ((candyc>vegc)&&(candyc>meatc)&&octopus) { *severity=2; return 64; }
  if ((meatc>vegc)&&(meatc>candyc)&&werewolf) { *severity=2; return 64; }
  if (werewolf) return 65;
  if (rabbit) return 66;
  if (octopus) return 67;
  if (man) return 68;
  
  return 0;
}
