#include "game/sauces.h"
#include "session.h"

// flags
#define VALID ((1<<NS_itemflag_valid))
#define CELL ((1<<NS_itemflag_valid)|(1<<NS_itemflag_cell))
#define PLANT ((1<<NS_itemflag_valid)|(1<<NS_itemflag_plant))
#define PRECIOUS ((1<<NS_itemflag_valid)|(1<<NS_itemflag_precious))
#define CELLPREC ((1<<NS_itemflag_valid)|(1<<NS_itemflag_cell)|(1<<NS_itemflag_precious))

const struct item itemv[256]={
  [NS_item_none]={0},
  
  [NS_item_rabbit ]={VALID,NS_foodgroup_meat, 1,NS_itemusage_free,1},
  [NS_item_rat    ]={VALID,NS_foodgroup_meat, 1,NS_itemusage_free,1},
  [NS_item_cheese ]={VALID,NS_foodgroup_meat, 2,NS_itemusage_drop,0},
  [NS_item_quail  ]={VALID,NS_foodgroup_meat, 3,NS_itemusage_free,1},
  [NS_item_egg    ]={VALID,NS_foodgroup_meat, 4,NS_itemusage_drop,0},
  [NS_item_goat   ]={VALID,NS_foodgroup_meat, 5,NS_itemusage_free,1},
  [NS_item_pig    ]={VALID,NS_foodgroup_meat,10,NS_itemusage_free,1},
  [NS_item_sheep  ]={VALID,NS_foodgroup_meat,15,NS_itemusage_free,1},
  [NS_item_deer   ]={VALID,NS_foodgroup_meat,20,NS_itemusage_free,1},
  [NS_item_cow    ]={VALID,NS_foodgroup_meat,25,NS_itemusage_free,1},
  [NS_item_fish   ]={VALID,NS_foodgroup_meat, 3,NS_itemusage_drop,0},
  
  [NS_item_carrot    ]={PLANT,NS_foodgroup_veg, 1,NS_itemusage_drop,3},
  [NS_item_potato    ]={PLANT,NS_foodgroup_veg, 2,NS_itemusage_drop,3},
  [NS_item_cabbage   ]={PLANT,NS_foodgroup_veg, 3,NS_itemusage_drop,3},
  [NS_item_banana    ]={PLANT,NS_foodgroup_veg, 4,NS_itemusage_drop,3},
  [NS_item_watermelon]={PLANT,NS_foodgroup_veg, 5,NS_itemusage_drop,3},
  [NS_item_pumpkin   ]={PLANT,NS_foodgroup_veg,10,NS_itemusage_drop,3},
  [NS_item_noodles   ]={VALID,NS_foodgroup_veg,15,NS_itemusage_drop,0},
  [NS_item_bread     ]={VALID,NS_foodgroup_veg,20,NS_itemusage_drop,0},
  [NS_item_bouillon  ]={VALID,NS_foodgroup_veg,25,NS_itemusage_drop,0},
  
  [NS_item_gumdrop     ]={PLANT,NS_foodgroup_candy, 1,NS_itemusage_drop,3},
  [NS_item_peppermint  ]={PLANT,NS_foodgroup_candy, 2,NS_itemusage_drop,3},
  [NS_item_licorice    ]={PLANT,NS_foodgroup_candy, 3,NS_itemusage_drop,3},
  [NS_item_chocolate   ]={PLANT,NS_foodgroup_candy, 4,NS_itemusage_drop,3},
  [NS_item_butterscotch]={PLANT,NS_foodgroup_candy, 5,NS_itemusage_drop,3},
  [NS_item_buckeye     ]={PLANT,NS_foodgroup_candy,10,NS_itemusage_drop,3},
  [NS_item_applepie    ]={VALID,NS_foodgroup_candy,15,NS_itemusage_drop,0},
  [NS_item_birthdaycake]={VALID,NS_foodgroup_candy,20,NS_itemusage_drop,0},
  [NS_item_weddingcake ]={VALID,NS_foodgroup_candy,25,NS_itemusage_drop,0},
  
  [NS_item_shovel    ]={CELLPREC,0,0,NS_itemusage_shovel  ,0},
  [NS_item_fishpole  ]={CELLPREC,0,0,NS_itemusage_fishpole,0},
  [NS_item_bow       ]={PRECIOUS,0,0,NS_itemusage_bow     ,0},
  [NS_item_sword     ]={PRECIOUS,0,0,NS_itemusage_sword   ,0},
  [NS_item_stone     ]={VALID   ,0,0,NS_itemusage_stone   ,0},
  [NS_item_apology   ]={VALID   ,0,0,NS_itemusage_letter  ,0},
  [NS_item_loveletter]={VALID   ,0,0,NS_itemusage_letter  ,0},
  [NS_item_trap      ]={CELL    ,0,0,NS_itemusage_trap    ,0},
  [NS_item_grapple   ]={PRECIOUS,0,0,NS_itemusage_grapple ,0},
  
  [NS_item_arsenic     ]={VALID,NS_foodgroup_poison,  1,NS_itemusage_drop,0},
  [NS_item_sunsauce    ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_brightsauce ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_royaljelly  ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_flashsauce  ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_heavysauce  ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_fragilesauce]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
  [NS_item_forbidden   ]={VALID,NS_foodgroup_sauce ,126,NS_itemusage_drop,0},
};
