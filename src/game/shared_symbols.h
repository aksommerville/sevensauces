/* shared_symbols.h
 * Consumed by both the game and the tools.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define NS_sys_tilesize 16
#define NS_sys_mapw 20
#define NS_sys_maph 15

#define CMD_map_forage     0x01 /* --- */
#define CMD_map_image      0x20 /* u16:imageid */
#define CMD_map_hero       0x21 /* u16:pos */
#define CMD_map_home       0x22 /* u16:pos */
#define CMD_map_condensery 0x23 /* u16:pos */
#define CMD_map_trappable  0x24 /* u8:itemid u8:odds. Odds are purely relative. */
#define CMD_map_shop       0x40 /* u16:pos u16:shopid */
#define CMD_map_dialogue   0x41 /* u16:pos u16:NS_dialogue_* */
#define CMD_map_neighbors  0x60 /* u16:left u16:right u16:up u16:down */
#define CMD_map_sprite     0x61 /* u16:pos u16:spriteid u32:reserved */
#define CMD_map_door       0x62 /* u16:pos u16:mapid u16:dstpos u16:reserved */

#define CMD_sprite_grapplable 0x01 /* --- */
#define CMD_sprite_image      0x20 /* u16:imageid */
#define CMD_sprite_tile       0x21 /* u8:tileid u8:xform */
#define CMD_sprite_type       0x22 /* u16:spritetype */

#define CMD_shop_item       0x20 /* u8:itemid u8:price */
#define CMD_shop_sellonly   0x21 /* u8:itemid u8:sellprice. Also use to override sale price, if after (item). */
#define CMD_shop_image      0x40 /* u16:imageid u8:x u8:y. Dimensions 96x64 */
#define CMD_shop_greeting   0x41 /* u16:stringsid u16:index */

#define NS_tilesheet_physics     1
#define NS_tilesheet_neighbors   0
#define NS_tilesheet_family      0
#define NS_tilesheet_weight      0
#define NS_tilesheet_item        2 /* For tiles with physics==harvest, which item? */

#define NS_physics_vacant 0
#define NS_physics_solid 1
#define NS_physics_water 2
#define NS_physics_harvest 3
#define NS_physics_diggable 4
#define NS_physics_trap 5
#define NS_physics_seed 6
#define NS_physics_grapplable 7 /* Same as 'solid' but the grapple can grap it. */

#define NS_spritetype_dummy     0
#define NS_spritetype_hero      1
#define NS_spritetype_item      2
#define NS_spritetype_arrow     3
#define NS_spritetype_daze      5
#define NS_spritetype_faun      6
#define NS_spritetype_grapple   7
#define NS_spritetype_loss      8
#define NS_spritetype_animonce  9
#define FOR_EACH_SPRITE_TYPE \
  _(dummy) \
  _(hero) \
  _(item) \
  _(arrow) \
  _(daze) \
  _(faun) \
  _(grapple) \
  _(loss) \
  _(animonce)
  
#define NS_race_man       1
#define NS_race_rabbit    2
#define NS_race_octopus   3
#define NS_race_werewolf  4
#define NS_race_princess  5

#define NS_foodgroup_inedible 0
#define NS_foodgroup_poison   1
#define NS_foodgroup_veg      2
#define NS_foodgroup_meat     3
#define NS_foodgroup_candy    4
#define NS_foodgroup_sauce    5

#define NS_itemflag_valid    0 /* This flag doesn't mean anything, but a full-zero flags byte means "unused". */
#define NS_itemflag_cell     1 /* Hero should highlight the operative cell when armed. */
#define NS_itemflag_plant    2 /* Allowed to plant. */
#define NS_itemflag_precious 3 /* Don't let her cook it without confirming. */

#define NS_itemusage_none     0 /* Not an item you can use. */
#define NS_itemusage_drop     1 /* Create an 'item' sprite that can be picked up. */
#define NS_itemusage_free     2 /* Create a 'faun' sprite that runs away. */
#define NS_itemusage_shovel   3
#define NS_itemusage_fishpole 4
#define NS_itemusage_bow      5
#define NS_itemusage_sword    6
#define NS_itemusage_stone    7
#define NS_itemusage_letter   8
#define NS_itemusage_trap     9
#define NS_itemusage_grapple 10
  
/* Items must also be listed in:
 *   src/game/session/item.c
 *   src/data/strings/*-3-item_name
 *   src/data/strings/*-4-item_desc
 *   src/data/image/5-item.png
 * If it's an animal:
 *   src/game/sprite/sprite_type_faun.c
 */
#define NS_item_none              0 /* Special blank slot at id zero. */
#define NS_item_rabbit         0x01 /* Meat... */
#define NS_item_rat            0x02
#define NS_item_cheese         0x03
#define NS_item_quail          0x04
#define NS_item_egg            0x05
#define NS_item_goat           0x06
#define NS_item_pig            0x07
#define NS_item_sheep          0x08
#define NS_item_deer           0x09
#define NS_item_cow            0x0a
#define NS_item_fish           0x0b
#define NS_item_carrot         0x20 /* Vegetables... */
#define NS_item_potato         0x21
#define NS_item_cabbage        0x22
#define NS_item_banana         0x23
#define NS_item_watermelon     0x24
#define NS_item_pumpkin        0x25
#define NS_item_noodles        0x26
#define NS_item_bread          0x27
#define NS_item_bouillon       0x28
#define NS_item_gumdrop        0x40 /* Candy... */
#define NS_item_peppermint     0x41
#define NS_item_licorice       0x42
#define NS_item_chocolate      0x43
#define NS_item_butterscotch   0x44
#define NS_item_buckeye        0x45
#define NS_item_applepie       0x46
#define NS_item_birthdaycake   0x47
#define NS_item_weddingcake    0x48
#define NS_item_shovel         0x60 /* Hardware... */
#define NS_item_fishpole       0x61
#define NS_item_bow            0x62
#define NS_item_sword          0x63
#define NS_item_stone          0x64
#define NS_item_apology        0x65
#define NS_item_loveletter     0x66
#define NS_item_trap           0x67
#define NS_item_grapple        0x68
#define NS_item_arsenic        0x80 /* Poisons and other condiments... */
#define NS_item_sunsauce       0x81
#define NS_item_brightsauce    0x82
#define NS_item_royaljelly     0x83
#define NS_item_flashsauce     0x84
#define NS_item_heavysauce     0x85
#define NS_item_fragilesauce   0x86
#define NS_item_forbidden      0x87

/* For CMD_map_dialogue, the action is one of these enums, all hard-coded.
 */
#define NS_dialogue_dailyspecial     1
#define NS_dialogue_party            2
#define NS_dialogue_recipe           3

#endif
