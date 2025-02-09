#include "game/sauces.h"
#include "game/session/session.h"
#include "kitchen.h"

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
