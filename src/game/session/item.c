#include "game/sauces.h"
#include "session.h"

// flags
#define VALID ((1<<NS_itemflag_valid))
#define CELL ((1<<NS_itemflag_valid)|(1<<NS_itemflag_cell))

const struct item itemv[256]={
  [NS_item_none]={0},
  
  [NS_item_rabbit ]={VALID,NS_foodgroup_meat, 1,NS_itemusage_free},
  [NS_item_rat    ]={VALID,NS_foodgroup_meat, 1,NS_itemusage_free},
  [NS_item_cheese ]={VALID,NS_foodgroup_meat, 2,NS_itemusage_drop},
  [NS_item_quail  ]={VALID,NS_foodgroup_meat, 3,NS_itemusage_free},
  [NS_item_egg    ]={VALID,NS_foodgroup_meat, 4,NS_itemusage_drop},
  [NS_item_goat   ]={VALID,NS_foodgroup_meat, 5,NS_itemusage_free},
  [NS_item_pig    ]={VALID,NS_foodgroup_meat,10,NS_itemusage_free},
  [NS_item_sheep  ]={VALID,NS_foodgroup_meat,15,NS_itemusage_free},
  [NS_item_deer   ]={VALID,NS_foodgroup_meat,20,NS_itemusage_free},
  [NS_item_cow    ]={VALID,NS_foodgroup_meat,25,NS_itemusage_free},
  
  [NS_item_carrot    ]={VALID,NS_foodgroup_veg, 1,NS_itemusage_drop},
  [NS_item_potato    ]={VALID,NS_foodgroup_veg, 2,NS_itemusage_drop},
  [NS_item_cabbage   ]={VALID,NS_foodgroup_veg, 3,NS_itemusage_drop},
  [NS_item_banana    ]={VALID,NS_foodgroup_veg, 4,NS_itemusage_drop},
  [NS_item_watermelon]={VALID,NS_foodgroup_veg, 5,NS_itemusage_drop},
  [NS_item_pumpkin   ]={VALID,NS_foodgroup_veg,10,NS_itemusage_drop},
  [NS_item_noodles   ]={VALID,NS_foodgroup_veg,15,NS_itemusage_drop},
  [NS_item_bread     ]={VALID,NS_foodgroup_veg,20,NS_itemusage_drop},
  [NS_item_bouillon  ]={VALID,NS_foodgroup_veg,25,NS_itemusage_drop},
  
  [NS_item_gumdrop     ]={VALID,NS_foodgroup_candy, 1,NS_itemusage_drop},
  [NS_item_peppermint  ]={VALID,NS_foodgroup_candy, 2,NS_itemusage_drop},
  [NS_item_licorice    ]={VALID,NS_foodgroup_candy, 3,NS_itemusage_drop},
  [NS_item_chocolate   ]={VALID,NS_foodgroup_candy, 4,NS_itemusage_drop},
  [NS_item_butterscotch]={VALID,NS_foodgroup_candy, 5,NS_itemusage_drop},
  [NS_item_buckeye     ]={VALID,NS_foodgroup_candy,10,NS_itemusage_drop},
  [NS_item_applepie    ]={VALID,NS_foodgroup_candy,15,NS_itemusage_drop},
  [NS_item_birthdaycake]={VALID,NS_foodgroup_candy,20,NS_itemusage_drop},
  [NS_item_weddingcake ]={VALID,NS_foodgroup_candy,25,NS_itemusage_drop},
  
  [NS_item_shovel    ]={CELL ,0,0,NS_itemusage_shovel},
  [NS_item_fishpole  ]={CELL ,0,0,NS_itemusage_fishpole},
  [NS_item_bow       ]={VALID,0,0,NS_itemusage_bow},
  [NS_item_sword     ]={VALID,0,0,NS_itemusage_sword},
  [NS_item_stone     ]={VALID,0,0,NS_itemusage_stone},
  [NS_item_apology   ]={VALID,0,0,NS_itemusage_letter},
  [NS_item_loveletter]={VALID,0,0,NS_itemusage_letter},
  [NS_item_trap      ]={CELL ,0,0,NS_itemusage_trap},
};
