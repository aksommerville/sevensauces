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
  switch (g.session->day) {//TODO daily time limits: test and tweak
    case 0: stew->clock_limit=30.0; break;
    case 1: stew->clock_limit=27.0; break;
    case 2: stew->clock_limit=24.0; break;
    case 3: stew->clock_limit=21.0; break;
    case 4: stew->clock_limit=18.0; break;
    case 5: stew->clock_limit=15.0; break;
    default:stew->clock_limit=12.0; break;
  }
  
  kitchen_analyze_inventory(stew,kitchen);
  kitchen_analyze_customers(stew,kitchen);
  kitchen_apply_rules(stew,kitchen);
}

/* Commit finished stew.
 */

void kitchen_commit_stew(const struct stew *stew) {
  int i;

  if (1) { //XXX dump stats
  fprintf(stderr,"%s\n",__func__);//TODO
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

#if 0 /*XXX Second attempt. Everything works great, but I need a single point of contact to run all the rules without committing ------------------------------------------*/
/* Populate (outcome) for all occupied kcustomer.
 */
 
void kitchen_apply(struct kitchen *kitchen) {
  fprintf(stderr,"%s...\n",__func__);

  /* Determine stew's characteristics.
   */
  int helpingc=0; // How many helpings, ie sum of densities.
  int vegc=0,meatc=0,candyc=0,poisonc=0,saucec=0,otherc=0; // Natural ingredient counts.
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    helpingc+=item->density;
    switch (item->foodgroup) {
      case NS_foodgroup_poison: poisonc++; break;
      case NS_foodgroup_veg: vegc++; break;
      case NS_foodgroup_meat: meatc++; break;
      case NS_foodgroup_candy: candyc++; break;
      case NS_foodgroup_sauce: saucec++; break;
      default: otherc++;
    }
  }
  fprintf(stderr,
    "helpingc=%d vegc=%d meatc=%d candyc=%d poisonc=%d saucec=%d otherc=%d\n",
    helpingc,vegc,meatc,candyc,poisonc,saucec,otherc
  );
  
  /* Threshold for man's rule: He's impatient, satisfaction is based on the clock.
   */
  double clock_limit;
  switch (g.session->day) {
    case  0: clock_limit=10.0; break;//TODO test and tweak...
    case  1: clock_limit= 9.0; break;
    case  2: clock_limit= 8.0; break;
    case  3: clock_limit= 7.0; break;
    case  4: clock_limit= 6.0; break;
    case  5: clock_limit= 5.0; break;
    default: clock_limit= 4.0; break;
  }
  
  /* Check each kcustomer in turn.
   */
  int served=0;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  int i=SESSION_CUSTOMER_LIMIT;
  for (;i-->0;kcustomer++) {
    if (!kcustomer->race) continue; // Vacant seat.
    
    /* First, if we're out of stew, they are UNFED.
     * Importantly: Poison doesn't affect them, because they didn't eat any of it.
     */
    if (served>=helpingc) {
      fprintf(stderr,"kcustomer race=%d UNFED\n",kcustomer->race);
      kcustomer->outcome=KITCHEN_OUTCOME_UNFED;
      continue;
    }
    served++;
    
    /* Second, if the stew is poisoned, everybody that eats it dies.
     * I don't imagine there's ever a case where you'd want to this.
     * It's in the rules just to allow players to fuck themselves over.
     */
    if (poisonc) {
      fprintf(stderr,"kcustomer race=%d DEAD\n",kcustomer->race);
      kcustomer->outcome=KITCHEN_OUTCOME_DEAD;
      continue;
    }
    
    /* If there was any secret sauce in the stew, everybody likes it.
     */
    if (saucec) {
      fprintf(stderr,"kcustomer race=%d HAPPY due to sauce\n",kcustomer->race);
      kcustomer->outcome=KITCHEN_OUTCOME_HAPPY;
      continue;
    }
    
    /* Most cases, decide between SAD and HAPPY based on a race-specific rule.
     */
    kcustomer->outcome=KITCHEN_OUTCOME_SAD;
    switch (kcustomer->race) {
      case NS_race_man: if (kitchen->clock<=clock_limit) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_rabbit: if (vegc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_octopus: if (candyc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_werewolf: if (meatc) kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
      case NS_race_princess: kcustomer->outcome=KITCHEN_OUTCOME_HAPPY; break;
    }
    fprintf(stderr,"kcustomer race=%d %s due to race rule\n",kcustomer->race,(kcustomer->outcome==KITCHEN_OUTCOME_HAPPY)?"HAPPY":"SAD");
  }
}

/* Helpers for the business rules below.
 */
 
static void kitchen_count_natural_foodgroups(int *vegc,int *meatc,int *candyc,const struct kitchen *kitchen) {
  *vegc=*meatc=*candyc=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    switch (item->foodgroup) {
      case NS_foodgroup_veg: (*vegc)++; break;
      case NS_foodgroup_meat: (*meatc)++; break;
      case NS_foodgroup_candy: (*candyc)++; break;
    }
  }
}

/* Per-race and per-day bonus rules.
 * Return nonzero to award the bonus.
 */
 
static int kitchen_rr_man(const struct kitchen *kitchen) {
  int vegc,meatc,candyc;
  kitchen_count_natural_foodgroups(&vegc,&meatc,&candyc,kitchen);
  fprintf(stderr,"%s vegc=%d meatc=%d candyc=%d\n",__func__,vegc,meatc,candyc);
  if (!vegc) return 0;
  if ((vegc==meatc)&&(meatc==candyc)) return 1;
  return 0;
}
 
static int kitchen_rr_rabbit(const struct kitchen *kitchen) {
  int vegc,meatc,candyc;
  kitchen_count_natural_foodgroups(&vegc,&meatc,&candyc,kitchen);
  fprintf(stderr,"%s vegc=%d meatc=%d candyc=%d\n",__func__,vegc,meatc,candyc);
  if ((vegc>meatc)&&(vegc>candyc)) return 1;
  return 0;
}
 
static int kitchen_rr_octopus(const struct kitchen *kitchen) {
  int vegc,meatc,candyc;
  kitchen_count_natural_foodgroups(&vegc,&meatc,&candyc,kitchen);
  fprintf(stderr,"%s vegc=%d meatc=%d candyc=%d\n",__func__,vegc,meatc,candyc);
  if ((candyc>vegc)&&(candyc>meatc)) return 1;
  return 0;
}
 
static int kitchen_rr_werewolf(const struct kitchen *kitchen) {
  int vegc,meatc,candyc;
  kitchen_count_natural_foodgroups(&vegc,&meatc,&candyc,kitchen);
  fprintf(stderr,"%s vegc=%d meatc=%d candyc=%d\n",__func__,vegc,meatc,candyc);
  if ((meatc>vegc)&&(meatc>candyc)) return 1;
  return 0;
}
 
// Each ingredient must be unique.
static int kitchen_ds_mon(const struct kitchen *kitchen) {
  int c=0;
  int ai=1; for (;ai<INVENTORY_SIZE;ai++) {
    if (!kitchen->selected[ai]) continue;
    c++;
    int bi=0; for (;bi<ai;bi++) {
      if (!kitchen->selected[bi]) continue;
      if (g.session->inventory[ai]==g.session->inventory[bi]) return 0;
    }
  }
  if (!c) return 0;
  return 1;
}

// All ingredients must be in pairs.
static int kitchen_ds_tue(const struct kitchen *kitchen) {
  uint8_t itemidv[INVENTORY_SIZE];
  int itemidc=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    itemidv[itemidc++]=g.session->inventory[invp];
  }
  if (!itemidc) return 0;
  while (itemidc>0) {
    uint8_t itemid=itemidv[--itemidc];
    int dupp=-1,i=itemidc;
    while (i-->0) {
      if (itemidv[i]==itemid) {
        dupp=i;
        break;
      }
    }
    if (dupp<0) return 0;
    itemidc--;
    memmove(itemidv+dupp,itemidv+dupp+1,itemidc-dupp);
  }
  return 1;
}

// No leftovers.
static int kitchen_ds_wed(const struct kitchen *kitchen) {
  /* Count helpings, accounting for density.
   * Keep the sauce separate: Extra sauce doesn't disqualify you from this rule.
   */
  int helpingc=0,saucec=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    if (item->foodgroup==NS_foodgroup_sauce) saucec+=item->density;
    else helpingc+=item->density;
  }
  /* Count customers in the kitchen.
   * The session's customer count has already been reduced, but the rule must operate against the initial count.
   */
  int customerc=0;
  const struct kcustomer *kcustomer=kitchen->kcustomerv;
  int i=SESSION_CUSTOMER_LIMIT;
  for (;i-->0;kcustomer++) {
    if (kcustomer->race) customerc++;
  }
  /* And finally, enforce it.
   * It's a little weird due to sauce.
   */
  if (helpingc>customerc) return 0;
  if (helpingc+saucec>=customerc) return 1;
  return 0;
}

// Leftovers at least double.
static int kitchen_ds_thu(const struct kitchen *kitchen) {
  /* It works a lot like the Wednesday rule but simpler, because sauce is always counted.
   */
  int helpingc=0;
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    helpingc+=item->density;
  }
  int customerc=0;
  const struct kcustomer *kcustomer=kitchen->kcustomerv;
  int i=SESSION_CUSTOMER_LIMIT;
  for (;i-->0;kcustomer++) {
    if (kcustomer->race) customerc++;
  }
  if (helpingc>=customerc<<1) return 1;
  return 0;
}

// No single-point ingredients. (also require at least one food ingredient).
static int kitchen_ds_fri(const struct kitchen *kitchen) {
  int invp=INVENTORY_SIZE,valid=0;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    if (item->density==1) return 0;
    if (item->density>1) valid=1;
  }
  return valid;
}

// Max 4 ingredients.
static int kitchen_ds_sat(const struct kitchen *kitchen) {
  int invp=INVENTORY_SIZE,ingrc=0;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    ingrc++;
  }
  if (ingrc>4) return 0;
  if (!ingrc) return 0;
  return 1;
}

// Must include secret sauce.
static int kitchen_ds_sun(const struct kitchen *kitchen) {
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    if (item->foodgroup==NS_foodgroup_sauce) return 1;
  }
  return 0;
}

/* Write results out to session.
 * Remove inventory, remove customers, add customers.
 TODO The logic is sound, but this needs rekajiggered to aid reporting in real time.
 */

void kitchen_commit_to_session(struct kitchen *kitchen) {
  fprintf(stderr,"%s...\n",__func__);
  
  /* Remove all DEAD, UNFED, and UNHAPPY customers.
   * UNFED and UNHAPPY produce a loss -- they can be apologized to tomorrow.
   * We're going to need counts by race next, so gather those here too.
   */
  int manc=0,rabbitc=0,octopusc=0,werewolfc=0;
  int all_happy=1;
  g.session->lossc=0;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  int i=SESSION_CUSTOMER_LIMIT;
  for (;i-->0;kcustomer++) {
    if (!kcustomer->race) continue;
    switch (kcustomer->race) {
      case NS_race_princess: // pass. Princesses count as man for census purposes, tho strictly speaking they are a different species.
      case NS_race_man: manc++; break;
      case NS_race_rabbit: rabbitc++; break;
      case NS_race_octopus: octopusc++; break;
      case NS_race_werewolf: werewolfc++; break;
    }
    if (kcustomer->outcome==KITCHEN_OUTCOME_DEAD) {
      fprintf(stderr,"...DEAD race=%d sesp=%d\n",kcustomer->race,kcustomer->sesp);
      session_remove_customer_at(g.session,kcustomer->sesp);
      all_happy=0;
    } else if ((kcustomer->outcome==KITCHEN_OUTCOME_UNFED)||(kcustomer->outcome==KITCHEN_OUTCOME_SAD)) {
      fprintf(stderr,"...SAD or UNFED race=%d sesp=%d\n",kcustomer->race,kcustomer->sesp);
      session_lose_customer_at(g.session,kcustomer->sesp);
      all_happy=0;
    }
  }
  session_commit_removals(g.session);
  
  /* First bonus: A new customer if everybody is happy.
   */
  if (all_happy) {
    fprintf(stderr,"...+1 ALL HAPPY\n");
    session_add_customer(g.session,0);
  }
  
  /* Second bonus: A rule depending on which race has the most members. (including dead ones)
   * In a tie, the rules for all tied races are all checked and only one needs to pass.
   * (so in a 4-way tie you automatically win this; the majority rules cover all possibilities).
   */
  int racemax=manc;
  if (rabbitc>racemax) racemax=rabbitc;
  if (octopusc>racemax) racemax=octopusc;
  if (werewolfc>racemax) racemax=werewolfc;
  int race_rule=0;
  if ((manc>=racemax)&&kitchen_rr_man(kitchen)) race_rule=1;
  else if ((rabbitc>=racemax)&&kitchen_rr_rabbit(kitchen)) race_rule=1;
  else if ((octopusc>=racemax)&&kitchen_rr_octopus(kitchen)) race_rule=1;
  else if ((werewolfc>=racemax)&&kitchen_rr_werewolf(kitchen)) race_rule=1;
  if (race_rule) {
    fprintf(stderr,"...+1 RACE RULE\n");
    session_add_customer(g.session,0);
  }
  
  /* Third bonus: A rule depending on the day of the week.
   */
  int daily_special=0;
  switch (g.session->day) {
    case 0: daily_special=kitchen_ds_mon(kitchen); break;
    case 1: daily_special=kitchen_ds_tue(kitchen); break;
    case 2: daily_special=kitchen_ds_wed(kitchen); break;
    case 3: daily_special=kitchen_ds_thu(kitchen); break;
    case 4: daily_special=kitchen_ds_fri(kitchen); break;
    case 5: daily_special=kitchen_ds_sat(kitchen); break;
    case 6: daily_special=kitchen_ds_sun(kitchen); break;
  }
  if (daily_special) {
    fprintf(stderr,"...+1 DAILY SPECIAL (%d)\n",g.session->day);
    session_add_customer(g.session,0);
  }

  /* Clear inventory slots for cooked items.
   */
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    g.session->inventory[invp]=0;
  }
}
#endif

#if 0 /*XXX First attempt, with large dining rooms.---------------------------------------------------------------------------------------------------------- */
/* Nonzero if every selected vegetable is unique.
 */
 
static int all_vegetables_unique(const struct kitchen *kitchen) {
  uint8_t itemidv[INVENTORY_SIZE];
  int itemidc=0;
  int i=INVENTORY_SIZE;
  while (i-->0) {
    if (!kitchen->selected[i]) continue;
    uint8_t itemid=g.session->inventory[i];
    if (itemv[itemid].foodgroup!=NS_foodgroup_veg) continue;
    int ii=itemidc; while (ii-->0) {
      if (itemidv[ii]==itemid) return 0;
    }
    itemidv[itemidc++]=itemid;
  }
  return 1;
}

/* XXX TEMP: Dump customer's outcome.
 */
 
static void kitchen_report_customer(const struct kitchen *kitchen,const struct kcustomer *kcustomer) {
  const char *name="???";
  switch (kcustomer->race) {
    case NS_race_man: name="man"; break;
    case NS_race_rabbit: name="rabbit"; break;
    case NS_race_octopus: name="octopus"; break;
    case NS_race_werewolf: name="werewolf"; break;
    case NS_race_princess: name="princess"; break;
  }
  const char *desc="???";
  switch (kcustomer->outcome) {
    case KITCHEN_OUTCOME_UNFED: desc="UNFED"; break;
    case KITCHEN_OUTCOME_DEAD: desc="DEAD"; break;
    case KITCHEN_OUTCOME_UNHAPPY: desc="UNHAPPY"; break;
    case KITCHEN_OUTCOME_SATISFIED: desc="SATISFIED"; break;
    case KITCHEN_OUTCOME_ECSTATIC: desc="ECSTATIC"; break;
  }
  fprintf(stderr,">> outcome for %s (sesp=%d): %s\n",name,kcustomer->sesp,desc);
}

/* Apply kitchen, main entry point.
 */
 
void kitchen_apply(struct kitchen *kitchen) {
  
  /* Gather the stew's characteristics.
   */
  int vegc=0,meatc=0,candyc=0,saucec=0,poisonc=0; // With density applied.
  int vic=0,mic=0,cic=0,sic=0,pic=0,ingredientc=0; // Straight ingredient counts.
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    ingredientc++;
    uint8_t itemid=g.session->inventory[invp];
    const struct item *item=itemv+itemid;
    switch (item->foodgroup) {
      case NS_foodgroup_veg: vegc+=item->density; vic++; break;
      case NS_foodgroup_meat: meatc+=item->density; mic++; break;
      case NS_foodgroup_candy: candyc+=item->density; cic++; break;
      case NS_foodgroup_sauce: saucec+=item->density; sic++; break;
      case NS_foodgroup_poison: poisonc+=item->density; pic++; break;
      // Non-food items like shovels don't participate. It's ok to cook to your shovel.
    }
  }
  int helpingc=vegc+meatc+candyc+saucec+poisonc; // Yes, poison does count for helpings.
  int leftoverc=helpingc-g.session->customerc; // Negative if we're short.
  if (saucec) { // Roll sauce into the 3 primary food groups; beyond this point, sauce doesn't count.
    saucec/=3;
    vegc+=saucec;
    meatc+=saucec;
    candyc+=saucec;
  }
  
  /**
  fprintf(stderr,"customerc: %d\n",g.session->customerc);
  fprintf(stderr,"helpingc: %d\n",helpingc);
  fprintf(stderr," - vegc: %d (%d)\n",vegc,vic);
  fprintf(stderr," - meatc: %d (%d)\n",meatc,mic);
  fprintf(stderr," - candyc: %d (%d)\n",candyc,cic);
  fprintf(stderr," - poisonc: %d (%d)\n",poisonc,pic);
  fprintf(stderr,"%s secret sauce. (%d,%d)\n",saucec?"Includes":"No",saucec,sic);
  fprintf(stderr,"ingredientc: %d\n",ingredientc);
  fprintf(stderr,"leftoverc: %d\n",leftoverc);
  /**/
  
  /* Determine outcome for each customer.
   */
  int helpings_remaining=helpingc;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  int i=kitchen->size;
  for (;i-->0;kcustomer++) {
    if (!kcustomer->race) continue;
    if (helpings_remaining<1) {
      kcustomer->outcome=KITCHEN_OUTCOME_UNFED;
    } else if (poisonc) {
      helpings_remaining--;
      kcustomer->outcome=KITCHEN_OUTCOME_DEAD;
    } else {
      helpings_remaining--;
      switch (kcustomer->race) {

        case NS_race_man: {
            if (!vegc||!meatc) { // Require at least one each of vegetable and meat. (secret sauces count)
              kcustomer->outcome=KITCHEN_OUTCOME_UNHAPPY;
            } else { // If (veg,meat,candy) by natural ingredient count are within 2 of each other, we're ecstatic. Man likes a balanced meal.
              int hi=vic,lo=vic;
              if (mic>hi) hi=mic; else if (mic<lo) lo=mic;
              if (cic>hi) hi=cic; else if (cic<lo) lo=cic;
              const int thresh=2;
              if (hi-lo>thresh) kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
              else kcustomer->outcome=KITCHEN_OUTCOME_ECSTATIC;
            }
          } break;

        case NS_race_rabbit: {
            if (vic<mic) { // Requires at least as much vegetable as meat, by natural ingredient count.
              kcustomer->outcome=KITCHEN_OUTCOME_UNHAPPY;
            } else if (all_vegetables_unique(kitchen)) {
              kcustomer->outcome=KITCHEN_OUTCOME_ECSTATIC;
            } else {
              kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
            }
          } break;

        case NS_race_octopus: {
            if (!candyc) { // Require candy, but secret sauce does count.
              kcustomer->outcome=KITCHEN_OUTCOME_UNHAPPY;
            } else if ((cic>vic)&&(cic>mic)) { // Then if there's more natural candy than the other groups, ecstatic.
              kcustomer->outcome=KITCHEN_OUTCOME_ECSTATIC;
            } else {
              kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
            }
          } break;

        case NS_race_werewolf: {
            if (leftoverc>0) { // Ecstatic if you make too much.
              kcustomer->outcome=KITCHEN_OUTCOME_ECSTATIC;
            } else { // Otherwise satified. He's a dog, he's not picky.
              kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
            }
          } break;

        case NS_race_princess: {
            // The princess is never ecstatic nor unhappy, as long as she gets fed and not poisoned.
            // One would expect a princess to be super picky, but it's opposite,
            // because her presence is a reward for having beaten some side quest.
            kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
          } break;

        default: kcustomer->outcome=KITCHEN_OUTCOME_DEAD; // Don't let it be zero. But this must never happen.
      }
      // And finally: If sauce was present, nobody can be unhappy. Bump them to satisfied.
      if (saucec&&(kcustomer->outcome==KITCHEN_OUTCOME_UNHAPPY)) kcustomer->outcome=KITCHEN_OUTCOME_SATISFIED;
    }
    //kitchen_report_customer(kitchen,kcustomer);
  }
}

/* Commit to session, last chance to persist anything.
 */
 
void kitchen_commit_to_session(struct kitchen *kitchen) {
  g.session->lossc=0;
  
  /* Remove selected items from inventory.
   */
  int invp=INVENTORY_SIZE;
  while (invp-->0) {
    if (!kitchen->selected[invp]) continue;
    g.session->inventory[invp]=0;
  }
  
  /* Check each customer and apply its outcome:
   *  - DEAD: Remove.
   *  - UNFED: Remove and add loss.
   *  - UNHAPPY: Remove and add loss.
   *  - SATISFIED: Do nothing.
   *  - ECSTATIC: Duplicate.
   */
  int count_by_outcome[6]={0};
  int all_satisfied=1,i=kitchen->size;
  struct kcustomer *kcustomer=kitchen->kcustomerv;
  for (;i-->0;kcustomer++) {
    if (!kcustomer->race) continue;
    if ((kcustomer->outcome>=0)&&(kcustomer->outcome<6)) count_by_outcome[kcustomer->outcome]++;
    switch (kcustomer->outcome) {
    
      case KITCHEN_OUTCOME_DEAD: {
          all_satisfied=0;
          session_remove_customer_at(g.session,kcustomer->sesp);
        } break;
        
      case KITCHEN_OUTCOME_UNFED:
      case KITCHEN_OUTCOME_UNHAPPY: {
          all_satisfied=0;
          session_lose_customer_at(g.session,kcustomer->sesp);
        } break;
        
      // No action for SATISFIED. They'll come back tomorrow.
        
      case KITCHEN_OUTCOME_ECSTATIC: {
          session_add_customer(g.session,kcustomer->race);
        } break;
    }
  }
  session_commit_removals(g.session);
  
  fprintf(stderr,"Applied kitchen...\n");
  if (count_by_outcome[KITCHEN_OUTCOME_UNFED])     fprintf(stderr,"    UNFED: %4d\n",count_by_outcome[KITCHEN_OUTCOME_UNFED]);
  if (count_by_outcome[KITCHEN_OUTCOME_DEAD])      fprintf(stderr,"     DEAD: %4d\n",count_by_outcome[KITCHEN_OUTCOME_DEAD]);
  if (count_by_outcome[KITCHEN_OUTCOME_UNHAPPY])   fprintf(stderr,"  UNHAPPY: %4d\n",count_by_outcome[KITCHEN_OUTCOME_UNHAPPY]);
  if (count_by_outcome[KITCHEN_OUTCOME_SATISFIED]) fprintf(stderr,"SATISFIED: %4d\n",count_by_outcome[KITCHEN_OUTCOME_SATISFIED]);
  if (count_by_outcome[KITCHEN_OUTCOME_ECSTATIC])  fprintf(stderr," ECSTATIC: %4d\n",count_by_outcome[KITCHEN_OUTCOME_ECSTATIC]);
  
  /* If every customer is SATISFIED or ECSTATIC, you get a bonus customer of random race.
   */
  if (all_satisfied) {
    fprintf(stderr,"New random customer.\n");
    session_add_customer(g.session,1+rand()%4);
  }
}
#endif
