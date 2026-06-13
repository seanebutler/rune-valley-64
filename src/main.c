/* ============================================================
 *  RUNE VALLEY 64
 *  An original N64 homage to old-school MMO skilling games,
 *  built with libdragon. All art, audio and code are original.
 *
 *  Controls:
 *    Stick/D-Pad  walk    (R toggles run)
 *    A            interact (chop / mine / fish / cook / bank / attack)
 *    B            inventory (A: use item, C-down: drop)
 *    C-right      skills panel
 *    Start        help
 *    L            toggle music
 * ============================================================ */

#include <libdragon.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FONT_ID        1
#define TILE           16
#define MAP_W          48
#define MAP_H          36
#define SCREEN_W       320
#define SCREEN_H       240
#define TICK_MS        600          /* one game tick, OSRS style */
#define INV_SIZE       28
#define MAX_GOB        16     /* overworld goblins, or dungeon skeletons + boss */
#define MAX_XPDROP     8
#define CHAT_LINES     6

#define WALK_SPEED     (TILE / 0.6f)        /* 1 tile per tick  */
#define RUN_SPEED      (2 * TILE / 0.6f)    /* 2 tiles per tick */

/* combat lunge/recoil: a short positional jab toward (attacker) or knockback
   away from (defender) the foe, eased back to rest over a handful of frames */
#define BUMP_FRAMES    9
#define BUMP_LUNGE     6.0f
#define BUMP_RECOIL    5.0f

/* ------------------------------------------------------------ terrain & map */

enum { TER_GRASS, TER_PATH, TER_SAND, TER_WATER, TER_BRIDGE, TER_WALL, TER_FLOOR,
       TER_CAVE };
enum { OBJ_NONE, OBJ_TREE, OBJ_OAK, OBJ_STUMP, OBJ_ROCK_COPPER, OBJ_ROCK_TIN,
       OBJ_ROCK_IRON, OBJ_ROCK_EMPTY, OBJ_FISH, OBJ_BOOTH, OBJ_FIRE,
       OBJ_ESSENCE, OBJ_ALTAR_AIR, OBJ_ALTAR_FIRE,
       OBJ_FURNACE, OBJ_ANVIL, OBJ_CHEF, OBJ_STAIRS_DOWN, OBJ_STAIRS_UP,
       OBJ_SHOP_GENERAL, OBJ_SHOP_WEAPON, OBJ_SHOP_ARMOR, OBJ_SHOP_MAGIC,
       OBJ_KNIGHT, OBJ_FENCE, OBJ_TUTOR,
       OBJ_ALTAR_WATER, OBJ_ALTAR_EARTH, OBJ_ALTAR_LAW, OBJ_ALTAR_CHAOS,
       OBJ_ROCK_MITHRIL, OBJ_TANNER, OBJ_ROCK_COAL };

enum { MAP_OVERWORLD, MAP_DUNGEON };

static const char *map_rows[MAP_H] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT~~~~TTTTTT",
    "T.....................................~~~~.....T",
    "T.................wwwwwwwwwwwww.......~~~~.....T",
    "T...E.E...........wfffffffffffw.......~~~~.....T",
    "T.................wfffffffffffw.......~~~~.....T",
    "T....A..W..D......wfffffffffffw.......~~~~.....T",
    "T.................wfBBBBBBBBBfw.......~~~~.....T",
    "T.................wfffffffffffw.......~~~~.....T",
    "T.................wwwwwpppwwwww.......~~~~.....T",
    "T......................ppp............~~~~.....T",
    "T....T.................ppp.H..........~~~~.....T",
    "T.............T........ppp..LLLLLLLLL.~~~~.....T",
    "T......................p@p..L.M...M.L.~~~~.....T",
    "T..T...................pppYZ....M...L.~~~~.....T",
    "T............O.........ppp..L.M...M.L.~~~~.....T",
    "T.......pppppppppppppppppp..L...M...L.~~~~.....T",
    "T.......pp.............ppp..LLLLLLLLL.~~~~.....T",
    "T.......pp.............ppp......T.....~~~~.G...T",
    "T.......pp.............ppppppppppppppp====ppp..T",
    "T.......pp..U.V.......................~~~~.....T",
    "T.......pp..........................ss~~~~..G..T",
    "T.......pp..........................ss~~~~.....T",
    "T.......pp..........................ssF~~~..T..T",
    "T.....C.pp...N......................ss~~~~.G...T",
    "T.......pp..........................ss~~~~.....T",
    "T.......C...N.......................ss~~~~.....T",
    "T.................1...2...3...4.....ss~~~~.....T",
    "T...................................ssF~~~.....T",
    "T......I..I..m.k.O.k................ss~~~~.....T",
    "T.....X......K......................ss~~~~.....T",
    "T.............................O.....ss~~~~..R..T",
    "T...................................ssF~~~.....T",
    "T...................T...............ss~~~~.....T",
    "T.....T...............................~~~~.....T",
    "T......................................~~~~....T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT~~~~TTTTTT",
};

static uint8_t terrain[MAP_H][MAP_W];
static uint8_t object [MAP_H][MAP_W];
static uint8_t obj_orig[MAP_H][MAP_W];
static int16_t obj_timer[MAP_H][MAP_W];

/* ------------------------------------------------------------ skills */

/* new skills go at the END so existing save indices (0-12) never shift */
enum { SK_ATT, SK_STR, SK_DEF, SK_HP, SK_WC, SK_MINE, SK_FISH, SK_FM, SK_COOK,
       SK_PRAY, SK_RC, SK_SMITH, SK_MAGIC, SK_RANGED, SK_FLETCH, SK_CRAFT,
       NUM_SKILLS };

static const char *skill_names[NUM_SKILLS] = {
    "Attack", "Strength", "Defence", "Hitpoints", "Woodcutting",
    "Mining", "Fishing", "Firemaking", "Cooking", "Prayer", "Runecraft",
    "Smithing", "Magic", "Ranged", "Fletching", "Crafting"
};

static int32_t xp[NUM_SKILLS];        /* stored as xp * 10 */
static int32_t xp_table[100];         /* xp*10 needed for level index    */

static void init_xp_table(void)
{
    double points = 0;
    xp_table[0] = 0; xp_table[1] = 0;
    for (int lvl = 2; lvl <= 99; lvl++) {
        points += floor((lvl - 1) + 300.0 * pow(2.0, (lvl - 1) / 7.0));
        xp_table[lvl] = (int32_t)(floor(points / 4.0)) * 10;
    }
}

static int level_of(int sk)
{
    for (int l = 99; l >= 1; l--)
        if (xp[sk] >= xp_table[l]) return l;
    return 1;
}

/* ------------------------------------------------------------ items */

enum { IT_NONE, IT_LOGS, IT_OAK_LOGS, IT_COPPER, IT_TIN, IT_IRON,
       IT_RAW_SHRIMP, IT_SHRIMP, IT_BURNT, IT_AXE, IT_PICK, IT_NET,
       IT_TINDER, IT_BONES, IT_ESSENCE, IT_AIR_RUNE, IT_FIRE_RUNE,
       IT_BRONZE_BAR, IT_IRON_BAR, IT_HAMMER, IT_BRONZE_SWORD, IT_IRON_SWORD,
       IT_IRON_AXE, IT_IRON_PICK,
       IT_BRONZE_HELM, IT_IRON_HELM, IT_BRONZE_SHIELD, IT_IRON_SHIELD,
       IT_BRONZE_BODY, IT_IRON_BODY,
       IT_STAFF, IT_WIZ_HAT, IT_WIZ_ROBE, IT_COINS,
       IT_STEEL_SWORD, IT_STEEL_HELM, IT_STEEL_SHIELD, IT_STEEL_BODY,
       IT_MITH_SWORD,  IT_MITH_HELM,  IT_MITH_SHIELD,  IT_MITH_BODY,
       IT_RUNE_SWORD,  IT_RUNE_HELM,  IT_RUNE_SHIELD,  IT_RUNE_BODY,
       IT_BANE, IT_DRAGONSTONE, IT_DRAGONFIRE,
       IT_WATER_RUNE, IT_EARTH_RUNE, IT_LAW_RUNE, IT_CHAOS_RUNE,
       IT_ROD, IT_LOBSTER_POT, IT_HARPOON,
       IT_RAW_TROUT, IT_TROUT, IT_RAW_LOBSTER, IT_LOBSTER,
       IT_RAW_SWORDFISH, IT_SWORDFISH,
       IT_KNIFE, IT_ARROW_SHAFT, IT_BRONZE_TIPS, IT_IRON_TIPS,
       IT_BRONZE_ARROW, IT_IRON_ARROW, IT_SHORTBOW, IT_OAK_BOW,
       IT_MITH_ORE, IT_MITH_BAR, IT_MITH_TIPS, IT_MITH_ARROW,
       IT_COWHIDE, IT_LEATHER, IT_DRAGON_HIDE, IT_DRAGON_LEATHER,
       IT_NEEDLE, IT_THREAD,
       IT_LEATHER_COIF, IT_LEATHER_BODY, IT_DHIDE_COIF, IT_DHIDE_BODY,
       IT_COAL, IT_STEEL_BAR,
       NUM_ITEMS };

/* worn equipment slots; SLOT_NONE = item is not equippable */
enum { SLOT_NONE, SLOT_WEAPON, SLOT_SHIELD, SLOT_HELM, SLOT_BODY };
#define NUM_SLOTS 4

/* equip fields default to 0 (= SLOT_NONE, no bonus) for non-gear items;
   stackable items share one inventory slot with a quantity */
typedef struct {
    const char *name; int spr; int heal; bool tool;
    int slot; int atk; int str; int def; int eqlvl; bool stackable; int mag;
    int rng;   /* ranged bonus (bows + arrows); 0 for everything else */
} iteminfo_t;

/* ------------------------------------------------------------ sprites */

enum {
    SPR_GRASS_A, SPR_GRASS_B, SPR_PATH, SPR_SAND, SPR_WATER_A, SPR_WATER_B,
    SPR_FLOOR, SPR_WALL, SPR_BRIDGE, SPR_CAVE,
    SPR_TREE, SPR_OAK, SPR_STUMP, SPR_ROCK_C, SPR_ROCK_T, SPR_ROCK_I,
    SPR_ROCK_E, SPR_FISH_A, SPR_FISH_B, SPR_BOOTH, SPR_FIRE_A, SPR_FIRE_B,
    SPR_STAIRS_DOWN, SPR_STAIRS_UP,
    SPR_PL_DOWN_A, SPR_PL_DOWN_B, SPR_PL_UP_A, SPR_PL_UP_B,
    SPR_PL_SIDE_A, SPR_PL_SIDE_B, SPR_PL_SIDE_LA, SPR_PL_SIDE_LB,
    SPR_GOB_A, SPR_GOB_B, SPR_SKEL_A, SPR_SKEL_B, SPR_BOSS,
    SPR_WIGHT_A, SPR_WIGHT_B, SPR_DEMON,
    SPR_I_LOGS, SPR_I_OAK_LOGS, SPR_I_COPPER, SPR_I_TIN, SPR_I_IRON,
    SPR_I_RAW_SHRIMP, SPR_I_SHRIMP, SPR_I_BURNT, SPR_I_AXE, SPR_I_PICK,
    SPR_I_NET, SPR_I_TINDER, SPR_I_BONES,
    SPR_ESSENCE, SPR_ALTAR_AIR, SPR_ALTAR_FIRE,
    SPR_I_ESSENCE, SPR_I_AIR_RUNE, SPR_I_FIRE_RUNE,
    SPR_BOTA_DOWN_A, SPR_BOTA_DOWN_B, SPR_BOTA_UP_A, SPR_BOTA_UP_B,
    SPR_BOTA_SIDE_A, SPR_BOTA_SIDE_B, SPR_BOTA_SIDE_LA, SPR_BOTA_SIDE_LB,
    SPR_BOTB_DOWN_A, SPR_BOTB_DOWN_B, SPR_BOTB_UP_A, SPR_BOTB_UP_B,
    SPR_BOTB_SIDE_A, SPR_BOTB_SIDE_B, SPR_BOTB_SIDE_LA, SPR_BOTB_SIDE_LB,
    SPR_BOTC_DOWN_A, SPR_BOTC_DOWN_B, SPR_BOTC_UP_A, SPR_BOTC_UP_B,
    SPR_BOTC_SIDE_A, SPR_BOTC_SIDE_B, SPR_BOTC_SIDE_LA, SPR_BOTC_SIDE_LB,
    SPR_LOGO_0, SPR_LOGO_1, SPR_LOGO_2, SPR_LOGO_3,
    SPR_LOGO_4, SPR_LOGO_5, SPR_LOGO_6, SPR_LOGO_7,
    SPR_LOGO_8, SPR_LOGO_9, SPR_LOGO_10, SPR_LOGO_11,
    SPR_LOGO_12, SPR_LOGO_13, SPR_LOGO_14, SPR_LOGO_15,
    SPR_FURNACE, SPR_ANVIL, SPR_CHEF, SPR_KNIGHT,
    SPR_I_BANE, SPR_EQ_BANE_WEP_D, SPR_EQ_BANE_WEP_U, SPR_EQ_BANE_WEP_S, SPR_EQ_BANE_WEP_SL,
    SPR_I_BRONZE_BAR, SPR_I_IRON_BAR, SPR_I_HAMMER,
    SPR_I_BRONZE_SWORD, SPR_I_IRON_SWORD, SPR_I_IRON_AXE, SPR_I_IRON_PICK,
    SPR_I_BRONZE_HELM, SPR_I_IRON_HELM, SPR_I_BRONZE_SHIELD, SPR_I_IRON_SHIELD,
    SPR_I_BRONZE_BODY, SPR_I_IRON_BODY,
    SPR_I_STAFF, SPR_I_WIZ_HAT, SPR_I_WIZ_ROBE, SPR_I_COINS,
    SPR_STALL_GENERAL, SPR_STALL_WEAPON, SPR_STALL_ARMOR, SPR_STALL_MAGIC,
    SPR_BOLT_AIR, SPR_BOLT_FIRE,
    /* worn-equipment overlays: 4 facings (down,up,side,side-left) per slot */
    SPR_EQ_BZ_HELM_D, SPR_EQ_BZ_HELM_U, SPR_EQ_BZ_HELM_S, SPR_EQ_BZ_HELM_SL,
    SPR_EQ_BZ_BODY_D, SPR_EQ_BZ_BODY_U, SPR_EQ_BZ_BODY_S, SPR_EQ_BZ_BODY_SL,
    SPR_EQ_BZ_WEP_D,  SPR_EQ_BZ_WEP_U,  SPR_EQ_BZ_WEP_S,  SPR_EQ_BZ_WEP_SL,
    SPR_EQ_BZ_SHD_D,  SPR_EQ_BZ_SHD_U,  SPR_EQ_BZ_SHD_S,  SPR_EQ_BZ_SHD_SL,
    SPR_EQ_IR_HELM_D, SPR_EQ_IR_HELM_U, SPR_EQ_IR_HELM_S, SPR_EQ_IR_HELM_SL,
    SPR_EQ_IR_BODY_D, SPR_EQ_IR_BODY_U, SPR_EQ_IR_BODY_S, SPR_EQ_IR_BODY_SL,
    SPR_EQ_IR_WEP_D,  SPR_EQ_IR_WEP_U,  SPR_EQ_IR_WEP_S,  SPR_EQ_IR_WEP_SL,
    SPR_EQ_IR_SHD_D,  SPR_EQ_IR_SHD_U,  SPR_EQ_IR_SHD_S,  SPR_EQ_IR_SHD_SL,
    SPR_EQ_HAT_D,  SPR_EQ_HAT_U,  SPR_EQ_HAT_S,  SPR_EQ_HAT_SL,
    SPR_EQ_ROBE_D, SPR_EQ_ROBE_U, SPR_EQ_ROBE_S, SPR_EQ_ROBE_SL,
    SPR_EQ_STAFF_D,SPR_EQ_STAFF_U,SPR_EQ_STAFF_S,SPR_EQ_STAFF_SL,
    SPR_I_STEEL_SWORD, SPR_I_STEEL_HELM, SPR_I_STEEL_SHIELD, SPR_I_STEEL_BODY,
    SPR_I_MITH_SWORD,  SPR_I_MITH_HELM,  SPR_I_MITH_SHIELD,  SPR_I_MITH_BODY,
    SPR_I_RUNE_SWORD,  SPR_I_RUNE_HELM,  SPR_I_RUNE_SHIELD,  SPR_I_RUNE_BODY,
    SPR_EQ_ST_HELM_D, SPR_EQ_ST_HELM_U, SPR_EQ_ST_HELM_S, SPR_EQ_ST_HELM_SL,
    SPR_EQ_ST_BODY_D, SPR_EQ_ST_BODY_U, SPR_EQ_ST_BODY_S, SPR_EQ_ST_BODY_SL,
    SPR_EQ_ST_WEP_D,  SPR_EQ_ST_WEP_U,  SPR_EQ_ST_WEP_S,  SPR_EQ_ST_WEP_SL,
    SPR_EQ_ST_SHD_D,  SPR_EQ_ST_SHD_U,  SPR_EQ_ST_SHD_S,  SPR_EQ_ST_SHD_SL,
    SPR_EQ_MI_HELM_D, SPR_EQ_MI_HELM_U, SPR_EQ_MI_HELM_S, SPR_EQ_MI_HELM_SL,
    SPR_EQ_MI_BODY_D, SPR_EQ_MI_BODY_U, SPR_EQ_MI_BODY_S, SPR_EQ_MI_BODY_SL,
    SPR_EQ_MI_WEP_D,  SPR_EQ_MI_WEP_U,  SPR_EQ_MI_WEP_S,  SPR_EQ_MI_WEP_SL,
    SPR_EQ_MI_SHD_D,  SPR_EQ_MI_SHD_U,  SPR_EQ_MI_SHD_S,  SPR_EQ_MI_SHD_SL,
    SPR_EQ_RU_HELM_D, SPR_EQ_RU_HELM_U, SPR_EQ_RU_HELM_S, SPR_EQ_RU_HELM_SL,
    SPR_EQ_RU_BODY_D, SPR_EQ_RU_BODY_U, SPR_EQ_RU_BODY_S, SPR_EQ_RU_BODY_SL,
    SPR_EQ_RU_WEP_D,  SPR_EQ_RU_WEP_U,  SPR_EQ_RU_WEP_S,  SPR_EQ_RU_WEP_SL,
    SPR_EQ_RU_SHD_D,  SPR_EQ_RU_SHD_U,  SPR_EQ_RU_SHD_S,  SPR_EQ_RU_SHD_SL,
    SPR_COW_A, SPR_COW_B, SPR_FENCE, SPR_TUTOR,
    SPR_WHELP_A, SPR_WHELP_B, SPR_DRAGON, SPR_I_DRAGONSTONE,
    SPR_I_DRAGONFIRE,
    SPR_EQ_DF_WEP_D, SPR_EQ_DF_WEP_U, SPR_EQ_DF_WEP_S, SPR_EQ_DF_WEP_SL,
    SPR_I_WATER_RUNE, SPR_I_EARTH_RUNE, SPR_I_LAW_RUNE,
    SPR_ALTAR_WATER, SPR_ALTAR_EARTH, SPR_ALTAR_LAW,
    SPR_I_CHAOS_RUNE, SPR_ALTAR_CHAOS,
    SPR_I_ROD, SPR_I_LOBSTER_POT, SPR_I_HARPOON,
    SPR_I_RAW_TROUT, SPR_I_TROUT, SPR_I_RAW_LOBSTER, SPR_I_LOBSTER,
    SPR_I_RAW_SWORDFISH, SPR_I_SWORDFISH,
    SPR_I_KNIFE, SPR_I_ARROW_SHAFT, SPR_I_BRONZE_TIPS, SPR_I_IRON_TIPS,
    SPR_I_BRONZE_ARROW, SPR_I_IRON_ARROW, SPR_I_SHORTBOW, SPR_I_OAK_BOW,
    SPR_ROCK_M, SPR_ROCK_COAL, SPR_TANNER,
    SPR_I_COAL, SPR_I_STEEL_BAR,
    SPR_I_MITH_ORE, SPR_I_MITH_BAR, SPR_I_MITH_TIPS, SPR_I_MITH_ARROW,
    SPR_I_COWHIDE, SPR_I_LEATHER, SPR_I_DRAGON_HIDE, SPR_I_DRAGON_LEATHER,
    SPR_I_NEEDLE, SPR_I_THREAD,
    SPR_I_LEATHER_COIF, SPR_I_LEATHER_BODY, SPR_I_DHIDE_COIF, SPR_I_DHIDE_BODY,
    SPR_EQ_LE_HELM_D, SPR_EQ_LE_HELM_U, SPR_EQ_LE_HELM_S, SPR_EQ_LE_HELM_SL,
    SPR_EQ_LE_BODY_D, SPR_EQ_LE_BODY_U, SPR_EQ_LE_BODY_S, SPR_EQ_LE_BODY_SL,
    SPR_EQ_DH_HELM_D, SPR_EQ_DH_HELM_U, SPR_EQ_DH_HELM_S, SPR_EQ_DH_HELM_SL,
    SPR_EQ_DH_BODY_D, SPR_EQ_DH_BODY_U, SPR_EQ_DH_BODY_S, SPR_EQ_DH_BODY_SL,
    NUM_SPR
};

static const char *spr_files[NUM_SPR] = {
    "tile_grass_a", "tile_grass_b", "tile_path", "tile_sand", "tile_water_a",
    "tile_water_b", "tile_floor", "tile_wall", "tile_bridge", "tile_cave",
    "obj_tree", "obj_oak", "obj_stump", "obj_rock_copper", "obj_rock_tin",
    "obj_rock_iron", "obj_rock_empty", "obj_fish_a", "obj_fish_b", "obj_booth",
    "obj_fire_a", "obj_fire_b",
    "obj_stairs_down", "obj_stairs_up",
    "pl_down_a", "pl_down_b", "pl_up_a", "pl_up_b", "pl_side_a", "pl_side_b",
    "pl_side_la", "pl_side_lb",
    "goblin_a", "goblin_b", "skeleton_a", "skeleton_b", "boss",
    "wight_a", "wight_b", "demon",
    "item_logs", "item_oak_logs", "item_copper_ore", "item_tin_ore",
    "item_iron_ore", "item_raw_shrimp", "item_shrimp", "item_burnt_shrimp",
    "item_axe", "item_pickaxe", "item_net", "item_tinderbox", "item_bones",
    "obj_essence_rock", "obj_altar_air", "obj_altar_fire",
    "item_essence", "item_air_rune", "item_fire_rune",
    "bota_down_a", "bota_down_b", "bota_up_a", "bota_up_b",
    "bota_side_a", "bota_side_b", "bota_side_la", "bota_side_lb",
    "botb_down_a", "botb_down_b", "botb_up_a", "botb_up_b",
    "botb_side_a", "botb_side_b", "botb_side_la", "botb_side_lb",
    "botc_down_a", "botc_down_b", "botc_up_a", "botc_up_b",
    "botc_side_a", "botc_side_b", "botc_side_la", "botc_side_lb",
    "ui_logo_00", "ui_logo_01", "ui_logo_02", "ui_logo_03",
    "ui_logo_04", "ui_logo_05", "ui_logo_06", "ui_logo_07",
    "ui_logo_08", "ui_logo_09", "ui_logo_10", "ui_logo_11",
    "ui_logo_12", "ui_logo_13", "ui_logo_14", "ui_logo_15",
    "obj_furnace", "obj_anvil", "chef_down_a", "knight_down_a",
    "item_bane", "eq_bane_wep_d", "eq_bane_wep_u", "eq_bane_wep_s", "eq_bane_wep_sl",
    "item_bronze_bar", "item_iron_bar", "item_hammer",
    "item_bronze_sword", "item_iron_sword", "item_iron_axe", "item_iron_pick",
    "item_bronze_helm", "item_iron_helm", "item_bronze_shield",
    "item_iron_shield", "item_bronze_body", "item_iron_body",
    "item_staff", "item_wizard_hat", "item_wizard_robe", "item_coins",
    "obj_stall_general", "obj_stall_weapon", "obj_stall_armor", "obj_stall_magic",
    "obj_bolt_air", "obj_bolt_fire",
    "eq_bz_helm_d", "eq_bz_helm_u", "eq_bz_helm_s", "eq_bz_helm_sl",
    "eq_bz_body_d", "eq_bz_body_u", "eq_bz_body_s", "eq_bz_body_sl",
    "eq_bz_wep_d",  "eq_bz_wep_u",  "eq_bz_wep_s",  "eq_bz_wep_sl",
    "eq_bz_shd_d",  "eq_bz_shd_u",  "eq_bz_shd_s",  "eq_bz_shd_sl",
    "eq_ir_helm_d", "eq_ir_helm_u", "eq_ir_helm_s", "eq_ir_helm_sl",
    "eq_ir_body_d", "eq_ir_body_u", "eq_ir_body_s", "eq_ir_body_sl",
    "eq_ir_wep_d",  "eq_ir_wep_u",  "eq_ir_wep_s",  "eq_ir_wep_sl",
    "eq_ir_shd_d",  "eq_ir_shd_u",  "eq_ir_shd_s",  "eq_ir_shd_sl",
    "eq_hat_d",  "eq_hat_u",  "eq_hat_s",  "eq_hat_sl",
    "eq_robe_d", "eq_robe_u", "eq_robe_s", "eq_robe_sl",
    "eq_staff_d","eq_staff_u","eq_staff_s","eq_staff_sl",
    "item_st_sword", "item_st_helm", "item_st_shield", "item_st_body",
    "item_mi_sword", "item_mi_helm", "item_mi_shield", "item_mi_body",
    "item_ru_sword", "item_ru_helm", "item_ru_shield", "item_ru_body",
    "eq_st_helm_d", "eq_st_helm_u", "eq_st_helm_s", "eq_st_helm_sl",
    "eq_st_body_d", "eq_st_body_u", "eq_st_body_s", "eq_st_body_sl",
    "eq_st_wep_d",  "eq_st_wep_u",  "eq_st_wep_s",  "eq_st_wep_sl",
    "eq_st_shd_d",  "eq_st_shd_u",  "eq_st_shd_s",  "eq_st_shd_sl",
    "eq_mi_helm_d", "eq_mi_helm_u", "eq_mi_helm_s", "eq_mi_helm_sl",
    "eq_mi_body_d", "eq_mi_body_u", "eq_mi_body_s", "eq_mi_body_sl",
    "eq_mi_wep_d",  "eq_mi_wep_u",  "eq_mi_wep_s",  "eq_mi_wep_sl",
    "eq_mi_shd_d",  "eq_mi_shd_u",  "eq_mi_shd_s",  "eq_mi_shd_sl",
    "eq_ru_helm_d", "eq_ru_helm_u", "eq_ru_helm_s", "eq_ru_helm_sl",
    "eq_ru_body_d", "eq_ru_body_u", "eq_ru_body_s", "eq_ru_body_sl",
    "eq_ru_wep_d",  "eq_ru_wep_u",  "eq_ru_wep_s",  "eq_ru_wep_sl",
    "eq_ru_shd_d",  "eq_ru_shd_u",  "eq_ru_shd_s",  "eq_ru_shd_sl",
    "mob_cow_a", "mob_cow_b", "obj_fence", "obj_tutor",
    "mob_whelp_a", "mob_whelp_b", "mob_dragon", "item_dragonstone",
    "item_dragonfire",
    "eq_df_wep_d", "eq_df_wep_u", "eq_df_wep_s", "eq_df_wep_sl",
    "item_water_rune", "item_earth_rune", "item_law_rune",
    "obj_altar_water", "obj_altar_earth", "obj_altar_law",
    "item_chaos_rune", "obj_altar_chaos",
    "item_rod", "item_lobster_pot", "item_harpoon",
    "item_raw_trout", "item_trout", "item_raw_lobster", "item_lobster",
    "item_raw_swordfish", "item_swordfish",
    "item_knife", "item_arrow_shaft", "item_bronze_tips", "item_iron_tips",
    "item_bronze_arrow", "item_iron_arrow", "item_shortbow", "item_oak_bow",
    "obj_rock_mithril", "obj_rock_coal", "tanner_down_a",
    "item_coal", "item_steel_bar",
    "item_mith_ore", "item_mith_bar", "item_mith_tips", "item_mith_arrow",
    "item_cowhide", "item_leather", "item_dragon_hide", "item_dragon_leather",
    "item_needle", "item_thread",
    "item_leather_coif", "item_leather_body", "item_dhide_coif", "item_dhide_body",
    "eq_le_helm_d", "eq_le_helm_u", "eq_le_helm_s", "eq_le_helm_sl",
    "eq_le_body_d", "eq_le_body_u", "eq_le_body_s", "eq_le_body_sl",
    "eq_dh_helm_d", "eq_dh_helm_u", "eq_dh_helm_s", "eq_dh_helm_sl",
    "eq_dh_body_d", "eq_dh_body_u", "eq_dh_body_s", "eq_dh_body_sl"
};

static sprite_t *spr[NUM_SPR];

static const iteminfo_t iteminfo[NUM_ITEMS] = {
    [IT_NONE]       = { "",            0,                0, false },
    [IT_LOGS]       = { "Logs",        SPR_I_LOGS,       0, false },
    [IT_OAK_LOGS]   = { "Oak logs",    SPR_I_OAK_LOGS,   0, false },
    [IT_COPPER]     = { "Copper ore",  SPR_I_COPPER,     0, false },
    [IT_TIN]        = { "Tin ore",     SPR_I_TIN,        0, false },
    [IT_IRON]       = { "Iron ore",    SPR_I_IRON,       0, false },
    [IT_RAW_SHRIMP] = { "Raw shrimp",  SPR_I_RAW_SHRIMP, 0, false },
    [IT_SHRIMP]     = { "Shrimp",      SPR_I_SHRIMP,     3, false },
    [IT_BURNT]      = { "Burnt shrimp",SPR_I_BURNT,      0, false },
    [IT_AXE]        = { "Bronze axe",  SPR_I_AXE,        0, true  },
    [IT_PICK]       = { "Bronze pick", SPR_I_PICK,       0, true  },
    [IT_NET]        = { "Small net",   SPR_I_NET,        0, true  },
    [IT_TINDER]     = { "Tinderbox",   SPR_I_TINDER,     0, true  },
    [IT_BONES]      = { "Bones",       SPR_I_BONES,      0, false },
    [IT_ESSENCE]    = { "Rune essence",SPR_I_ESSENCE,    0, false },
    [IT_AIR_RUNE]   = { "Air rune",    SPR_I_AIR_RUNE,   0, false, 0,0,0,0,0, true },
    [IT_FIRE_RUNE]  = { "Fire rune",   SPR_I_FIRE_RUNE,  0, false, 0,0,0,0,0, true },
    [IT_WATER_RUNE] = { "Water rune",  SPR_I_WATER_RUNE, 0, false, 0,0,0,0,0, true },
    [IT_EARTH_RUNE] = { "Earth rune",  SPR_I_EARTH_RUNE, 0, false, 0,0,0,0,0, true },
    [IT_LAW_RUNE]   = { "Law rune",    SPR_I_LAW_RUNE,   0, false, 0,0,0,0,0, true },
    [IT_CHAOS_RUNE] = { "Chaos rune",  SPR_I_CHAOS_RUNE, 0, false, 0,0,0,0,0, true },
    [IT_BRONZE_BAR] = { "Bronze bar",  SPR_I_BRONZE_BAR, 0, false },
    [IT_IRON_BAR]   = { "Iron bar",    SPR_I_IRON_BAR,   0, false },
    [IT_HAMMER]     = { "Hammer",      SPR_I_HAMMER,     0, true  },
    [IT_BRONZE_SWORD]={ "Bronze sword",SPR_I_BRONZE_SWORD,0,false, SLOT_WEAPON,4,3,0,1 },
    [IT_IRON_SWORD] = { "Iron sword",  SPR_I_IRON_SWORD, 0,false, SLOT_WEAPON,7,6,0,1 },
    [IT_IRON_AXE]   = { "Iron axe",    SPR_I_IRON_AXE,   0, true  },
    [IT_IRON_PICK]  = { "Iron pick",   SPR_I_IRON_PICK,  0, true  },
    [IT_BRONZE_HELM]  ={ "Bronze helm",  SPR_I_BRONZE_HELM,  0,false, SLOT_HELM,  0,0,3,1  },
    [IT_IRON_HELM]    ={ "Iron helm",    SPR_I_IRON_HELM,    0,false, SLOT_HELM,  0,0,5,10 },
    [IT_BRONZE_SHIELD]={ "Bronze shield",SPR_I_BRONZE_SHIELD,0,false, SLOT_SHIELD,1,0,5,1  },
    [IT_IRON_SHIELD]  ={ "Iron shield",  SPR_I_IRON_SHIELD,  0,false, SLOT_SHIELD,2,0,8,10 },
    [IT_BRONZE_BODY]  ={ "Bronze body",  SPR_I_BRONZE_BODY,  0,false, SLOT_BODY,  0,0,8,1  },
    [IT_IRON_BODY]    ={ "Iron body",    SPR_I_IRON_BODY,    0,false, SLOT_BODY,  0,0,14,10},
    [IT_STAFF]        ={ "Wizard staff", SPR_I_STAFF,    0,false, SLOT_WEAPON,1,2,0,1, false,12 },
    [IT_WIZ_HAT]      ={ "Wizard hat",   SPR_I_WIZ_HAT,  0,false, SLOT_HELM,  0,0,1,1, false, 3 },
    [IT_WIZ_ROBE]     ={ "Wizard robe",  SPR_I_WIZ_ROBE, 0,false, SLOT_BODY,  0,0,2,1, false, 5 },
    [IT_COINS]        ={ "Coins",        SPR_I_COINS,    0,false },
    [IT_STEEL_SWORD]  ={ "Steel sword",  SPR_I_STEEL_SWORD, 0,false, SLOT_WEAPON,10,9,0,20  },
    [IT_STEEL_HELM]   ={ "Steel helm",   SPR_I_STEEL_HELM,  0,false, SLOT_HELM,  0,0,7,20  },
    [IT_STEEL_SHIELD] ={ "Steel shield", SPR_I_STEEL_SHIELD,0,false, SLOT_SHIELD,2,0,11,20 },
    [IT_STEEL_BODY]   ={ "Steel body",   SPR_I_STEEL_BODY,  0,false, SLOT_BODY,  0,0,20,20 },
    [IT_MITH_SWORD]   ={ "Mithril sword",SPR_I_MITH_SWORD,  0,false, SLOT_WEAPON,14,13,0,30 },
    [IT_MITH_HELM]    ={ "Mithril helm", SPR_I_MITH_HELM,   0,false, SLOT_HELM,  0,0,10,30 },
    [IT_MITH_SHIELD]  ={ "Mithril shield",SPR_I_MITH_SHIELD,0,false, SLOT_SHIELD,3,0,15,30 },
    [IT_MITH_BODY]    ={ "Mithril body", SPR_I_MITH_BODY,   0,false, SLOT_BODY,  0,0,28,30 },
    [IT_RUNE_SWORD]   ={ "Rune sword",   SPR_I_RUNE_SWORD,  0,false, SLOT_WEAPON,20,18,0,40 },
    [IT_RUNE_HELM]    ={ "Rune helm",    SPR_I_RUNE_HELM,   0,false, SLOT_HELM,  0,0,14,40 },
    [IT_RUNE_SHIELD]  ={ "Rune shield",  SPR_I_RUNE_SHIELD, 0,false, SLOT_SHIELD,4,0,20,40 },
    [IT_RUNE_BODY]    ={ "Rune body",    SPR_I_RUNE_BODY,   0,false, SLOT_BODY,  0,0,40,40 },
    [IT_BANE]         ={ "Warlord's Bane",SPR_I_BANE,       0,false, SLOT_WEAPON,18,17,0,20 },
    [IT_DRAGONSTONE]  ={ "Dragonstone",  SPR_I_DRAGONSTONE, 0,false },
    /* the Dragon's rare unique: a hybrid blade strong in melee AND magic */
    [IT_DRAGONFIRE]   ={ "Dragonfire blade",SPR_I_DRAGONFIRE,0,false, SLOT_WEAPON,22,21,2,40,false,18 },
    /* fishing tackle: each tool lands a better fish at the spots */
    [IT_ROD]          ={ "Fishing rod",   SPR_I_ROD,          0, true },
    [IT_LOBSTER_POT]  ={ "Lobster pot",   SPR_I_LOBSTER_POT,  0, true },
    [IT_HARPOON]      ={ "Harpoon",       SPR_I_HARPOON,      0, true },
    /* the catch (cooked fish heal more the better they are) */
    [IT_RAW_TROUT]    ={ "Raw trout",     SPR_I_RAW_TROUT,    0, false },
    [IT_TROUT]        ={ "Trout",         SPR_I_TROUT,        7, false },
    [IT_RAW_LOBSTER]  ={ "Raw lobster",   SPR_I_RAW_LOBSTER,  0, false },
    [IT_LOBSTER]      ={ "Lobster",       SPR_I_LOBSTER,     12, false },
    [IT_RAW_SWORDFISH]={ "Raw swordfish", SPR_I_RAW_SWORDFISH,0, false },
    [IT_SWORDFISH]    ={ "Swordfish",     SPR_I_SWORDFISH,   16, false },
    /* fletching + ranged: knife to fletch, shafts + smithed tips make arrows,
       bows are fletched from logs. mag field stays 0; rng is the last field. */
    [IT_KNIFE]        ={ "Knife",         SPR_I_KNIFE,        0, true },
    [IT_ARROW_SHAFT]  ={ "Arrow shaft",   SPR_I_ARROW_SHAFT,  0, false, 0,0,0,0,0, true },
    [IT_BRONZE_TIPS]  ={ "Bronze arrowtips",SPR_I_BRONZE_TIPS,0, false, 0,0,0,0,0, true },
    [IT_IRON_TIPS]    ={ "Iron arrowtips",SPR_I_IRON_TIPS,    0, false, 0,0,0,0,0, true },
    [IT_BRONZE_ARROW] ={ "Bronze arrow",  SPR_I_BRONZE_ARROW, 0, false, 0,0,0,0,0, true, 0, 3 },
    [IT_IRON_ARROW]   ={ "Iron arrow",    SPR_I_IRON_ARROW,   0, false, 0,0,0,0,0, true, 0, 7 },
    [IT_SHORTBOW]     ={ "Shortbow",      SPR_I_SHORTBOW,     0, false, SLOT_WEAPON,0,0,0,1,  false,0, 8 },
    [IT_OAK_BOW]      ={ "Oak shortbow",  SPR_I_OAK_BOW,      0, false, SLOT_WEAPON,0,0,0,20, false,0, 14 },
    /* coal: fuels steel (1 iron + 1 coal) and mithril (1 ore + 2 coal) smelts */
    [IT_COAL]         ={ "Coal",          SPR_I_COAL,         0, false },
    [IT_STEEL_BAR]    ={ "Steel bar",     SPR_I_STEEL_BAR,    0, false },
    /* mithril: a deeper ore that smelts into bars for gear and arrowtips */
    [IT_MITH_ORE]     ={ "Mithril ore",   SPR_I_MITH_ORE,     0, false },
    [IT_MITH_BAR]     ={ "Mithril bar",   SPR_I_MITH_BAR,     0, false },
    [IT_MITH_TIPS]    ={ "Mithril arrowtips",SPR_I_MITH_TIPS, 0, false, 0,0,0,0,0, true },
    [IT_MITH_ARROW]   ={ "Mithril arrow", SPR_I_MITH_ARROW,   0, false, 0,0,0,0,0, true, 0, 12 },
    /* crafting: cure hides into leather, stitch with needle + thread */
    [IT_COWHIDE]      ={ "Cowhide",       SPR_I_COWHIDE,      0, false },
    [IT_LEATHER]      ={ "Leather",       SPR_I_LEATHER,      0, false },
    [IT_DRAGON_HIDE]  ={ "Dragonhide",    SPR_I_DRAGON_HIDE,  0, false },
    [IT_DRAGON_LEATHER]={ "Dragon leather",SPR_I_DRAGON_LEATHER,0,false },
    [IT_NEEDLE]       ={ "Needle",        SPR_I_NEEDLE,       0, true },
    [IT_THREAD]       ={ "Thread",        SPR_I_THREAD,       0, false, 0,0,0,0,0, true },
    /* ranged armour: worn for a Ranged accuracy bonus, needs Ranged to wear */
    [IT_LEATHER_COIF] ={ "Leather coif",  SPR_I_LEATHER_COIF, 0, false, SLOT_HELM, 0,0,2,1,  false,0, 2 },
    [IT_LEATHER_BODY] ={ "Leather body",  SPR_I_LEATHER_BODY, 0, false, SLOT_BODY, 0,0,4,1,  false,0, 4 },
    [IT_DHIDE_COIF]   ={ "D'hide coif",   SPR_I_DHIDE_COIF,   0, false, SLOT_HELM, 0,0,5,20, false,0, 5 },
    [IT_DHIDE_BODY]   ={ "D'hide body",   SPR_I_DHIDE_BODY,   0, false, SLOT_BODY, 0,0,8,30, false,0, 8 },
};

/* shop value in coins; buy price = value, sell price = value/2 (min 1).
   value 0 means the item can't be sold */
static const int item_value[NUM_ITEMS] = {
    [IT_LOGS]=4, [IT_OAK_LOGS]=15, [IT_COPPER]=8, [IT_TIN]=8, [IT_IRON]=25,
    [IT_RAW_SHRIMP]=3, [IT_SHRIMP]=8, [IT_ESSENCE]=2, [IT_BONES]=1,
    [IT_AIR_RUNE]=4, [IT_FIRE_RUNE]=5, [IT_WATER_RUNE]=6, [IT_EARTH_RUNE]=7,
    [IT_LAW_RUNE]=30, [IT_CHAOS_RUNE]=12, [IT_BRONZE_BAR]=20, [IT_IRON_BAR]=40,
    [IT_AXE]=16, [IT_PICK]=16, [IT_NET]=10, [IT_TINDER]=12, [IT_HAMMER]=14,
    [IT_BRONZE_SWORD]=32, [IT_IRON_SWORD]=80, [IT_IRON_AXE]=70, [IT_IRON_PICK]=70,
    [IT_BRONZE_HELM]=26, [IT_IRON_HELM]=70, [IT_BRONZE_SHIELD]=40, [IT_IRON_SHIELD]=100,
    [IT_BRONZE_BODY]=60, [IT_IRON_BODY]=150,
    [IT_STAFF]=200, [IT_WIZ_HAT]=80, [IT_WIZ_ROBE]=120,
    [IT_STEEL_SWORD]=200, [IT_STEEL_HELM]=150, [IT_STEEL_SHIELD]=250, [IT_STEEL_BODY]=400,
    [IT_MITH_SWORD]=500, [IT_MITH_HELM]=400, [IT_MITH_SHIELD]=600, [IT_MITH_BODY]=1000,
    [IT_RUNE_SWORD]=1500, [IT_RUNE_HELM]=1200, [IT_RUNE_SHIELD]=2000, [IT_RUNE_BODY]=3500,
    [IT_DRAGONSTONE]=2000,
    [IT_ROD]=20, [IT_LOBSTER_POT]=40, [IT_HARPOON]=80,
    [IT_RAW_TROUT]=10, [IT_TROUT]=25, [IT_RAW_LOBSTER]=30, [IT_LOBSTER]=60,
    [IT_RAW_SWORDFISH]=50, [IT_SWORDFISH]=100,
    [IT_KNIFE]=6, [IT_ARROW_SHAFT]=1, [IT_BRONZE_TIPS]=2, [IT_IRON_TIPS]=4,
    [IT_BRONZE_ARROW]=2, [IT_IRON_ARROW]=4, [IT_SHORTBOW]=25, [IT_OAK_BOW]=80,
    [IT_COAL]=18, [IT_STEEL_BAR]=80,
    [IT_MITH_ORE]=60, [IT_MITH_BAR]=120, [IT_MITH_TIPS]=8, [IT_MITH_ARROW]=8,
    [IT_COWHIDE]=12, [IT_LEATHER]=20, [IT_DRAGON_HIDE]=120, [IT_DRAGON_LEATHER]=200,
    [IT_NEEDLE]=2, [IT_THREAD]=2,
    [IT_LEATHER_COIF]=40, [IT_LEATHER_BODY]=80,
    [IT_DHIDE_COIF]=400, [IT_DHIDE_BODY]=1200,
};
static int sell_price(int item) { int v = item_value[item]; return v ? (v / 2 < 1 ? 1 : v / 2) : 0; }

/* fishing tiers: the tackle you carry (and your Fishing level) decides the
   catch at a fishing spot. Cooked fish heal per iteminfo[].heal, and burn
   until you reach cook_lvl. */
enum { FISH_SHRIMP, FISH_TROUT, FISH_LOBSTER, FISH_SWORD, NUM_FISH };
static const struct {
    const char *name; int raw, cooked, tool;
    int fish_lvl, cook_lvl, base_p, fish_xp, cook_xp;
} fishinfo[NUM_FISH] = {
    { "shrimp",    IT_RAW_SHRIMP,    IT_SHRIMP,    IT_NET,          1, 34, 20, 100,  300 },
    { "trout",     IT_RAW_TROUT,     IT_TROUT,     IT_ROD,         20, 40, 12, 175,  500 },
    { "lobster",   IT_RAW_LOBSTER,   IT_LOBSTER,   IT_LOBSTER_POT, 40, 55,  8, 300,  900 },
    { "swordfish", IT_RAW_SWORDFISH, IT_SWORDFISH, IT_HARPOON,     50, 70,  6, 400, 1200 },
};

static int gp = 0;     /* the player's coins */

/* worn equipment: equipped[slot-1] holds an item id (IT_NONE = empty) */
static int equipped[NUM_SLOTS];
static const char *slot_names[NUM_SLOTS] = { "Weapon", "Shield", "Helm", "Body" };

static int equip_atk(void)
{
    int t = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (equipped[i]) t += iteminfo[equipped[i]].atk;
    return t;
}
static int equip_str(void)
{
    int t = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (equipped[i]) t += iteminfo[equipped[i]].str;
    return t;
}
static int equip_def(void)
{
    int t = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (equipped[i]) t += iteminfo[equipped[i]].def;
    return t;
}
static int equip_mag(void)
{
    int t = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (equipped[i]) t += iteminfo[equipped[i]].mag;
    return t;
}
static int equip_rng(void)
{
    int t = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (equipped[i]) t += iteminfo[equipped[i]].rng;
    return t;
}
/* a bow: a weapon whose bonus is ranged, not melee */
static bool is_bow(int item)
{
    return item != IT_NONE && iteminfo[item].slot == SLOT_WEAPON &&
           iteminfo[item].rng > 0;
}
/* ranged armour: worn gear (helm/body/shield) carrying a Ranged bonus */
static bool is_ranged_armour(int item)
{
    int s = iteminfo[item].slot;
    return (s == SLOT_HELM || s == SLOT_BODY || s == SLOT_SHIELD) &&
           iteminfo[item].rng > 0;
}

/* ------------------------------------------------------------ audio */

enum { SND_CHOP, SND_MINE, SND_SPLASH, SND_HIT, SND_EAT, SND_COOK, SND_FIRE,
       SND_LEVELUP, SND_DEATH, SND_CRAFT, SND_SMITH, NUM_SND };
static const char *snd_files[NUM_SND] = {
    "chop", "mine", "splash", "hit", "eat", "cook", "fire", "levelup", "death",
    "craft", "smith"
};
static wav64_t snd[NUM_SND];
static wav64_t music;
static bool music_on = true;

#define CH_MUSIC 0
#define CH_SFX   2
#define CH_UI    4

static void sfx(int s) { wav64_play(&snd[s], CH_SFX); }
static void sfx_ui(int s) { wav64_play(&snd[s], CH_UI); }

/* ------------------------------------------------------------ player */

typedef enum { ST_IDLE, ST_CHOP, ST_MINE, ST_FISH, ST_COOK, ST_LIGHT, ST_FIGHT,
               ST_SMELT } pstate_t;

static struct {
    int   tx, ty;            /* current tile           */
    float px, py;            /* feet-center pixel pos  */
    int   mtx, mty;          /* tile we're stepping to */
    bool  moving;
    int   facing;            /* 0 down 1 up 2 left 3 right */
    float walk_anim;
    pstate_t state;
    int   act_x, act_y;      /* action target tile     */
    int   act_timer;
    int   fight_target;
    int   atk_cd;
    int   hp;
    int   run_energy;
    bool  run_on;
    int   eat_cd;
    int   regen;
    int   hitsplat, hitsplat_t;
    int   spawn_x, spawn_y;
    float bump_x, bump_y;    /* combat lunge/recoil offset (decays to 0) */
    int   bump_t;            /* frames of bump animation left */
} pl;

static int inv[INV_SIZE];
static int inv_qty[INV_SIZE];      /* stack count per slot (1 for non-stackable) */
static int bank[NUM_ITEMS];

/* ------------------------------------------------------------ goblins */

typedef struct {
    bool  exists;
    int   type;              /* MOB_* */
    int   tx, ty, sx, sy;
    float px, py;
    int   mtx, mty;
    bool  moving;
    int   hp;
    bool  dead;
    int   respawn;
    bool  aggro;
    int   atk_cd, wander_cd;
    int   hitsplat, hitsplat_t;
    int   hurt_timer;        /* show hp bar briefly */
    float bump_x, bump_y;    /* combat lunge/recoil offset (decays to 0) */
    int   bump_t;            /* frames of bump animation left */
    int   special_cd;        /* boss: ranged-attack timer */
    int   phase;             /* boss: 0 normal, 1 enraged */
    bool  summoned;          /* spawned by a boss; vanishes (no respawn) on death */
} gob_t;

static gob_t gob[MAX_GOB];
static int num_gob = 0;

/* monster types: stats, sprite, size, drop. mob_def lowers player accuracy;
   hit_base is the mob's chance to land before the player's defence. */
enum { MOB_GOBLIN, MOB_SKELETON, MOB_BOSS, MOB_WIGHT, MOB_DEMON, MOB_COW,
       MOB_WHELP, MOB_DRAGON, NUM_MOBS };
typedef struct {
    const char *name; int max_hp; int max_dmg; int mob_def; int hit_base;
    int aggro; int respawn; int spr_a, spr_b; int w, h;
} mobinfo_t;
static const mobinfo_t mobinfo[NUM_MOBS] = {
    [MOB_GOBLIN]   = { "goblin",         5, 1,  1, 52, 3, 25, SPR_GOB_A,   SPR_GOB_B,   16, 16 },
    [MOB_SKELETON] = { "skeleton",       9, 2,  6, 60, 4, 25, SPR_SKEL_A,  SPR_SKEL_B,  16, 16 },
    [MOB_BOSS]     = { "Goblin Warlord",45, 5, 15, 75, 6, 80, SPR_BOSS,    SPR_BOSS,    24, 24 },
    [MOB_WIGHT]    = { "wight",         14, 3, 10, 64, 4, 25, SPR_WIGHT_A, SPR_WIGHT_B, 16, 16 },
    [MOB_DEMON]    = { "Demon",         80, 8, 25, 85, 7,100, SPR_DEMON,   SPR_DEMON,   24, 24 },
    /* a placid training target: lots of HP for the xp, defence 0 so you hit
       almost every swing, never aggressive, barely scratches back, respawns fast */
    [MOB_COW]      = { "cow",            8, 1,  0, 25, 0,  8, SPR_COW_A,   SPR_COW_B,   16, 16 },
    /* floor-3 fodder, and the Dragon's brood */
    [MOB_WHELP]    = { "whelp",         18, 4, 12, 66, 5, 25, SPR_WHELP_A, SPR_WHELP_B, 16, 16 },
    /* the deepest boss: huge HP, hits hard in melee, and breathes fire from
       range. mob_def 35 demands rune-tier gear to land hits reliably */
    [MOB_DRAGON]   = { "Ancient Dragon",150,12, 35, 90, 8,140, SPR_DRAGON,  SPR_DRAGON,  24, 24 },
};

/* ------------------------------------------------------------ UI state */

typedef enum { UI_NONE, UI_INV, UI_SKILLS, UI_BANK, UI_HELP, UI_SMITH,
               UI_DIALOG, UI_EQUIP, UI_SPELL, UI_SHOP, UI_QUEST, UI_PRAYER,
               UI_ALMANAC, UI_FLETCH, UI_CRAFT } ui_t;
static ui_t ui_mode = UI_NONE;
static int smith_cursor = 0;
static int fletch_cursor = 0;
static int craft_cursor = 0;
static int equip_cursor = 0;
static int shop_id = 0, shop_mode = 0, shop_cursor = 0;   /* mode: 0 buy, 1 sell */

/* Almanac: a scrollable reference of all gear stats and monster stats/loot.
   Lines are built once when the screen opens. */
#define ALMANAC_MAX 160
#define ALMANAC_VIS 15
static char almanac_buf[ALMANAC_MAX][48];
static uint8_t almanac_style[ALMANAC_MAX];
static int almanac_count = 0;
static int almanac_scroll = 0;

/* magic: the selected combat spell (SPELL_MELEE = ordinary melee).
   SPELL_HOME is an instant utility cast handled in the spellbook, not a combat
   spell, so it never becomes the standing cast_spell. */
enum { SPELL_MELEE, SPELL_AIR, SPELL_WATER, SPELL_EARTH, SPELL_FIRE,
       SPELL_EBOLT, SPELL_FBOLT, SPELL_HOME, SPELL_BANK, SPELL_CAVE, NUM_SPELLS };
/* rune2/runes2 is a second required rune (IT_NONE = none): bolts need a
   Chaos rune on top of their element. */
static const struct { const char *name; int rune; int runes; int rune2; int runes2;
                      int maxhit; int lvl; int xp_x10; }
spellinfo[NUM_SPELLS] = {
    { "Melee",         IT_NONE,       0, IT_NONE,       0, 0,  1,   0 },
    { "Wind Strike",   IT_AIR_RUNE,   1, IT_NONE,       0, 2,  1,  55 },
    { "Water Strike",  IT_WATER_RUNE, 1, IT_NONE,       0, 3,  5,  76 },
    { "Earth Strike",  IT_EARTH_RUNE, 1, IT_NONE,       0, 4,  9,  95 },
    { "Fire Strike",   IT_FIRE_RUNE,  1, IT_NONE,       0, 5, 13, 100 },
    { "Earth Bolt",    IT_EARTH_RUNE, 3, IT_CHAOS_RUNE, 1, 7, 29, 190 },
    { "Fire Bolt",     IT_FIRE_RUNE,  3, IT_CHAOS_RUNE, 1, 8, 35, 225 },
    { "Home Teleport", IT_AIR_RUNE,   1, IT_NONE,       0, 0,  1,  35 },
    { "Bank Teleport", IT_LAW_RUNE,   1, IT_NONE,       0, 0, 20,  90 },
    { "Cave Teleport", IT_LAW_RUNE,   1, IT_NONE,       0, 0, 25, 110 },
};
static int cast_spell = SPELL_MELEE;
static int spell_cursor = 0;

/* prayer: toggleable boosts that drain prayer points (1 point per Prayer level).
   At most one prayer per category may be active; the percent feeds the matching
   combat level. Steel/strength/reflex/might are the high-level upgrades. */
enum { PCAT_DEF, PCAT_STR, PCAT_ATK, PCAT_MAG, NUM_PCAT };
enum { PR_THICK, PR_BURST, PR_CLARITY, PR_MYSTICW,
       PR_STEEL, PR_ULTSTR, PR_REFLEX, PR_MYSTICM, NUM_PRAYERS };
static const struct { const char *name; int level, cat, pct, drain; }
prayerinfo[NUM_PRAYERS] = {
    { "Thick Skin",          1, PCAT_DEF, 10, 3 },
    { "Burst of Strength",   4, PCAT_STR, 10, 3 },
    { "Clarity of Thought",  7, PCAT_ATK, 10, 3 },
    { "Mystic Will",         9, PCAT_MAG, 10, 3 },
    { "Steel Skin",         20, PCAT_DEF, 20, 6 },
    { "Ultimate Strength",  22, PCAT_STR, 20, 6 },
    { "Incredible Reflexes",24, PCAT_ATK, 20, 6 },
    { "Mystic Might",       26, PCAT_MAG, 20, 6 },
};
static bool pray_on[NUM_PRAYERS];
static int pray_pts;            /* current prayer points */
static int pray_drain_acc;      /* accumulates active drain; -1 point per 120 */
static int pray_regen;          /* slow passive regen accumulator when inactive */
static int pray_cursor = 0;

/* prayer points cap = Prayer level */
static int pray_max(void) { return level_of(SK_PRAY); }

/* percent boost the active prayer of a category grants (0 if none active) */
static int pray_bonus(int cat)
{
    for (int i = 0; i < NUM_PRAYERS; i++)
        if (pray_on[i] && prayerinfo[i].cat == cat) return prayerinfo[i].pct;
    return 0;
}

static void deactivate_all_prayers(void)
{
    for (int i = 0; i < NUM_PRAYERS; i++) pray_on[i] = false;
}

/* spell projectiles (purely visual; damage applies on cast) */
#define MAX_PROJ 6
typedef struct { bool active; float x, y, tx, ty; int spr; } proj_t;
static proj_t proj[MAX_PROJ];

/* quest: The Chef's Little Problem */
enum { QUEST_NONE, QUEST_ACTIVE, QUEST_DONE };
static int quest_state = QUEST_NONE;
static int quest_kills = 0;
#define QUEST_KILLS_NEEDED 3
#define QUEST_SHRIMP_NEEDED 2

/* The Warlord's Bane - a multi-stage quest from the Knight */
enum { WQ_NONE, WQ_STARTED, WQ_SCOUTED, WQ_ARMED, WQ_SLAIN, WQ_DONE };
static int wquest = WQ_NONE;
#define WQ_BONES_NEEDED 5
#define WQ_BARS_NEEDED  2
#define WQ_COIN_COST    300
/* you must be a proven warrior-smith before Sir Garrick will trust you */
#define WQ_REQ_ATT   20
#define WQ_REQ_DEF   20
#define WQ_REQ_SMITH 15

/* The Demon Below - the follow-up quest, on the dungeon's second floor */
enum { DQ_NONE, DQ_STARTED, DQ_SLAIN, DQ_DONE };
static int dquest = DQ_NONE;
#define DQ_REQ_ATT 30
#define DQ_REQ_DEF 30
#define DQ_REQ_HP  30

/* Basic Training - Sergeant Hardy's intro quest at the cow pasture */
enum { CQ_NONE, CQ_STARTED, CQ_DONE };
static int cquest = CQ_NONE;
static int cow_kills = 0;
#define CQ_COWS_NEEDED 5

/* dialog box */
static const char *dlg_title = "";
static char dlg_buf[4][64];
static int dlg_count = 0;

typedef enum { STATE_TITLE, STATE_PLAY } gstate_t;
static gstate_t game_state = STATE_TITLE;
static bool save_found = false;
static int title_sel = 0;
static int inv_cursor = 0;
static int bank_cursor = 0;

static char chat[CHAT_LINES][64];

typedef struct { bool active; char text[24]; float y; int age; } xpdrop_t;
static xpdrop_t xpdrop[MAX_XPDROP];

static int frame_counter = 0;
static int gratz_timer = 0;        /* bots say "gz" while this is set */

/* ------------------------------------------------------------ helpers */

static void msg(const char *fmt, ...)
{
    for (int i = 0; i < CHAT_LINES - 1; i++)
        strcpy(chat[i], chat[i + 1]);
    va_list args;
    va_start(args, fmt);
    vsnprintf(chat[CHAT_LINES - 1], sizeof chat[0], fmt, args);
    va_end(args);
}

static void show_xpdrop(int amount_x10)
{
    for (int i = 0; i < MAX_XPDROP; i++) {
        if (xpdrop[i].active) continue;
        xpdrop[i].active = true;
        snprintf(xpdrop[i].text, sizeof xpdrop[i].text, "+%d xp", amount_x10 / 10);
        xpdrop[i].y = 0;
        xpdrop[i].age = 0;
        return;
    }
}

static void add_xp(int sk, int amount_x10, bool drop)
{
    int before = level_of(sk);
    xp[sk] += amount_x10;
    if (drop) show_xpdrop(amount_x10);
    int after = level_of(sk);
    if (after > before) {
        msg("Congratulations, you just advanced a %s level.", skill_names[sk]);
        msg("Your %s level is now %d.", skill_names[sk], after);
        sfx_ui(SND_LEVELUP);
        if (sk == SK_HP) pl.hp++;
        gratz_timer = 3;
    }
}

static void toggle_prayer(int i)
{
    if (level_of(SK_PRAY) < prayerinfo[i].level) {
        msg("You need a Prayer level of %d for %s.",
            prayerinfo[i].level, prayerinfo[i].name);
        return;
    }
    if (pray_on[i]) { pray_on[i] = false; return; }
    if (pray_pts <= 0) { msg("You have run out of prayer points."); return; }
    /* one prayer per category: drop any rival in the same category */
    for (int j = 0; j < NUM_PRAYERS; j++)
        if (j != i && prayerinfo[j].cat == prayerinfo[i].cat) pray_on[j] = false;
    pray_on[i] = true;
    sfx_ui(SND_CRAFT);
    msg("You activate %s.", prayerinfo[i].name);
}

static int inv_count(int item)
{
    int n = 0;
    for (int i = 0; i < INV_SIZE; i++) if (inv[i] == item) n += inv_qty[i];
    return n;
}

static bool has_item(int item) { return inv_count(item) > 0; }

static bool add_item(int item)
{
    /* stackable items merge into their existing slot */
    if (iteminfo[item].stackable)
        for (int i = 0; i < INV_SIZE; i++)
            if (inv[i] == item) { inv_qty[i]++; return true; }
    for (int i = 0; i < INV_SIZE; i++)
        if (inv[i] == IT_NONE) { inv[i] = item; inv_qty[i] = 1; return true; }
    msg("Your inventory is too full to carry any more.");
    return false;
}

static bool remove_item(int item)
{
    for (int i = 0; i < INV_SIZE; i++)
        if (inv[i] == item) {
            if (--inv_qty[i] <= 0) { inv[i] = IT_NONE; inv_qty[i] = 0; }
            return true;
        }
    return false;
}

static bool inv_full(void)
{
    for (int i = 0; i < INV_SIZE; i++) if (inv[i] == IT_NONE) return false;
    return true;
}

static int chance(int percent) { return (rand() % 100) < percent; }

/* gear: the best tool/weapon you carry applies automatically */
static bool has_axe(void)  { return has_item(IT_AXE) || has_item(IT_IRON_AXE); }
static bool has_pick(void) { return has_item(IT_PICK) || has_item(IT_IRON_PICK); }
static int axe_bonus(void)  { return has_item(IT_IRON_AXE) ? 12 : 0; }
static int pick_bonus(void) { return has_item(IT_IRON_PICK) ? 12 : 0; }

static int cheb(int x0, int y0, int x1, int y1)
{
    int dx = abs(x0 - x1), dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

/* ------------------------------------------------------------ map */

static int cur_map = MAP_OVERWORLD;
static int dungeon_floor = 1;
static int up_x, up_y;             /* up-stairs of the current map */
static int down_x, down_y;         /* down-stairs of the current map (-1 if none) */

static gob_t *spawn_mob(int type, int x, int y)
{
    gob_t *g = NULL;
    for (int i = 0; i < num_gob; i++)        /* reclaim a vacated (summoned) slot */
        if (!gob[i].exists) { g = &gob[i]; break; }
    if (!g) {
        if (num_gob >= MAX_GOB) return NULL;
        g = &gob[num_gob++];
    }
    memset(g, 0, sizeof *g);
    g->exists = true;
    g->type = type;
    g->tx = g->sx = g->mtx = x;
    g->ty = g->sy = g->mty = y;
    g->px = x * TILE + 8; g->py = y * TILE + 12;
    g->hp = mobinfo[type].max_hp;
    return g;
}

static void load_overworld(void)
{
    num_gob = 0;
    pl.spawn_x = 24; pl.spawn_y = 12;
    down_x = 6; down_y = 29;       /* cave mouth, overwritten by 'X' below */
    up_x = up_y = -1;
    for (int y = 0; y < MAP_H; y++) {
        assertf(strlen(map_rows[y]) == MAP_W, "map row %d is %d chars", y,
                (int)strlen(map_rows[y]));
        for (int x = 0; x < MAP_W; x++) {
            char c = map_rows[y][x];
            int ter = TER_GRASS, obj = OBJ_NONE;
            switch (c) {
            case 'p': ter = TER_PATH;   break;
            case 's': ter = TER_SAND;   break;
            case '~': ter = TER_WATER;  break;
            case '=': ter = TER_BRIDGE; break;
            case 'w': ter = TER_WALL;   break;
            case 'f': ter = TER_FLOOR;  break;
            case 'B': ter = TER_FLOOR;  obj = OBJ_BOOTH; break;
            case 'T': obj = OBJ_TREE;        break;
            case 'O': obj = OBJ_OAK;         break;
            case 'C': obj = OBJ_ROCK_COPPER; break;
            case 'N': obj = OBJ_ROCK_TIN;    break;
            case 'I': obj = OBJ_ROCK_IRON;   break;
            case 'm': obj = OBJ_ROCK_MITHRIL;break;
            case 'k': obj = OBJ_ROCK_COAL;   break;
            case 'Z': obj = OBJ_TANNER;      break;
            case 'F': ter = TER_WATER; obj = OBJ_FISH; break;
            case 'E': obj = OBJ_ESSENCE;    break;
            case 'A': obj = OBJ_ALTAR_AIR;  break;
            case 'W': obj = OBJ_ALTAR_WATER; break;
            case 'D': obj = OBJ_ALTAR_EARTH; break;
            case 'R': obj = OBJ_ALTAR_FIRE; break;
            case 'U': obj = OBJ_FURNACE;    break;
            case 'V': obj = OBJ_ANVIL;      break;
            case 'H': obj = OBJ_CHEF;       break;
            case 'X': obj = OBJ_STAIRS_DOWN; down_x = x; down_y = y; break;
            case 'K': obj = OBJ_KNIGHT;     break;
            case 'L': obj = OBJ_FENCE;      break;
            case 'Y': obj = OBJ_TUTOR;      break;
            case 'M': spawn_mob(MOB_COW, x, y); break;
            case '1': obj = OBJ_SHOP_GENERAL; break;
            case '2': obj = OBJ_SHOP_WEAPON;  break;
            case '3': obj = OBJ_SHOP_ARMOR;   break;
            case '4': obj = OBJ_SHOP_MAGIC;   break;
            case 'G': spawn_mob(MOB_GOBLIN, x, y); break;
            case '@': ter = TER_PATH; pl.spawn_x = x; pl.spawn_y = y; break;
            }
            terrain[y][x] = ter;
            object[y][x] = obj;
            obj_orig[y][x] = obj;
            obj_timer[y][x] = 0;
        }
    }
}

static void build_dungeon(int floor)
{
    num_gob = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            bool wall = (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1);
            terrain[y][x] = wall ? TER_WALL : TER_CAVE;
            object[y][x] = OBJ_NONE;
            obj_orig[y][x] = OBJ_NONE;
            obj_timer[y][x] = 0;
        }
    /* a wall dividing the boss chamber (top) from the hall, with a doorway */
    for (int x = 1; x < MAP_W - 1; x++)
        if (x < 22 || x > 26) terrain[10][x] = TER_WALL;
    static const int pillars[][2] = { {12,16},{35,16},{12,24},{35,24},{24,28} };
    for (int i = 0; i < 5; i++) terrain[pillars[i][1]][pillars[i][0]] = TER_WALL;

    /* up-stairs (to the level above), bottom-centre */
    up_x = 24; up_y = 32;
    object[up_y][up_x] = OBJ_STAIRS_UP;
    obj_orig[up_y][up_x] = OBJ_STAIRS_UP;

    if (floor == 1) {
        /* a stair deeper down sits in the Warlord's chamber */
        down_x = 10; down_y = 4;
        object[down_y][down_x] = OBJ_STAIRS_DOWN;
        obj_orig[down_y][down_x] = OBJ_STAIRS_DOWN;
        static const int skel[][2] = {
            {6,14},{40,14},{18,18},{30,18},{10,22},{38,22},{24,24},{16,30},{32,30}
        };
        for (int i = 0; i < 9; i++) spawn_mob(MOB_SKELETON, skel[i][0], skel[i][1]);
        spawn_mob(MOB_BOSS, 24, 5);
        /* the Chaos and Law altars hide in the hall, guarded by the skeletons */
        object[16][8]  = OBJ_ALTAR_CHAOS;  obj_orig[16][8]  = OBJ_ALTAR_CHAOS;
        object[16][40] = OBJ_ALTAR_LAW;    obj_orig[16][40] = OBJ_ALTAR_LAW;
    } else if (floor == 2) {
        /* a final stair opens in the Demon's chamber once it is bested */
        if (dquest == DQ_DONE) {
            down_x = 38; down_y = 4;
            object[down_y][down_x] = OBJ_STAIRS_DOWN;
            obj_orig[down_y][down_x] = OBJ_STAIRS_DOWN;
        } else {
            down_x = down_y = -1;
        }
        static const int wt[][2] = {
            {8,14},{38,14},{16,17},{30,17},{12,21},{34,21},{20,24},{28,24},
            {10,30},{36,30}
        };
        for (int i = 0; i < 10; i++) spawn_mob(MOB_WIGHT, wt[i][0], wt[i][1]);
        spawn_mob(MOB_DEMON, 24, 5);
    } else {
        down_x = down_y = -1;          /* the deepest floor: the Dragon's lair */
        static const int wh[][2] = {
            {8,14},{38,14},{16,18},{30,18},{12,22},{34,22},{20,26},{28,26}
        };
        for (int i = 0; i < 8; i++) spawn_mob(MOB_WHELP, wh[i][0], wh[i][1]);
        spawn_mob(MOB_DRAGON, 24, 5);
    }
}

static void load_map(int which)
{
    cur_map = which;
    if (which == MAP_DUNGEON) build_dungeon(dungeon_floor);
    else load_overworld();
}

static void place_player(int x, int y)
{
    pl.tx = pl.mtx = x;
    pl.ty = pl.mty = y;
    pl.px = x * TILE + 8; pl.py = y * TILE + 12;
    pl.moving = false;
    pl.state = ST_IDLE;
}

static void enter_dungeon(void)
{
    dungeon_floor = 1;
    load_map(MAP_DUNGEON);
    place_player(up_x, up_y + 1);    /* arrive below the up-stairs */
    pl.facing = 0;
    msg("You descend into the gloom of the dungeon.");
    msg("Something large stirs beyond the doorway...");
}

static void descend_floor(void)      /* down one level via the deeper stair */
{
    dungeon_floor++;
    load_map(MAP_DUNGEON);
    place_player(up_x, up_y + 1);
    pl.facing = 0;
    if (dungeon_floor == 2) {
        msg("You climb down into the deep dark.");
        msg("The air reeks of brimstone - a Demon prowls here...");
    } else {
        msg("You descend the last stair into a vast, hot cavern.");
        msg("Scales rasp on stone - an Ancient Dragon stirs...");
    }
}

static void exit_dungeon(void)
{
    load_map(MAP_OVERWORLD);
    place_player(down_x, down_y + 1);   /* step out of the cave mouth */
    pl.facing = 0;
    msg("You climb back into the daylight.");
}

static void ascend_floor(void)       /* up one level from the up-stairs */
{
    if (dungeon_floor > 1) {
        dungeon_floor--;
        load_map(MAP_DUNGEON);
        place_player(down_x, down_y + 1);   /* back beside the descent stair */
        pl.facing = 0;
        msg("You climb back up to the floor above.");
    } else {
        exit_dungeon();
    }
}

static bool tile_walkable(int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return false;
    int t = terrain[y][x];
    if (t == TER_WATER || t == TER_WALL) return false;
    int o = object[y][x];
    if (o != OBJ_NONE && o != OBJ_FIRE) return false;
    return true;
}

/* ------------------------------------------------------------ saves (EEPROM 4K) */

#define SAVE_MAGIC   0x52563634u   /* 'RV64' */
#define SAVE_VERSION 15            /* current save format */
#define SAVE_VER_MIN 15            /* oldest forward-compatible format we read */
#define SAVE_VER_V14 14            /* the last fixed-layout save; auto-migrated */

/* Forward-compatible save. Arrays use generous fixed caps so adding items,
   skills or slots never shifts the layout, and new scalar fields are carved
   from reserved[] - so future content no longer wipes saves. Only raise a cap
   (and re-test) if the game ever exceeds one. */
#define SAVE_CAP_SKILLS 24
#define SAVE_CAP_INV    32
#define SAVE_CAP_SLOTS  8
#define SAVE_CAP_ITEMS  96

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version;
    uint8_t  hp;
    uint8_t  tx, ty;
    uint8_t  run_energy;
    uint8_t  quest_state, quest_kills;
    uint8_t  wquest, dquest;
    uint8_t  cquest, cow_kills;
    uint32_t gp;
    int32_t  xp[SAVE_CAP_SKILLS];
    uint8_t  inv[SAVE_CAP_INV];
    uint8_t  inv_qty[SAVE_CAP_INV];
    uint8_t  equipped[SAVE_CAP_SLOTS];
    uint16_t bank[SAVE_CAP_ITEMS];
    uint8_t  reserved[32];         /* future scalar fields; zero in older saves */
    uint16_t checksum;
    uint8_t  pad2[3];              /* align sizeof to the 8-byte EEPROM block */
} save_t;
_Static_assert(sizeof(save_t) % 8 == 0, "save_t must be EEPROM-block aligned");
_Static_assert(sizeof(save_t) <= 512,   "save_t must fit EEPROM 4k");
_Static_assert(NUM_SKILLS <= SAVE_CAP_SKILLS, "raise SAVE_CAP_SKILLS");
_Static_assert(INV_SIZE   <= SAVE_CAP_INV,    "raise SAVE_CAP_INV");
_Static_assert(NUM_SLOTS  <= SAVE_CAP_SLOTS,  "raise SAVE_CAP_SLOTS");
_Static_assert(NUM_ITEMS  <= SAVE_CAP_ITEMS,  "raise SAVE_CAP_ITEMS");

/* v14: the previous fixed-layout save, dimensions frozen. Loaded once and
   migrated so existing progress survives. Do NOT use the live constants here. */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version, hp, tx, ty;
    int32_t  xp[13];
    uint8_t  inv[28];
    uint8_t  inv_qty[28];
    uint16_t bank[53];
    uint32_t gp;
    uint8_t  run_energy;
    uint8_t  quest_state, quest_kills;
    uint8_t  wquest, dquest;
    uint8_t  cquest, cow_kills;
    uint8_t  equipped[4];
    uint8_t  pad;
    uint16_t checksum;
    uint8_t  pad2[8];
} save_v14_t;
_Static_assert(sizeof(save_v14_t) == 248, "v14 layout is frozen");

static int save_timer = 0;

static uint16_t csum_bytes(const void *p, size_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint16_t sum = 0;
    for (size_t i = 0; i < n; i++) sum += b[i];
    return sum;
}

static void save_game(void)
{
    if (!eeprom_present()) return;
    save_t s;
    memset(&s, 0, sizeof s);       /* zero reserved[] + unused array tails */
    s.magic = SAVE_MAGIC;
    s.version = SAVE_VERSION;
    s.hp = pl.hp;
    s.tx = pl.tx; s.ty = pl.ty;
    for (int i = 0; i < NUM_SKILLS; i++) s.xp[i] = xp[i];
    for (int i = 0; i < INV_SIZE; i++) {
        s.inv[i] = inv[i];
        s.inv_qty[i] = inv_qty[i] > 255 ? 255 : inv_qty[i];
    }
    for (int i = 0; i < NUM_ITEMS; i++) s.bank[i] = bank[i];
    s.gp = gp;
    s.run_energy = pl.run_energy;
    s.quest_state = quest_state;
    s.quest_kills = quest_kills;
    s.wquest = wquest;
    s.dquest = dquest;
    s.cquest = cquest;
    s.cow_kills = cow_kills;
    for (int i = 0; i < NUM_SLOTS; i++) s.equipped[i] = equipped[i];
    s.checksum = csum_bytes(&s, offsetof(save_t, checksum));

    /* write only the blocks that changed, and keep the audio mixer fed
       between block writes: a long blocking EEPROM burst starves the
       audio/rspq pipeline and corrupts the RDP stream */
    uint8_t cur[sizeof(save_t)];
    eeprom_read_bytes(cur, 0, sizeof cur);
    const uint8_t *ns = (const uint8_t *)&s;
    for (int b = 0; b < (int)(sizeof(save_t) / 8); b++) {
        if (memcmp(cur + b * 8, ns + b * 8, 8) != 0)
            eeprom_write(b, ns + b * 8);
        if (audio_can_write()) {
            short *abuf = audio_write_begin();
            mixer_poll(abuf, audio_get_buffer_length());
            audio_write_end();
        }
    }
    save_found = true;
}

/* is there a save we can load - current format, or the migratable v14? */
static bool save_present(void)
{
    if (!eeprom_present()) return false;
    uint8_t hdr[8];
    eeprom_read_bytes(hdr, 0, sizeof hdr);
    uint32_t magic;
    memcpy(&magic, hdr, 4);
    if (magic != SAVE_MAGIC) return false;
    uint8_t ver = hdr[4];
    if (ver == SAVE_VER_V14) {
        save_v14_t o;
        eeprom_read_bytes((uint8_t *)&o, 0, sizeof o);
        return o.checksum == csum_bytes(&o, offsetof(save_v14_t, checksum));
    }
    if (ver >= SAVE_VER_MIN) {
        save_t s;
        eeprom_read_bytes((uint8_t *)&s, 0, sizeof s);
        return s.checksum == csum_bytes(&s, offsetof(save_t, checksum));
    }
    return false;
}

/* migrate the frozen v14 layout into the current struct */
static void v14_to_v15(const save_v14_t *o, save_t *n)
{
    memset(n, 0, sizeof *n);
    n->magic = SAVE_MAGIC; n->version = SAVE_VERSION;
    n->hp = o->hp; n->tx = o->tx; n->ty = o->ty;
    n->run_energy = o->run_energy;
    n->quest_state = o->quest_state; n->quest_kills = o->quest_kills;
    n->wquest = o->wquest; n->dquest = o->dquest;
    n->cquest = o->cquest; n->cow_kills = o->cow_kills;
    n->gp = o->gp;
    for (int i = 0; i < 13; i++) n->xp[i] = o->xp[i];
    for (int i = 0; i < 28; i++) { n->inv[i] = o->inv[i]; n->inv_qty[i] = o->inv_qty[i]; }
    for (int i = 0; i < 4;  i++) n->equipped[i] = o->equipped[i];
    for (int i = 0; i < 53; i++) n->bank[i] = o->bank[i];
}

/* copy a (validated, current-format) save into the live game state */
static void apply_save(const save_t *s)
{
    for (int i = 0; i < NUM_SKILLS; i++) xp[i] = s->xp[i];
    for (int i = 0; i < INV_SIZE; i++) {
        inv[i] = s->inv[i] < NUM_ITEMS ? s->inv[i] : IT_NONE;
        inv_qty[i] = (inv[i] == IT_NONE) ? 0 : (s->inv_qty[i] < 1 ? 1 : s->inv_qty[i]);
    }
    for (int i = 0; i < NUM_ITEMS; i++) bank[i] = s->bank[i];
    gp = s->gp;
    int maxhp = level_of(SK_HP);
    pl.hp = (s->hp >= 1 && s->hp <= maxhp) ? s->hp : maxhp;
    pl.run_energy = s->run_energy <= 100 ? s->run_energy : 100;
    quest_state = s->quest_state <= QUEST_DONE ? s->quest_state : QUEST_NONE;
    quest_kills = s->quest_kills <= QUEST_KILLS_NEEDED ? s->quest_kills : 0;
    wquest = s->wquest <= WQ_DONE ? s->wquest : WQ_NONE;
    dquest = s->dquest <= DQ_DONE ? s->dquest : DQ_NONE;
    cquest = s->cquest <= CQ_DONE ? s->cquest : CQ_NONE;
    cow_kills = s->cow_kills <= CQ_COWS_NEEDED ? s->cow_kills : 0;
    for (int i = 0; i < NUM_SLOTS; i++) {
        int it = s->equipped[i];
        /* only restore if it really belongs in this slot */
        equipped[i] = (it > 0 && it < NUM_ITEMS && iteminfo[it].slot == i + 1)
                      ? it : IT_NONE;
    }
    if (tile_walkable(s->tx, s->ty)) { pl.tx = s->tx; pl.ty = s->ty; }
    pl.mtx = pl.tx; pl.mty = pl.ty;
    pl.px = pl.tx * TILE + 8; pl.py = pl.ty * TILE + 12;
    /* prayer points are transient; you reload with a full prayer pool */
    deactivate_all_prayers();
    pray_pts = pray_max();
    pray_drain_acc = pray_regen = 0;
}

static void load_game(void)
{
    if (!eeprom_present()) return;
    uint8_t hdr[8];
    eeprom_read_bytes(hdr, 0, sizeof hdr);
    uint32_t magic;
    memcpy(&magic, hdr, 4);
    if (magic != SAVE_MAGIC) return;
    uint8_t ver = hdr[4];
    save_t s;
    if (ver == SAVE_VER_V14) {                 /* migrate the old layout */
        save_v14_t o;
        eeprom_read_bytes((uint8_t *)&o, 0, sizeof o);
        if (o.checksum != csum_bytes(&o, offsetof(save_v14_t, checksum))) return;
        v14_to_v15(&o, &s);
    } else if (ver >= SAVE_VER_MIN) {           /* current/forward-compatible */
        eeprom_read_bytes((uint8_t *)&s, 0, sizeof s);
        if (s.checksum != csum_bytes(&s, offsetof(save_t, checksum))) return;
    } else {
        return;
    }
    apply_save(&s);
}

/* ------------------------------------------------------------ combat & death */

static void player_die(void)
{
    msg("Oh dear, you are dead!");
    sfx_ui(SND_DEATH);
    pl.hp = level_of(SK_HP);
    deactivate_all_prayers();
    pray_pts = pray_max();
    pray_drain_acc = 0;
    ui_mode = UI_NONE;
    /* always wake up on the surface */
    if (cur_map != MAP_OVERWORLD) load_map(MAP_OVERWORLD);
    place_player(pl.spawn_x, pl.spawn_y);
    for (int i = 0; i < num_gob; i++) {
        gob[i].aggro = false;
        gob[i].hurt_timer = 0;
    }
    save_game();
}

/* death is deferred to the end of the tick: calling player_die() mid-combat
   would reload the map under the goblin loop and let auto-retaliate re-engage */
static bool pending_death = false;

/* kick an actor's render offset by `mag` px along (vx,vy); decays over
   BUMP_FRAMES. attackers lunge toward the foe, defenders recoil away. */
static void bump_set(float *bx, float *by, int *bt, float vx, float vy, float mag)
{
    float l = sqrtf(vx * vx + vy * vy);
    if (l < 0.001f) { vx = 0; vy = -1; l = 1; }   /* degenerate: nudge up */
    *bx = vx / l * mag;
    *by = vy / l * mag;
    *bt = BUMP_FRAMES;
}

static void hurt_player(int dmg)
{
    pl.hp -= dmg;
    pl.hitsplat = dmg;
    pl.hitsplat_t = 40;
    if (pl.hp <= 0) pending_death = true;
}

/* ---- monster drop tables (weighted; an IT_NONE entry means "nothing") ---- */
typedef struct { int item, qty, weight; } drop_t;
#define NDROPS(t) ((int)(sizeof(t) / sizeof(t[0])))

static const drop_t goblin_drops[] = {
    { IT_COINS, 12, 28 }, { IT_AIR_RUNE, 3, 24 }, { IT_RAW_SHRIMP, 1, 16 },
    { IT_COPPER, 1, 12 }, { IT_NONE, 0, 20 },
};
static const drop_t skeleton_drops[] = {
    { IT_COINS, 30, 25 }, { IT_FIRE_RUNE, 2, 20 }, { IT_AIR_RUNE, 4, 15 },
    { IT_IRON, 1, 13 }, { IT_SHRIMP, 1, 12 }, { IT_WIZ_HAT, 1, 5 }, { IT_NONE, 0, 10 },
};
static const drop_t boss_drops[] = {   /* no "nothing": the boss always pays */
    { IT_COINS, 350, 20 }, { IT_STAFF, 1, 14 }, { IT_WIZ_ROBE, 1, 12 },
    { IT_WIZ_HAT, 1, 10 }, { IT_FIRE_RUNE, 15, 10 },
    { IT_STEEL_BODY, 1, 14 }, { IT_MITH_HELM, 1, 12 }, { IT_MITH_SWORD, 1, 8 },
};
static const drop_t wight_drops[] = {
    { IT_COINS, 60, 26 }, { IT_FIRE_RUNE, 5, 18 }, { IT_IRON_BAR, 1, 14 },
    { IT_SHRIMP, 2, 14 }, { IT_MITH_HELM, 1, 4 }, { IT_NONE, 0, 24 },
};
static const drop_t demon_drops[] = {  /* the deepest boss pays the richest */
    { IT_COINS, 800, 22 }, { IT_RUNE_SWORD, 1, 14 }, { IT_RUNE_HELM, 1, 14 },
    { IT_RUNE_SHIELD, 1, 12 }, { IT_RUNE_BODY, 1, 8 }, { IT_FIRE_RUNE, 30, 14 },
    { IT_MITH_BODY, 1, 16 },
};
static const drop_t cow_drops[] = {   /* mostly cowhide for crafting, plus a few coins */
    { IT_COWHIDE, 1, 55 }, { IT_COINS, 6, 25 }, { IT_RAW_SHRIMP, 1, 8 },
    { IT_NONE, 0, 12 },
};
static const drop_t whelp_drops[] = {  /* floor-3 fodder: better than wights */
    { IT_COINS, 90, 26 }, { IT_FIRE_RUNE, 6, 20 }, { IT_SHRIMP, 2, 16 },
    { IT_MITH_HELM, 1, 4 }, { IT_NONE, 0, 34 },
};
static const drop_t dragon_drops[] = {  /* on top of a guaranteed coin+stone payout */
    { IT_COINS, 1500, 16 }, { IT_RUNE_BODY, 1, 12 }, { IT_RUNE_SWORD, 1, 11 },
    { IT_RUNE_SHIELD, 1, 10 }, { IT_RUNE_HELM, 1, 10 }, { IT_FIRE_RUNE, 60, 11 },
    { IT_DRAGON_HIDE, 2, 13 },   /* tan into dragon leather for the best ranged armour */
    { IT_DRAGONSTONE, 1, 10 },
    { IT_DRAGONFIRE, 1, 7 },   /* the rare unique: the Dragonfire blade */
};

static void give_drop(int item, int qty)
{
    if (item == IT_NONE || qty <= 0) return;
    if (item == IT_COINS) { gp += qty; msg("Drop: %d coins.", qty); return; }
    for (int i = 0; i < qty; i++)
        if (!add_item(item)) { msg("You have no room for all the loot!"); return; }
    if (qty > 1) msg("Drop: %s x%d.", iteminfo[item].name, qty);
    else         msg("Drop: %s.", iteminfo[item].name);
}

static void roll_drops(const drop_t *tbl, int n)
{
    int total = 0;
    for (int i = 0; i < n; i++) total += tbl[i].weight;
    if (total <= 0) return;
    int r = rand() % total, acc = 0;
    for (int i = 0; i < n; i++) {
        acc += tbl[i].weight;
        if (r < acc) { give_drop(tbl[i].item, tbl[i].qty); return; }
    }
}

/* the weighted loot table for a monster type (NULL if it has none) */
static const drop_t *mob_drops(int type, int *n)
{
    switch (type) {
    case MOB_GOBLIN:   *n = NDROPS(goblin_drops);   return goblin_drops;
    case MOB_SKELETON: *n = NDROPS(skeleton_drops); return skeleton_drops;
    case MOB_BOSS:     *n = NDROPS(boss_drops);     return boss_drops;
    case MOB_WIGHT:    *n = NDROPS(wight_drops);    return wight_drops;
    case MOB_DEMON:    *n = NDROPS(demon_drops);    return demon_drops;
    case MOB_COW:      *n = NDROPS(cow_drops);      return cow_drops;
    case MOB_WHELP:    *n = NDROPS(whelp_drops);    return whelp_drops;
    case MOB_DRAGON:   *n = NDROPS(dragon_drops);   return dragon_drops;
    default:           *n = 0;                      return NULL;
    }
}

/* ---- the Almanac: an in-game reference of gear and monster stats/loot ---- */
static void al_add(int style, const char *fmt, ...)
{
    if (almanac_count >= ALMANAC_MAX) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(almanac_buf[almanac_count], sizeof almanac_buf[0], fmt, ap);
    va_end(ap);
    almanac_style[almanac_count++] = style;
}

static void build_almanac(void)
{
    almanac_count = 0;

    al_add(1, "== WEAPONS ==  (Atk/Str/Mag, level to wield)");
    for (int it = 1; it < NUM_ITEMS; it++)
        if (iteminfo[it].slot == SLOT_WEAPON && !is_bow(it))
            al_add(0, "%-16s A+%-2d S+%-2d M+%-2d  L%d", iteminfo[it].name,
                   iteminfo[it].atk, iteminfo[it].str, iteminfo[it].mag,
                   iteminfo[it].eqlvl);

    al_add(1, "== ARMOUR ==  (Defence, level to wield)");
    for (int it = 1; it < NUM_ITEMS; it++) {
        int s = iteminfo[it].slot;
        if ((s == SLOT_HELM || s == SLOT_SHIELD || s == SLOT_BODY) &&
            !is_ranged_armour(it))
            al_add(0, "%-16s D+%-2d %-6s L%d", iteminfo[it].name,
                   iteminfo[it].def, slot_names[s - 1], iteminfo[it].eqlvl);
    }

    al_add(1, "== MONSTERS ==  (all drop bones; %% = drop roll)");
    for (int m = 0; m < NUM_MOBS; m++) {
        al_add(4, "%s", mobinfo[m].name);
        al_add(0, " HP %d   max hit %d   defence %d",
               mobinfo[m].max_hp, mobinfo[m].max_dmg, mobinfo[m].mob_def);
        if (m == MOB_DRAGON)
            al_add(5, " always: 2500 coins + Dragonstone");
        int n;
        const drop_t *t = mob_drops(m, &n);
        int total = 0;
        for (int i = 0; i < n; i++) total += t[i].weight;
        for (int i = 0; i < n; i++) {
            int pct = total ? t[i].weight * 100 / total : 0;
            if (t[i].item == IT_NONE)
                al_add(6, "  nothing                  %d%%", pct);
            else if (t[i].qty > 1)
                al_add(6, "  %-16s x%-4d %d%%",
                       iteminfo[t[i].item].name, t[i].qty, pct);
            else
                al_add(6, "  %-21s %d%%", iteminfo[t[i].item].name, pct);
        }
    }

    al_add(1, "== SPELLS ==  (spellbook, C-left)");
    for (int sp = 1; sp < NUM_SPELLS; sp++) {   /* skip Melee */
        al_add(4, "%s (Magic %d)", spellinfo[sp].name, spellinfo[sp].lvl);
        if (spellinfo[sp].maxhit > 0) {
            if (spellinfo[sp].runes2 > 0)
                al_add(6, "  max hit %d - %dx %s + %dx %s", spellinfo[sp].maxhit,
                       spellinfo[sp].runes,  iteminfo[spellinfo[sp].rune].name,
                       spellinfo[sp].runes2, iteminfo[spellinfo[sp].rune2].name);
            else
                al_add(6, "  max hit %d - %dx %s", spellinfo[sp].maxhit,
                       spellinfo[sp].runes, iteminfo[spellinfo[sp].rune].name);
        } else {
            al_add(6, "  teleport - %dx %s",
                   spellinfo[sp].runes, iteminfo[spellinfo[sp].rune].name);
        }
    }

    al_add(1, "== RANGED ==  (bows/armour: Rng/level; arrows: Rng)");
    for (int it = 1; it < NUM_ITEMS; it++)
        if (is_bow(it))
            al_add(0, "%-16s R+%-2d  L%d", iteminfo[it].name,
                   iteminfo[it].rng, iteminfo[it].eqlvl);
    for (int it = 1; it < NUM_ITEMS; it++)
        if (is_ranged_armour(it))
            al_add(0, "%-16s R+%-2d D+%-2d L%d", iteminfo[it].name,
                   iteminfo[it].rng, iteminfo[it].def, iteminfo[it].eqlvl);
    for (int it = 1; it < NUM_ITEMS; it++)
        if (iteminfo[it].slot == SLOT_NONE && iteminfo[it].rng > 0)
            al_add(0, "%-16s R+%d", iteminfo[it].name, iteminfo[it].rng);

    al_add(1, "== FISHING & FOOD ==  (tackle, levels, heal)");
    for (int f = 0; f < NUM_FISH; f++)
        al_add(0, "%-10s F%-2d %-12s cook%-2d  heal %d", fishinfo[f].name,
               fishinfo[f].fish_lvl, iteminfo[fishinfo[f].tool].name,
               fishinfo[f].cook_lvl, iteminfo[fishinfo[f].cooked].heal);
}

static void mob_die(gob_t *g)
{
    const mobinfo_t *mi = &mobinfo[g->type];
    g->dead = true;
    g->respawn = mi->respawn;
    g->aggro = false;
    g->hurt_timer = 0;
    if (g->type == MOB_BOSS) {
        msg("With a final roar, the Goblin Warlord falls!");
        sfx_ui(SND_LEVELUP);
        if (wquest == WQ_ARMED) {
            wquest = WQ_SLAIN;
            msg("The Warlord is slain! Return to Sir Garrick.");
        }
        /* a hero's bounty of combat experience */
        add_xp(SK_ATT, 4000, false);
        add_xp(SK_STR, 4000, false);
        add_xp(SK_DEF, 4000, false);
        add_xp(SK_HP,  3000, false);
        gratz_timer = 4;
    } else if (g->type == MOB_DEMON) {
        msg("The Demon shrieks and crumbles to ash!");
        sfx_ui(SND_LEVELUP);
        if (dquest == DQ_STARTED) {
            dquest = DQ_SLAIN;
            msg("The Demon is destroyed! Return to Sir Garrick.");
        }
        add_xp(SK_ATT, 9000, false);
        add_xp(SK_STR, 9000, false);
        add_xp(SK_DEF, 9000, false);
        add_xp(SK_HP,  7000, false);
        gratz_timer = 4;
    } else if (g->type == MOB_COW) {
        /* a brisk training reward on top of the in-fight combat xp */
        msg("You fell the cow.");
        add_xp(SK_ATT, 60, false);
        add_xp(SK_STR, 60, false);
        add_xp(SK_DEF, 60, false);
        if (cquest == CQ_STARTED && cow_kills < CQ_COWS_NEEDED) {
            cow_kills++;
            msg("Cow felled for Sergeant Hardy! (%d/%d)", cow_kills, CQ_COWS_NEEDED);
        }
    } else if (g->type == MOB_DRAGON) {
        msg("The Ancient Dragon collapses with an earth-shaking roar!");
        sfx_ui(SND_LEVELUP);
        /* the deepest kill in the game pays the most combat xp */
        add_xp(SK_ATT, 18000, false);
        add_xp(SK_STR, 18000, false);
        add_xp(SK_DEF, 18000, false);
        add_xp(SK_HP,  14000, false);
        /* a guaranteed hoard on top of the weighted table below */
        gp += 2500;
        if (!inv_full()) add_item(IT_DRAGONSTONE);
        msg("You plunder its hoard: 2500 coins and a Dragonstone!");
        gratz_timer = 4;
    } else {
        msg("You have defeated the %s!", mi->name);
    }
    /* summoned brood vanish on death: no loot (no farming), no respawn */
    if (g->summoned) {
        g->exists = false;
        if (pl.state == ST_FIGHT) pl.state = ST_IDLE;
        return;
    }
    if (g->type == MOB_GOBLIN && quest_state == QUEST_ACTIVE &&
        quest_kills < QUEST_KILLS_NEEDED) {
        quest_kills++;
        msg("Goblin bashed for the Chef! (%d/%d)", quest_kills, QUEST_KILLS_NEEDED);
    }
    /* always-bones, plus a roll on this monster's table */
    int bones = (g->type == MOB_BOSS || g->type == MOB_DEMON ||
                 g->type == MOB_DRAGON) ? 2 : 1;
    for (int i = 0; i < bones; i++) if (!inv_full()) add_item(IT_BONES);
    switch (g->type) {
    case MOB_GOBLIN:   roll_drops(goblin_drops,   NDROPS(goblin_drops));   break;
    case MOB_SKELETON: roll_drops(skeleton_drops, NDROPS(skeleton_drops)); break;
    case MOB_BOSS:     roll_drops(boss_drops,     NDROPS(boss_drops));     break;
    case MOB_WIGHT:    roll_drops(wight_drops,    NDROPS(wight_drops));    break;
    case MOB_DEMON:    roll_drops(demon_drops,    NDROPS(demon_drops));    break;
    case MOB_COW:      roll_drops(cow_drops,      NDROPS(cow_drops));      break;
    case MOB_WHELP:    roll_drops(whelp_drops,    NDROPS(whelp_drops));    break;
    case MOB_DRAGON:   roll_drops(dragon_drops,   NDROPS(dragon_drops));   break;
    }
    if (pl.state == ST_FIGHT) pl.state = ST_IDLE;
}

static void player_attack(gob_t *g)
{
    int att = level_of(SK_ATT) * (100 + pray_bonus(PCAT_ATK)) / 100 + equip_atk();
    int str = level_of(SK_STR) * (100 + pray_bonus(PCAT_STR)) / 100 + equip_str();
    int max_hit = 1 + str / 8;
    int p_hit = 100 * (att + 8) / (att + 18) - mobinfo[g->type].mob_def * 2;
    if (p_hit > 95) p_hit = 95;
    if (p_hit < 5) p_hit = 5;
    int dmg = chance(p_hit) ? (rand() % (max_hit + 1)) : 0;
    float vx = g->px - pl.px, vy = g->py - pl.py;
    bump_set(&pl.bump_x, &pl.bump_y, &pl.bump_t, vx, vy, BUMP_LUNGE);
    bump_set(&g->bump_x, &g->bump_y, &g->bump_t, vx, vy, BUMP_RECOIL);
    g->hp -= dmg;
    g->hitsplat = dmg;
    g->hitsplat_t = 40;
    g->hurt_timer = 16;     /* ~10s hp bar */
    g->aggro = true;
    sfx(SND_HIT);
    if (dmg > 0) {
        add_xp(SK_ATT, dmg * 13, false);
        add_xp(SK_STR, dmg * 13, false);
        add_xp(SK_DEF, dmg * 13, false);
        add_xp(SK_HP,  dmg * 13, false);
        show_xpdrop(dmg * 52);
    }
    if (g->hp <= 0) mob_die(g);
}

static void mob_attack(gob_t *g)
{
    int def = level_of(SK_DEF) * (100 + pray_bonus(PCAT_DEF)) / 100 + equip_def();
    int p_hit = mobinfo[g->type].hit_base - def;
    if (p_hit < 5) p_hit = 5;
    int dmg = chance(p_hit) ? (rand() % (mobinfo[g->type].max_dmg + 1)) : 0;
    float vx = pl.px - g->px, vy = pl.py - g->py;
    bump_set(&g->bump_x, &g->bump_y, &g->bump_t, vx, vy, BUMP_LUNGE);
    bump_set(&pl.bump_x, &pl.bump_y, &pl.bump_t, vx, vy, BUMP_RECOIL);
    sfx(SND_HIT);
    hurt_player(dmg);
    if (pending_death) return;          /* dead - don't fight on */
    /* auto-retaliate */
    if (pl.state == ST_IDLE && pl.hp > 0) {
        pl.state = ST_FIGHT;
        pl.fight_target = (int)(g - gob);
        pl.atk_cd = 2;
    }
}

static void spawn_proj(float x, float y, float tx, float ty, int spr)
{
    for (int i = 0; i < MAX_PROJ; i++)
        if (!proj[i].active) {
            proj[i] = (proj_t){ true, x, y, tx, ty, spr };
            return;
        }
}

/* returns false (and stops the fight) if the spell can't be cast */
static bool player_cast(gob_t *g)
{
    int rune = spellinfo[cast_spell].rune;
    int need = spellinfo[cast_spell].runes;
    int rune2 = spellinfo[cast_spell].rune2;
    int need2 = spellinfo[cast_spell].runes2;
    if (level_of(SK_MAGIC) < spellinfo[cast_spell].lvl) {
        msg("You need a Magic level of %d for that spell.",
            spellinfo[cast_spell].lvl);
        cast_spell = SPELL_MELEE;
        return false;
    }
    /* check every required rune before spending any */
    if (inv_count(rune) < need) {
        msg("You do not have enough %s.", iteminfo[rune].name);
        cast_spell = SPELL_MELEE;
        return false;
    }
    if (need2 > 0 && inv_count(rune2) < need2) {
        msg("You do not have enough %s.", iteminfo[rune2].name);
        cast_spell = SPELL_MELEE;
        return false;
    }
    for (int k = 0; k < need; k++) remove_item(rune);
    for (int k = 0; k < need2; k++) remove_item(rune2);
    spawn_proj(pl.px, pl.py - 8, g->px, g->py - 8,
               rune == IT_FIRE_RUNE ? SPR_BOLT_FIRE : SPR_BOLT_AIR);
    sfx(SND_CRAFT);
    int magic = level_of(SK_MAGIC) * (100 + pray_bonus(PCAT_MAG)) / 100;
    int mbonus = equip_mag();
    int p_hit = 55 + magic * 2 + mbonus - mobinfo[g->type].mob_def * 2;
    if (p_hit > 95) p_hit = 95;
    if (p_hit < 5) p_hit = 5;
    int maxhit = spellinfo[cast_spell].maxhit + mbonus / 10;   /* gear ups the cap */
    int dmg = chance(p_hit) ? (rand() % (maxhit + 1)) : 0;
    float vx = g->px - pl.px, vy = g->py - pl.py;
    bump_set(&pl.bump_x, &pl.bump_y, &pl.bump_t, vx, vy, BUMP_LUNGE);
    bump_set(&g->bump_x, &g->bump_y, &g->bump_t, vx, vy, BUMP_RECOIL);
    g->hp -= dmg;
    g->hitsplat = dmg;
    g->hitsplat_t = 40;
    g->hurt_timer = 16;
    g->aggro = true;
    add_xp(SK_MAGIC, spellinfo[cast_spell].xp_x10, true);
    if (dmg > 0) {
        add_xp(SK_HP, dmg * 13, false);
        add_xp(SK_MAGIC, dmg * 20, false);
    }
    if (g->hp <= 0) mob_die(g);
    return true;
}

/* the arrow the bow will fire (best held first), -1 if none usable.
   mithril arrows need an oak shortbow or better (wield level 20+). */
static int held_arrow(void)
{
    int bow = equipped[SLOT_WEAPON - 1];
    bool bigbow = is_bow(bow) && iteminfo[bow].eqlvl >= 20;
    if (bigbow && has_item(IT_MITH_ARROW)) return IT_MITH_ARROW;
    if (has_item(IT_IRON_ARROW))   return IT_IRON_ARROW;
    if (has_item(IT_BRONZE_ARROW)) return IT_BRONZE_ARROW;
    return -1;
}

/* ranged: a bow looses the best arrow in the pack. returns false (stopping the
   fight) when out of arrows. */
static bool player_shoot(gob_t *g)
{
    int arrow = held_arrow();
    if (arrow < 0) {
        if (has_item(IT_MITH_ARROW))
            msg("Your bow is too weak for mithril arrows. Use an oak shortbow.");
        else
            msg("You have no arrows for your bow.");
        return false;
    }
    remove_item(arrow);
    spawn_proj(pl.px, pl.py - 8, g->px, g->py - 8, SPR_BOLT_AIR);
    sfx(SND_HIT);
    int rng = level_of(SK_RANGED);
    int rbonus = equip_rng() + iteminfo[arrow].rng;
    int p_hit = 60 + rng * 2 + rbonus - mobinfo[g->type].mob_def * 2;
    if (p_hit > 95) p_hit = 95;
    if (p_hit < 5) p_hit = 5;
    int maxhit = 1 + (rng + rbonus) / 8;
    int dmg = chance(p_hit) ? (rand() % (maxhit + 1)) : 0;
    float vx = g->px - pl.px, vy = g->py - pl.py;
    bump_set(&pl.bump_x, &pl.bump_y, &pl.bump_t, vx, vy, BUMP_LUNGE);
    bump_set(&g->bump_x, &g->bump_y, &g->bump_t, vx, vy, BUMP_RECOIL);
    g->hp -= dmg;
    g->hitsplat = dmg;
    g->hitsplat_t = 40;
    g->hurt_timer = 16;
    g->aggro = true;
    add_xp(SK_RANGED, 40, true);
    if (dmg > 0) {
        add_xp(SK_RANGED, dmg * 20, false);
        add_xp(SK_HP, dmg * 13, false);
    }
    if (g->hp <= 0) mob_die(g);
    return true;
}

/* Teleports are instant utility casts (not combat spells). They spend their
   rune and whisk you to a fixed overworld tile - the escape from the dungeon
   and a fast-travel for bank/cave runs. */
static void do_teleport(int spell, int tx, int ty, const char *where)
{
    int rune = spellinfo[spell].rune;
    if (!has_item(rune)) {
        msg("You have run out of %s.", iteminfo[rune].name);
        return;
    }
    remove_item(rune);
    sfx_ui(SND_CRAFT);
    add_xp(SK_MAGIC, spellinfo[spell].xp_x10, true);
    if (cur_map != MAP_OVERWORLD) load_map(MAP_OVERWORLD);
    place_player(tx, ty);
    pl.state = ST_IDLE;
    msg("You teleport to %s.", where);
}

/* ------------------------------------------------------------ skilling ticks */

static void stop_action(void) { pl.state = ST_IDLE; }

/* the best fish the held tackle + Fishing level can land (-1 = no tackle) */
static int pick_fish(void)
{
    for (int f = NUM_FISH - 1; f >= 0; f--)
        if (has_item(fishinfo[f].tool) && level_of(SK_FISH) >= fishinfo[f].fish_lvl)
            return f;
    return -1;
}

/* the first raw fish in the pack, for cooking (-1 = none) */
static int raw_fish_kind(void)
{
    for (int f = 0; f < NUM_FISH; f++)
        if (has_item(fishinfo[f].raw)) return f;
    return -1;
}

static void tick_skilling(void)
{
    int ax = pl.act_x, ay = pl.act_y;

    switch (pl.state) {
    case ST_CHOP: {
        int o = object[ay][ax];
        if (o != OBJ_TREE && o != OBJ_OAK) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 4;
        int wc = level_of(SK_WC);
        bool oak = (o == OBJ_OAK);
        if (oak && wc < 15) { msg("You need a Woodcutting level of 15 to chop oaks."); stop_action(); break; }
        if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
        int p = (oak ? (10 + wc * 3 / 2) : (25 + wc * 2)) + axe_bonus();
        if (p > 90) p = 90;
        if (chance(p)) {
            sfx(SND_CHOP);
            add_item(oak ? IT_OAK_LOGS : IT_LOGS);
            msg(oak ? "You get some oak logs." : "You get some logs.");
            add_xp(SK_WC, oak ? 375 : 250, true);
            if (!oak || chance(13)) {           /* oaks survive ~7/8 logs */
                object[ay][ax] = OBJ_STUMP;
                obj_timer[ay][ax] = oak ? 14 : 10;
                stop_action();
            }
        } else {
            sfx(SND_CHOP);
        }
        break;
    }
    case ST_MINE: {
        int o = object[ay][ax];
        if (o != OBJ_ROCK_COPPER && o != OBJ_ROCK_TIN && o != OBJ_ROCK_IRON &&
            o != OBJ_ROCK_MITHRIL && o != OBJ_ROCK_COAL && o != OBJ_ESSENCE) {
            if (o == OBJ_ROCK_EMPTY) msg("There is no ore currently available in this rock.");
            stop_action(); break;
        }
        if (o == OBJ_ESSENCE) {
            /* essence rocks never deplete */
            if (--pl.act_timer > 0) break;
            pl.act_timer = 3;
            if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
            sfx(SND_MINE);
            if (chance(85)) {
                add_item(IT_ESSENCE);
                msg("You manage to mine some rune essence.");
                add_xp(SK_MINE, 50, true);
            }
            break;
        }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 4;
        int mn = level_of(SK_MINE);
        /* per-rock: ore item, level needed, success base, xp, respawn, noun */
        int ore, req, base, oxp, resp; const char *noun;
        switch (o) {
        case OBJ_ROCK_IRON:    ore=IT_IRON;     req=15; base=8+mn*3/2; oxp=350; resp=9;  noun="iron";        break;
        case OBJ_ROCK_COAL:    ore=IT_COAL;     req=20; base=6+mn;     oxp=250; resp=7;  noun="coal";        break;
        case OBJ_ROCK_MITHRIL: ore=IT_MITH_ORE; req=30; base=4+mn;     oxp=600; resp=16; noun="mithril ore"; break;
        case OBJ_ROCK_COPPER:  ore=IT_COPPER;   req=1;  base=25+mn*2;  oxp=175; resp=4;  noun="copper";      break;
        default:               ore=IT_TIN;      req=1;  base=25+mn*2;  oxp=175; resp=4;  noun="tin";         break;
        }
        if (mn < req) { msg("You need a Mining level of %d to mine that.", req); stop_action(); break; }
        if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
        int p = base + pick_bonus();
        if (p > 90) p = 90;
        if (chance(p)) {
            sfx(SND_MINE);
            add_item(ore);
            msg("You manage to mine some %s.", noun);
            add_xp(SK_MINE, oxp, true);
            object[ay][ax] = OBJ_ROCK_EMPTY;
            obj_timer[ay][ax] = resp;
            stop_action();
        } else {
            sfx(SND_MINE);
        }
        break;
    }
    case ST_FISH: {
        if (object[ay][ax] != OBJ_FISH) { stop_action(); break; }
        int f = pick_fish();
        if (f < 0) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 5;
        if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
        int p = fishinfo[f].base_p + level_of(SK_FISH) * 2;
        if (p > 85) p = 85;
        sfx(SND_SPLASH);
        if (chance(p)) {
            add_item(fishinfo[f].raw);
            msg("You catch some %s.", fishinfo[f].name);
            add_xp(SK_FISH, fishinfo[f].fish_xp, true);
        }
        break;
    }
    case ST_COOK: {
        if (object[ay][ax] != OBJ_FIRE) { stop_action(); break; }
        int f = raw_fish_kind();
        if (f < 0) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 4;
        remove_item(fishinfo[f].raw);
        int ck = level_of(SK_COOK);
        int p_burn = fishinfo[f].cook_lvl + 11 - ck;   /* burns until cook_lvl */
        if (ck >= fishinfo[f].cook_lvl) p_burn = 0;
        else if (p_burn > 50) p_burn = 50;
        else if (p_burn < 4) p_burn = 4;
        sfx(SND_COOK);
        if (chance(p_burn)) {
            add_item(IT_BURNT);
            msg("You accidentally burn the %s.", fishinfo[f].name);
        } else {
            add_item(fishinfo[f].cooked);
            msg("You successfully cook the %s.", fishinfo[f].name);
            add_xp(SK_COOK, fishinfo[f].cook_xp, true);
        }
        if (raw_fish_kind() < 0) stop_action();
        break;
    }
    case ST_LIGHT: {
        if (--pl.act_timer > 0) break;
        pl.act_timer = 2;
        int fm = level_of(SK_FM);
        int p = 30 + fm * 3 / 2;
        if (p > 90) p = 90;
        if (chance(p)) {
            bool oak = false;
            if (!remove_item(IT_LOGS)) {
                if (remove_item(IT_OAK_LOGS)) oak = true;
                else { stop_action(); break; }
            }
            object[pl.ty][pl.tx] = OBJ_FIRE;
            obj_timer[pl.ty][pl.tx] = 60 + rand() % 40;
            sfx(SND_FIRE);
            msg("The fire catches and the logs begin to burn.");
            add_xp(SK_FM, oak ? 600 : 400, true);
            /* step west like the old days */
            if (tile_walkable(pl.tx - 1, pl.ty)) {
                pl.mtx = pl.tx - 1; pl.mty = pl.ty;
                pl.moving = true;
                pl.facing = 2;
            }
            stop_action();
        }
        break;
    }
    case ST_SMELT: {
        if (object[ay][ax] != OBJ_FURNACE) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 3;
        if (has_item(IT_COPPER) && has_item(IT_TIN)) {
            remove_item(IT_COPPER);
            remove_item(IT_TIN);
            add_item(IT_BRONZE_BAR);
            sfx(SND_FIRE);
            msg("You smelt a bronze bar.");
            add_xp(SK_SMITH, 63, true);
        } else if (has_item(IT_MITH_ORE)) {
            /* mithril needs 2 coal to reach forging heat */
            if (level_of(SK_SMITH) < 30) {
                msg("You need a Smithing level of 30 to smelt mithril.");
                stop_action(); break;
            }
            if (inv_count(IT_COAL) < 2) {
                msg("Mithril needs 2 coal to smelt.");
                stop_action(); break;
            }
            remove_item(IT_MITH_ORE);
            remove_item(IT_COAL);
            remove_item(IT_COAL);
            add_item(IT_MITH_BAR);
            sfx(SND_FIRE);
            msg("You smelt a mithril bar.");
            add_xp(SK_SMITH, 250, true);
        } else if (has_item(IT_IRON) && has_item(IT_COAL)) {
            /* iron + coal makes steel */
            if (level_of(SK_SMITH) < 20) {
                msg("You need a Smithing level of 20 to smelt steel.");
                stop_action(); break;
            }
            remove_item(IT_IRON);
            remove_item(IT_COAL);
            add_item(IT_STEEL_BAR);
            sfx(SND_FIRE);
            msg("You smelt a steel bar.");
            add_xp(SK_SMITH, 175, true);
        } else if (has_item(IT_IRON)) {
            if (level_of(SK_SMITH) < 15) {
                msg("You need a Smithing level of 15 to smelt iron.");
                stop_action();
                break;
            }
            remove_item(IT_IRON);
            sfx(SND_FIRE);
            if (chance(50)) {
                add_item(IT_IRON_BAR);
                msg("You smelt an iron bar.");
                add_xp(SK_SMITH, 125, true);
            } else {
                msg("The iron ore crumbles in the heat. Too impure.");
            }
        } else {
            stop_action();
        }
        break;
    }
    case ST_FIGHT: {
        gob_t *g = &gob[pl.fight_target];
        if (!g->exists || g->dead) { stop_action(); break; }
        int d = cheb(pl.tx, pl.ty, g->tx, g->ty);
        bool magic = (cast_spell != SPELL_MELEE);
        bool ranged = !magic && is_bow(equipped[SLOT_WEAPON - 1]);
        int reach = (magic || ranged) ? 5 : 1;   /* spells and bows strike from afar */
        /* face the foe while fighting */
        if (d >= 1) {
            int fdx = (g->tx > pl.tx) - (g->tx < pl.tx);
            int fdy = (g->ty > pl.ty) - (g->ty < pl.ty);
            if (abs(g->tx - pl.tx) >= abs(g->ty - pl.ty)) {
                if (fdx) pl.facing = fdx > 0 ? 3 : 2;
            } else if (fdy) pl.facing = fdy > 0 ? 0 : 1;
        }
        if (d <= reach) {
            if (--pl.atk_cd <= 0) {
                if (magic) {
                    pl.atk_cd = 5;
                    if (!player_cast(g)) stop_action();
                } else if (ranged) {
                    pl.atk_cd = 4;
                    if (!player_shoot(g)) stop_action();
                } else {
                    pl.atk_cd = 4;
                    player_attack(g);
                }
            }
        } else if (d > ((magic || ranged) ? 8 : 6)) {
            stop_action();
        } else if (!pl.moving) {
            /* close the distance one tile */
            int dx = (g->tx > pl.tx) - (g->tx < pl.tx);
            int dy = (g->ty > pl.ty) - (g->ty < pl.ty);
            int nx = pl.tx + dx, ny = pl.ty + dy;
            if (dx && dy && (!tile_walkable(pl.tx + dx, pl.ty) ||
                             !tile_walkable(pl.tx, pl.ty + dy))) { dy = 0; ny = pl.ty; }
            if (tile_walkable(nx, ny)) {
                pl.mtx = nx; pl.mty = ny;
                pl.moving = true;
                if (dx) pl.facing = dx > 0 ? 3 : 2;
                else if (dy) pl.facing = dy > 0 ? 0 : 1;
            }
        }
        break;
    }
    default: break;
    }
}

/* ------------------------------------------------------------ goblin ticks */

static bool gob_tile_free(int x, int y, gob_t *self)
{
    if (!tile_walkable(x, y)) return false;
    for (int i = 0; i < num_gob; i++) {
        gob_t *o = &gob[i];
        if (o == self || !o->exists || o->dead) continue;
        if ((o->tx == x && o->ty == y) || (o->moving && o->mtx == x && o->mty == y))
            return false;
    }
    return true;
}

static void gob_step(gob_t *g, int tx, int ty)
{
    if (g->moving) return;
    int dx = (tx > g->tx) - (tx < g->tx);
    int dy = (ty > g->ty) - (ty < g->ty);
    if (dx && dy && (!tile_walkable(g->tx + dx, g->ty) ||
                     !tile_walkable(g->tx, g->ty + dy))) {
        if (gob_tile_free(g->tx + dx, g->ty, g)) dy = 0;
        else dx = 0;
    }
    int nx = g->tx + dx, ny = g->ty + dy;
    if ((dx || dy) && gob_tile_free(nx, ny, g)) {
        g->mtx = nx; g->mty = ny;
        g->moving = true;
    }
}

/* the Dragon's signature: a ranged gout of fire that ignores distance and armour */
static void dragon_breath(gob_t *g)
{
    int dmg = g->phase ? 8 : 5;
    spawn_proj(g->px, g->py - 8, pl.px, pl.py - 8, SPR_BOLT_FIRE);
    sfx(SND_FIRE);
    msg("The Ancient Dragon breathes a torrent of fire!");
    float vx = pl.px - g->px, vy = pl.py - g->py;
    bump_set(&g->bump_x, &g->bump_y, &g->bump_t, vx, vy, BUMP_LUNGE);
    bump_set(&pl.bump_x, &pl.bump_y, &pl.bump_t, vx, vy, BUMP_RECOIL);
    hurt_player(dmg);
}

/* the Dragon calls reinforcements when enraged; brood vanish on death */
static void summon_whelps(gob_t *dragon, int n)
{
    static const int off[][2] = {
        {-2,2},{2,2},{-3,0},{3,0},{0,3},{-2,-1},{2,-1}
    };
    int got = 0;
    for (int k = 0; k < 7 && got < n; k++) {
        int sx = dragon->sx + off[k][0], sy = dragon->sy + off[k][1];
        if (!gob_tile_free(sx, sy, dragon)) continue;
        gob_t *w = spawn_mob(MOB_WHELP, sx, sy);
        if (!w) break;
        w->summoned = true;
        w->aggro = true;
        got++;
    }
    if (got) msg("The Dragon calls its brood to its defence!");
}

static void tick_goblins(void)
{
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists) continue;
        if (g->dead) {
            if (--g->respawn <= 0 && gob_tile_free(g->sx, g->sy, g)) {
                g->dead = false;
                g->hp = mobinfo[g->type].max_hp;
                g->tx = g->mtx = g->sx;
                g->ty = g->mty = g->sy;
                g->px = g->sx * TILE + 8; g->py = g->sy * TILE + 12;
                g->moving = false;
                g->aggro = false;
            }
            continue;
        }
        if (g->hurt_timer > 0) g->hurt_timer--;

        int dp = cheb(g->tx, g->ty, pl.tx, pl.ty);
        if (!g->aggro && dp <= mobinfo[g->type].aggro) g->aggro = true;

        if (g->aggro) {
            if (cheb(g->tx, g->ty, g->sx, g->sy) > 10) {
                g->aggro = false;
            } else {
                if (g->type == MOB_DRAGON) {
                    /* enrage once at half health: faster, fiercer, summons brood */
                    if (g->phase == 0 && g->hp <= mobinfo[MOB_DRAGON].max_hp / 2) {
                        g->phase = 1;
                        msg("The Ancient Dragon roars and erupts in fury!");
                        summon_whelps(g, 2);
                    }
                    /* breathe fire at range (and up close) on a cooldown */
                    if (dp <= 5 && --g->special_cd <= 0) {
                        g->special_cd = g->phase ? 4 : 7;
                        dragon_breath(g);
                    }
                }
                if (dp <= 1) {
                    int cd = (g->type == MOB_DRAGON && g->phase) ? 3 : 4;
                    if (--g->atk_cd <= 0) {
                        g->atk_cd = cd;
                        mob_attack(g);
                    }
                } else {
                    gob_step(g, pl.tx, pl.ty);
                }
            }
        } else {
            if (g->tx != g->sx || g->ty != g->sy) {
                if (cheb(g->tx, g->ty, g->sx, g->sy) > 3) {
                    gob_step(g, g->sx, g->sy);
                    continue;
                }
            }
            if (--g->wander_cd <= 0) {
                g->wander_cd = 3 + rand() % 7;
                int dx = rand() % 3 - 1, dy = rand() % 3 - 1;
                int nx = g->tx + dx, ny = g->ty + dy;
                if (cheb(nx, ny, g->sx, g->sy) <= 3)
                    gob_step(g, nx, ny);
            }
        }
    }
}

/* ------------------------------------------------------------ player bots */

#define MAX_BOT 5
#define SAY_LEN 32

enum { BS_IDLE, BS_GOTO, BS_WORK };
enum { GK_HARVEST, GK_WANDER, GK_BANK, GK_FIRE };

typedef struct {
    int   tx, ty;
    float px, py;
    int   mtx, mty;
    bool  moving;
    int   facing;
    float walk_anim;
    int   look;                 /* sprite recolor set 0-2 */
    const char *name;
    int   state;
    int   goal_x, goal_y, goal_obj, goal_kind;
    int   work_left, act_timer, idle_ticks, stuck;
    char  say[SAY_LEN];
    int   say_t;                /* frames of overhead text left */
    int   chat_cd;              /* ticks until next random line */
} bot_t;

static bot_t bots[MAX_BOT];

static const char *bot_lines[] = {
    "selling logs 25gp ea",
    "buying shrimp in bulk",
    "anyone seen iron rocks?",
    "free armor trimming",
    "dancing 4 money",
    "follow me 4 a suprise",
    "wc 99 someday, trust",
    "essence is gud xp",
    "lost my axe in the river",
    "nice valley innit",
};
#define NUM_BOT_LINES (int)(sizeof bot_lines / sizeof bot_lines[0])

static const char *gratz_lines[] = { "gz", "grats", "nice one", "gz m8" };

static void bot_say(bot_t *b, const char *text)
{
    snprintf(b->say, SAY_LEN, "%s", text);
    b->say_t = 240;
    msg("%s: %s", b->name, text);
}

static bool bot_goal_valid(bot_t *b)
{
    return object[b->goal_y][b->goal_x] == b->goal_obj;
}

static void bot_pick_goal(bot_t *b)
{
    b->stuck = 0;
    int roll = rand() % 100;
    /* wander somewhere walkable nearby */
    if (roll < 20) {
        for (int tries = 0; tries < 8; tries++) {
            int x = b->tx + rand() % 13 - 6, y = b->ty + rand() % 13 - 6;
            if (tile_walkable(x, y)) {
                b->goal_x = x; b->goal_y = y; b->goal_obj = OBJ_NONE;
                b->goal_kind = GK_WANDER;
                b->state = BS_GOTO;
                return;
            }
        }
        b->state = BS_IDLE; b->idle_ticks = 2 + rand() % 4;
        return;
    }
    /* trip to the bank */
    if (roll < 32) {
        b->goal_x = 20 + rand() % 9; b->goal_y = 6;
        b->goal_obj = OBJ_BOOTH;
        b->goal_kind = GK_BANK;
        b->state = BS_GOTO;
        return;
    }
    /* light a fire somewhere open */
    if (roll < 40) {
        for (int tries = 0; tries < 8; tries++) {
            int x = b->tx + rand() % 9 - 4, y = b->ty + rand() % 9 - 4;
            if (!tile_walkable(x, y) || object[y][x] != OBJ_NONE) continue;
            int t = terrain[y][x];
            if (t == TER_GRASS || t == TER_PATH || t == TER_SAND) {
                b->goal_x = x; b->goal_y = y; b->goal_obj = OBJ_NONE;
                b->goal_kind = GK_FIRE;
                b->state = BS_GOTO;
                return;
            }
        }
        b->state = BS_IDLE; b->idle_ticks = 2 + rand() % 4;
        return;
    }
    /* otherwise find something to harvest */
    b->goal_kind = GK_HARVEST;
    static int cx[64], cy[64];
    int n = 0;
    for (int y = 1; y < MAP_H - 1 && n < 64; y++)
        for (int x = 1; x < MAP_W - 1 && n < 64; x++) {
            int o = object[y][x];
            if (o != OBJ_TREE && o != OBJ_OAK && o != OBJ_ROCK_COPPER &&
                o != OBJ_ROCK_TIN && o != OBJ_ROCK_IRON && o != OBJ_ESSENCE &&
                o != OBJ_FISH) continue;
            if (cheb(x, y, b->tx, b->ty) > 16) continue;
            cx[n] = x; cy[n] = y; n++;
        }
    if (!n) { b->state = BS_IDLE; b->idle_ticks = 4 + rand() % 6; return; }
    int pick = rand() % n;
    b->goal_x = cx[pick]; b->goal_y = cy[pick];
    b->goal_obj = object[cy[pick]][cx[pick]];
    b->state = BS_GOTO;
}

static void bot_step(bot_t *b, int tx, int ty)
{
    if (b->moving) return;
    int dx = (tx > b->tx) - (tx < b->tx);
    int dy = (ty > b->ty) - (ty < b->ty);
    if (dx && dy && (!tile_walkable(b->tx + dx, b->ty) ||
                     !tile_walkable(b->tx, b->ty + dy))) {
        if (tile_walkable(b->tx + dx, b->ty)) dy = 0;
        else dx = 0;
    }
    int nx = b->tx + dx, ny = b->ty + dy;
    if ((dx || dy) && tile_walkable(nx, ny)) {
        b->mtx = nx; b->mty = ny;
        b->moving = true;
        if (dx) b->facing = dx > 0 ? 3 : 2;
        else if (dy) b->facing = dy > 0 ? 0 : 1;
    } else {
        b->stuck++;
    }
}

static void bot_face_goal(bot_t *b)
{
    int dx = b->goal_x - b->tx, dy = b->goal_y - b->ty;
    if (abs(dx) >= abs(dy)) { if (dx) b->facing = dx > 0 ? 3 : 2; }
    else b->facing = dy > 0 ? 0 : 1;
}

static void tick_bots(void)
{
    for (int i = 0; i < MAX_BOT; i++) {
        bot_t *b = &bots[i];

        /* chatter */
        bool near = cheb(b->tx, b->ty, pl.tx, pl.ty) <= 14;
        if (b->say_t == 0 && gratz_timer > 0 && near && chance(35)) {
            bot_say(b, gratz_lines[rand() % 4]);
        } else if (--b->chat_cd <= 0) {
            b->chat_cd = 50 + rand() % 120;
            if (near) bot_say(b, bot_lines[rand() % NUM_BOT_LINES]);
        }

        switch (b->state) {
        case BS_IDLE:
            if (--b->idle_ticks <= 0) bot_pick_goal(b);
            break;
        case BS_GOTO: {
            if (b->goal_kind == GK_HARVEST && !bot_goal_valid(b)) { bot_pick_goal(b); break; }
            if (b->goal_kind == GK_FIRE && object[b->goal_y][b->goal_x] != OBJ_NONE) { bot_pick_goal(b); break; }
            bool arrived = (b->goal_kind == GK_HARVEST || b->goal_kind == GK_BANK)
                    ? cheb(b->tx, b->ty, b->goal_x, b->goal_y) <= 1
                    : (b->tx == b->goal_x && b->ty == b->goal_y);
            if (arrived) {
                switch (b->goal_kind) {
                case GK_WANDER:
                    b->state = BS_IDLE;
                    b->idle_ticks = 2 + rand() % 5;
                    break;
                case GK_BANK:
                    b->state = BS_WORK;
                    bot_face_goal(b);
                    b->act_timer = 4;
                    b->work_left = 2 + rand() % 3;
                    break;
                case GK_FIRE:
                    b->state = BS_WORK;
                    b->act_timer = 2;
                    b->work_left = 2;
                    break;
                default:
                    b->state = BS_WORK;
                    bot_face_goal(b);
                    b->act_timer = 4;
                    b->work_left = 5 + rand() % 10;
                    break;
                }
                break;
            }
            /* route through the bank door when entering or leaving */
            int tx = b->goal_x, ty = b->goal_y;
            bool here_in = (b->ty <= 8 && b->tx >= 19 && b->tx <= 29);
            bool goal_in = (ty <= 8 && tx >= 19 && tx <= 29);
            if (here_in != goal_in) { tx = 24; ty = here_in ? 9 : 8; }
            bot_step(b, tx, ty);
            if (b->stuck > 4) bot_pick_goal(b);
            break;
        }
        case BS_WORK: {
            if (b->goal_kind == GK_HARVEST && !bot_goal_valid(b)) {
                b->state = BS_IDLE; b->idle_ticks = 1 + rand() % 3;
                break;
            }
            if (--b->act_timer > 0) break;
            b->act_timer = 4;
            if (b->goal_kind == GK_HARVEST && chance(45)) {
                /* bots take resources for real */
                int gx = b->goal_x, gy = b->goal_y;
                switch (b->goal_obj) {
                case OBJ_TREE:
                    object[gy][gx] = OBJ_STUMP; obj_timer[gy][gx] = 10; break;
                case OBJ_OAK:
                    if (chance(13)) { object[gy][gx] = OBJ_STUMP; obj_timer[gy][gx] = 14; }
                    break;
                case OBJ_ROCK_COPPER: case OBJ_ROCK_TIN:
                    object[gy][gx] = OBJ_ROCK_EMPTY; obj_timer[gy][gx] = 4; break;
                case OBJ_ROCK_IRON:
                    object[gy][gx] = OBJ_ROCK_EMPTY; obj_timer[gy][gx] = 9; break;
                default: break;     /* essence + fish never deplete */
                }
            }
            if (--b->work_left <= 0) {
                if (b->goal_kind == GK_FIRE) {
                    int t = terrain[b->ty][b->tx];
                    if (object[b->ty][b->tx] == OBJ_NONE &&
                        (t == TER_GRASS || t == TER_PATH || t == TER_SAND)) {
                        object[b->ty][b->tx] = OBJ_FIRE;
                        obj_timer[b->ty][b->tx] = 60 + rand() % 40;
                        if (near && chance(50)) bot_say(b, "free fire, gather round");
                        if (tile_walkable(b->tx - 1, b->ty)) {
                            b->mtx = b->tx - 1; b->mty = b->ty;
                            b->moving = true;
                            b->facing = 2;
                        }
                    }
                } else if (b->goal_kind == GK_BANK && near && chance(40)) {
                    bot_say(b, "bank space is a myth");
                }
                b->state = BS_IDLE;
                b->idle_ticks = 3 + rand() % 6;
            }
            break;
        }
        }
    }
    if (gratz_timer > 0) gratz_timer--;
}

static void init_bots(void)
{
    static const struct { int x, y, look; const char *name; } seed[MAX_BOT] = {
        { 14, 14, 0, "Sir Logsalot" },
        {  8, 24, 1, "rockcrushr7"  },
        { 35, 26, 2, "shrimp4lyfe"  },
        {  8,  4, 1, "xXEssenceXx"  },
        { 20, 18, 0, "Lumber Jacq"  },
    };
    for (int i = 0; i < MAX_BOT; i++) {
        bot_t *b = &bots[i];
        memset(b, 0, sizeof *b);
        b->tx = b->mtx = seed[i].x;
        b->ty = b->mty = seed[i].y;
        b->px = b->tx * TILE + 8; b->py = b->ty * TILE + 12;
        b->look = seed[i].look;
        b->name = seed[i].name;
        b->state = BS_IDLE;
        b->idle_ticks = 1 + i;
        b->chat_cd = 20 + i * 37;
    }
}

/* ------------------------------------------------------------ world tick */

static void game_tick(void)
{
    /* object respawns / fire burnout */
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            if (obj_timer[y][x] > 0 && --obj_timer[y][x] == 0) {
                if (object[y][x] == OBJ_FIRE)
                    object[y][x] = OBJ_NONE;
                else
                    object[y][x] = obj_orig[y][x];
            }
        }

    tick_skilling();
    tick_goblins();
    if (cur_map == MAP_OVERWORLD) tick_bots();   /* bots stay on the surface */

    /* resolve a death now that the combat loops are done (safe to reload map) */
    if (pending_death) {
        pending_death = false;
        player_die();
        return;
    }

    /* hp regen: 1 per minute */
    if (pl.hp < level_of(SK_HP) && ++pl.regen >= 100) {
        pl.regen = 0;
        pl.hp++;
    }
    /* prayer points: active prayers drain, otherwise a slow trickle restores.
       Visit an altar for an instant top-up. */
    {
        int drain = 0;
        for (int i = 0; i < NUM_PRAYERS; i++)
            if (pray_on[i]) drain += prayerinfo[i].drain;
        if (drain > 0) {
            pray_drain_acc += drain;
            while (pray_drain_acc >= 120 && pray_pts > 0) {
                pray_drain_acc -= 120;
                pray_pts--;
            }
            if (pray_pts <= 0) {
                pray_pts = 0;
                pray_drain_acc = 0;
                deactivate_all_prayers();
                msg("You have run out of prayer points.");
            }
        } else if (pray_pts < pray_max() && ++pray_regen >= 50) {
            pray_regen = 0;
            pray_pts++;
        }
    }
    /* silent autosave every ~60s of overworld play (saves never record the
       dungeon, so you always reload on the surface) */
    if (game_state == STATE_PLAY && cur_map == MAP_OVERWORLD && ++save_timer >= 100) {
        save_timer = 0;
        save_game();
    }
    /* run energy regen */
    if (!pl.moving || !pl.run_on) {
        pl.run_energy += 2;
        if (pl.run_energy > 100) pl.run_energy = 100;
    }
    if (pl.eat_cd > 0) pl.eat_cd--;
}

/* ------------------------------------------------------------ interactions */

typedef struct { int x, y, obj; } target_t;

/* find the best interaction target around the player */
static bool find_target(target_t *out)
{
    static const int fdx[4] = { 0, 0, -1, 1 };
    static const int fdy[4] = { 1, -1, 0, 0 };
    /* facing tile first, then the 8 neighbours */
    int order_x[9], order_y[9], n = 0;
    order_x[n] = pl.tx + fdx[pl.facing]; order_y[n] = pl.ty + fdy[pl.facing]; n++;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy) continue;
            order_x[n] = pl.tx + dx; order_y[n] = pl.ty + dy; n++;
        }
    for (int i = 0; i < n; i++) {
        int x = order_x[i], y = order_y[i];
        if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) continue;
        int o = object[y][x];
        if (o == OBJ_NONE || o == OBJ_STUMP) continue;
        out->x = x; out->y = y; out->obj = o;
        return true;
    }
    /* fire under our feet (we may stand on it) */
    if (object[pl.ty][pl.tx] == OBJ_FIRE) {
        out->x = pl.tx; out->y = pl.ty; out->obj = OBJ_FIRE;
        return true;
    }
    return false;
}

static gob_t *adjacent_goblin(void)
{
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists || g->dead) continue;
        if (cheb(pl.tx, pl.ty, g->tx, g->ty) <= 1) return g;
    }
    return NULL;
}

static gob_t *nearest_goblin(int maxdist)
{
    gob_t *best = NULL;
    int bestd = maxdist + 1;
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists || g->dead) continue;
        int d = cheb(pl.tx, pl.ty, g->tx, g->ty);
        if (d < bestd) { bestd = d; best = g; }
    }
    return best;
}

static void dlg_line(const char *text)
{
    if (dlg_count < 4) snprintf(dlg_buf[dlg_count++], sizeof dlg_buf[0], "%s", text);
}

static void chef_talk(void)
{
    dlg_title = "Chef Bouillon";
    dlg_count = 0;
    switch (quest_state) {
    case QUEST_NONE:
        dlg_line("Psst! Those rotten goblins across the");
        dlg_line("river keep raiding my kitchen stores!");
        dlg_line("Bash 3 of them, then bring me 2 cooked");
        dlg_line("shrimp to restock. I'll reward you well!");
        quest_state = QUEST_ACTIVE;
        quest_kills = 0;
        msg("Quest started: The Chef's Little Problem.");
        break;
    case QUEST_ACTIVE:
        if (quest_kills >= QUEST_KILLS_NEEDED &&
            inv_count(IT_SHRIMP) >= QUEST_SHRIMP_NEEDED) {
            remove_item(IT_SHRIMP);
            remove_item(IT_SHRIMP);
            quest_state = QUEST_DONE;
            dlg_line("You're a hero! My kitchen is safe and");
            dlg_line("my shrimp are back on the menu.");
            dlg_line("Let me teach you a few tricks of the");
            dlg_line("trade. Do come back for a meal!");
            sfx_ui(SND_LEVELUP);
            msg("Quest complete: The Chef's Little Problem!");
            add_xp(SK_COOK, 3000, false);
            add_xp(SK_ATT, 3000, false);
            msg("The Chef rewards your Cooking and Attack.");
        } else {
            char buf[44];
            snprintf(buf, sizeof buf, "Goblins bashed: %d of %d.",
                     quest_kills, QUEST_KILLS_NEEDED);
            dlg_line(buf);
            snprintf(buf, sizeof buf, "Cooked shrimp with you: %d of %d.",
                     inv_count(IT_SHRIMP), QUEST_SHRIMP_NEEDED);
            dlg_line(buf);
            dlg_line("Hop to it, friend! My customers grow");
            dlg_line("hungrier by the minute.");
        }
        break;
    default:
        dlg_line("My shrimp empire flourishes once more,");
        dlg_line("thanks to you. You're always welcome");
        dlg_line("at my table, hero.");
        break;
    }
    ui_mode = UI_DIALOG;
}

static void knight_talk(void)
{
    dlg_title = "Sir Garrick";
    dlg_count = 0;
    switch (wquest) {
    case WQ_NONE:
        if (level_of(SK_ATT) < WQ_REQ_ATT || level_of(SK_DEF) < WQ_REQ_DEF ||
            level_of(SK_SMITH) < WQ_REQ_SMITH) {
            char b[44];
            dlg_line("This is no errand for a novice. Come");
            dlg_line("back when you can fight and forge:");
            snprintf(b, sizeof b, "Need Att %d, Def %d, Smith %d.",
                     WQ_REQ_ATT, WQ_REQ_DEF, WQ_REQ_SMITH);
            dlg_line(b);
            snprintf(b, sizeof b, "You: Att %d, Def %d, Smith %d.",
                     level_of(SK_ATT), level_of(SK_DEF), level_of(SK_SMITH));
            dlg_line(b);
            break;
        }
        dlg_line("The Goblin Warlord festers in the dungeon");
        dlg_line("and I am too grey to make the descent.");
        dlg_line("Prove your nerve: bring me 5 bones from");
        dlg_line("the creatures that haunt those depths.");
        wquest = WQ_STARTED;
        msg("Quest started: The Warlord's Bane.");
        break;
    case WQ_STARTED:
        if (inv_count(IT_BONES) >= WQ_BONES_NEEDED) {
            for (int i = 0; i < WQ_BONES_NEEDED; i++) remove_item(IT_BONES);
            wquest = WQ_SCOUTED;
            dlg_line("Steady hands. Now, the Warlord's hide");
            dlg_line("turns aside common steel. Bring me 2");
            dlg_line("iron bars and 300 coins, and I'll forge");
            dlg_line("you a blade with a keener bite.");
            msg("You hand over the bones.");
        } else {
            char b[44];
            snprintf(b, sizeof b, "Bring me %d bones (you have %d).",
                     WQ_BONES_NEEDED, inv_count(IT_BONES));
            dlg_line(b);
            dlg_line("The dungeon lies past the cave mouth");
            dlg_line("southwest of the mine.");
        }
        break;
    case WQ_SCOUTED:
        if (inv_count(IT_IRON_BAR) >= WQ_BARS_NEEDED && gp >= WQ_COIN_COST) {
            for (int i = 0; i < WQ_BARS_NEEDED; i++) remove_item(IT_IRON_BAR);
            gp -= WQ_COIN_COST;
            add_item(IT_BANE);
            wquest = WQ_ARMED;
            dlg_line("Take it - the Warlord's Bane. Wield it");
            dlg_line("well. Now descend, and do not return");
            dlg_line("until the beast lies cold. Go!");
            sfx_ui(SND_LEVELUP);
            msg("Sir Garrick gives you the Warlord's Bane!");
        } else {
            char b[44];
            snprintf(b, sizeof b, "I need %d iron bars (you have %d)",
                     WQ_BARS_NEEDED, inv_count(IT_IRON_BAR));
            dlg_line(b);
            snprintf(b, sizeof b, "and %d coins (you have %d).",
                     WQ_COIN_COST, gp);
            dlg_line(b);
            dlg_line("Smelt iron at the furnace by the mine.");
        }
        break;
    case WQ_ARMED:
        dlg_line("The Warlord still draws breath! Take the");
        dlg_line("Bane into the dungeon and end him.");
        break;
    case WQ_SLAIN:
        wquest = WQ_DONE;
        dlg_line("Slain! You've lifted a shadow from the");
        dlg_line("whole valley. Take this purse, and my");
        dlg_line("deepest thanks, champion.");
        sfx_ui(SND_LEVELUP);
        gp += 2000;
        add_xp(SK_ATT, 6000, false);
        add_xp(SK_STR, 6000, false);
        add_xp(SK_DEF, 6000, false);
        add_xp(SK_HP,  5000, false);
        msg("Quest complete: The Warlord's Bane!");
        msg("Sir Garrick rewards you with 2000 coins.");
        break;
    default:   /* WQ_DONE - now the follow-up quest, The Demon Below */
        switch (dquest) {
        case DQ_NONE:
            if (level_of(SK_ATT) < DQ_REQ_ATT || level_of(SK_DEF) < DQ_REQ_DEF ||
                level_of(SK_HP) < DQ_REQ_HP) {
                char b[44];
                dlg_line("The Warlord guarded a sealed stair. A");
                dlg_line("Demon stirs below - but only a master");
                snprintf(b, sizeof b, "may face it. Need Att %d, Def %d, HP %d.",
                         DQ_REQ_ATT, DQ_REQ_DEF, DQ_REQ_HP);
                dlg_line(b);
                snprintf(b, sizeof b, "You: Att %d, Def %d, HP %d.",
                         level_of(SK_ATT), level_of(SK_DEF), level_of(SK_HP));
                dlg_line(b);
                break;
            }
            dlg_line("With the Warlord gone, a sealed stair");
            dlg_line("lies open in his chamber - and a Demon");
            dlg_line("rises from the deep. Descend and destroy");
            dlg_line("it, champion. This is the true test.");
            dquest = DQ_STARTED;
            msg("Quest started: The Demon Below.");
            break;
        case DQ_STARTED:
            dlg_line("The Demon yet lives. Take the stair in");
            dlg_line("the Warlord's chamber down to the depths");
            dlg_line("and end its reign of brimstone.");
            break;
        case DQ_SLAIN:
            dquest = DQ_DONE;
            dlg_line("The Demon, destroyed! No darkness remains");
            dlg_line("in these lands that you have not bested.");
            dlg_line("Take this fortune - you have more than");
            dlg_line("earned it, greatest of Rune Valley.");
            sfx_ui(SND_LEVELUP);
            gp += 5000;
            add_xp(SK_ATT, 12000, false);
            add_xp(SK_DEF, 12000, false);
            add_xp(SK_HP,  10000, false);
            msg("Quest complete: The Demon Below!");
            msg("Sir Garrick rewards you with 5000 coins.");
            break;
        default:
            dlg_line("A stair has opened where the Demon fell.");
            dlg_line("An Ancient Dragon sleeps in the deep -");
            dlg_line("the last terror of these lands. Go in");
            dlg_line("rune armour, and ware its fire!");
            break;
        }
        break;
    }
    ui_mode = UI_DIALOG;
}

/* Sergeant Hardy: the combat tutor at the cow pasture. His quest, Basic
   Training, has you fell five cows; finishing it drills a jumpstart of combat
   xp into you. The CQ_DONE state makes the reward a one-shot. */
static void tutor_talk(void)
{
    dlg_title = "Sergeant Hardy";
    dlg_count = 0;
    switch (cquest) {
    case CQ_NONE:
        dlg_line("New blood? Then earn your stripes.");
        dlg_line("Fell five of these cows to prove you");
        dlg_line("can swing a blade - then I'll drill");
        dlg_line("some real fight into your arms.");
        cquest = CQ_STARTED;
        cow_kills = 0;
        msg("Quest started: Basic Training.");
        break;
    case CQ_STARTED:
        if (cow_kills >= CQ_COWS_NEEDED) {
            cquest = CQ_DONE;
            add_xp(SK_ATT, 2000, false);
            add_xp(SK_STR, 2000, false);
            add_xp(SK_DEF, 2000, false);
            add_xp(SK_HP,   800, false);
            gp += 50;
            sfx_ui(SND_LEVELUP);
            gratz_timer = 4;
            dlg_line("Hah - a natural! There, I've drilled");
            dlg_line("the basics into your arms and legs.");
            dlg_line("Train on the herd to sharpen up, then");
            dlg_line("see Sir Garrick by the cave for war.");
            msg("Quest complete: Basic Training!");
        } else {
            dlg_line("Not done yet, recruit. Fresh cows");
            dlg_line("wander in fast - keep swinging until");
            dlg_line("five of them lie still in the field.");
            msg("Basic Training: %d of %d cows felled.",
                cow_kills, CQ_COWS_NEEDED);
        }
        break;
    default:   /* CQ_DONE */
        dlg_line("Good drilling, soldier. Train on the");
        dlg_line("cows whenever you like - they barely");
        dlg_line("kick back. Then take the fight to the");
        dlg_line("goblins, and the dungeon below.");
        break;
    }
    ui_mode = UI_DIALOG;
}

/* the Tanner cures hides into leather for a coin fee per hide */
#define TAN_COW_FEE 3
#define TAN_DRAGON_FEE 20
static void tanner_talk(void)
{
    dlg_title = "Pelt the Tanner";
    dlg_count = 0;
    int cow = inv_count(IT_COWHIDE), drag = inv_count(IT_DRAGON_HIDE);
    if (cow == 0 && drag == 0) {
        dlg_line("Bring me cowhide and I'll cure it into");
        dlg_line("leather - stitch that with a needle and");
        dlg_line("thread for armour. Dragonhide too, if");
        dlg_line("you're bold enough to skin one.");
        ui_mode = UI_DIALOG;
        return;
    }
    int tanned = 0, spent = 0;
    while (inv_count(IT_COWHIDE) > 0 && gp >= TAN_COW_FEE) {
        remove_item(IT_COWHIDE); gp -= TAN_COW_FEE; add_item(IT_LEATHER);
        tanned++; spent += TAN_COW_FEE;
    }
    while (inv_count(IT_DRAGON_HIDE) > 0 && gp >= TAN_DRAGON_FEE) {
        remove_item(IT_DRAGON_HIDE); gp -= TAN_DRAGON_FEE;
        add_item(IT_DRAGON_LEATHER);
        tanned++; spent += TAN_DRAGON_FEE;
    }
    if (tanned > 0) {
        char b[64];
        snprintf(b, sizeof b, "There - %d hide%s cured for %d coins.",
                 tanned, tanned == 1 ? "" : "s", spent);
        dlg_line(b);
        dlg_line("Fine work. Stitch it into ranged armour");
        dlg_line("with a needle and thread.");
        sfx_ui(SND_CRAFT);
    } else {
        dlg_line("Tanning isn't free, friend. Cowhide is");
        char b[64];
        snprintf(b, sizeof b, "%d coins a hide, dragonhide %d.",
                 TAN_COW_FEE, TAN_DRAGON_FEE);
        dlg_line(b);
        dlg_line("Come back when your purse is heavier.");
    }
    ui_mode = UI_DIALOG;
}

static void interact(void)
{
    /* with a spell selected, or a bow drawn, A targets a monster from range */
    bool ranged = (cast_spell == SPELL_MELEE) && is_bow(equipped[SLOT_WEAPON - 1]);
    if (cast_spell != SPELL_MELEE || ranged) {
        gob_t *g = nearest_goblin(5);
        if (g) {
            pl.state = ST_FIGHT;
            pl.fight_target = (int)(g - gob);
            pl.atk_cd = 1;
            if (ranged) msg("You take aim at the %s.", mobinfo[g->type].name);
            else msg("You aim your %s at the %s.", spellinfo[cast_spell].name,
                     mobinfo[g->type].name);
            return;
        }
    }
    gob_t *g = adjacent_goblin();
    if (g) {
        pl.state = ST_FIGHT;
        pl.fight_target = (int)(g - gob);
        pl.atk_cd = 2;
        msg("You attack the %s!", mobinfo[g->type].name);
        return;
    }
    target_t t;
    if (!find_target(&t)) return;
    pl.act_x = t.x; pl.act_y = t.y;

    /* face the target */
    if (t.x != pl.tx || t.y != pl.ty) {
        int dx = t.x - pl.tx, dy = t.y - pl.ty;
        if (abs(dx) >= abs(dy)) pl.facing = dx > 0 ? 3 : 2;
        else pl.facing = dy > 0 ? 0 : 1;
    }

    switch (t.obj) {
    case OBJ_TREE: case OBJ_OAK:
        if (!has_axe()) { msg("You need an axe to chop down this tree."); return; }
        pl.state = ST_CHOP; pl.act_timer = 2;
        msg("You swing your axe at the tree.");
        break;
    case OBJ_ROCK_COPPER: case OBJ_ROCK_TIN: case OBJ_ROCK_IRON:
    case OBJ_ROCK_MITHRIL: case OBJ_ROCK_COAL: case OBJ_ESSENCE:
        if (!has_pick()) { msg("You need a pickaxe to mine this rock."); return; }
        pl.state = ST_MINE; pl.act_timer = 2;
        msg("You swing your pick at the rock.");
        break;
    case OBJ_ALTAR_AIR:   case OBJ_ALTAR_WATER: case OBJ_ALTAR_EARTH:
    case OBJ_ALTAR_FIRE:  case OBJ_ALTAR_CHAOS: case OBJ_ALTAR_LAW: {
        int rune, rcreq, rcxp; const char *rname;
        switch (t.obj) {
        case OBJ_ALTAR_WATER: rune=IT_WATER_RUNE; rcreq=5;  rcxp=55;  rname="water"; break;
        case OBJ_ALTAR_EARTH: rune=IT_EARTH_RUNE; rcreq=9;  rcxp=60;  rname="earth"; break;
        case OBJ_ALTAR_FIRE:  rune=IT_FIRE_RUNE;  rcreq=14; rcxp=70;  rname="fire";  break;
        case OBJ_ALTAR_CHAOS: rune=IT_CHAOS_RUNE; rcreq=20; rcxp=90;  rname="chaos"; break;
        case OBJ_ALTAR_LAW:   rune=IT_LAW_RUNE;   rcreq=25; rcxp=100; rname="law";   break;
        default:              rune=IT_AIR_RUNE;   rcreq=1;  rcxp=50;  rname="air";   break;
        }
        /* with no essence to bind, any altar restores your prayer instead */
        if (!has_item(IT_ESSENCE)) {
            if (pray_pts >= pray_max()) {
                msg("You feel spiritually full already.");
                return;
            }
            pray_pts = pray_max();
            sfx_ui(SND_CRAFT);
            msg("You pray at the altar; your prayer is restored.");
            return;
        }
        if (level_of(SK_RC) < rcreq) {
            msg("You need a Runecraft level of %d to bind %s runes.", rcreq, rname);
            return;
        }
        int n = 0;
        while (remove_item(IT_ESSENCE)) {
            add_item(rune);
            n++;
        }
        sfx_ui(SND_CRAFT);
        msg("You bind the temple's power into %s rune%s.", rname, n > 1 ? "s" : "");
        add_xp(SK_RC, n * rcxp, true);
        break;
    }
    case OBJ_ROCK_EMPTY:
        msg("There is no ore currently available in this rock.");
        break;
    case OBJ_FISH:
        if (pick_fish() < 0) {
            msg("You need fishing tackle - a net, rod, pot or harpoon.");
            return;
        }
        pl.state = ST_FISH; pl.act_timer = 3;
        msg("You cast out your line...");
        break;
    case OBJ_BOOTH:
        ui_mode = UI_BANK;
        bank_cursor = 0;
        break;
    case OBJ_FIRE:
        if (raw_fish_kind() >= 0) {
            pl.state = ST_COOK; pl.act_timer = 2;
            msg("You set your catch over the fire.");
        } else {
            msg("The fire is nice and warm.");
        }
        break;
    case OBJ_FURNACE:
        if ((has_item(IT_COPPER) && has_item(IT_TIN)) || has_item(IT_IRON) ||
            has_item(IT_MITH_ORE)) {
            pl.state = ST_SMELT; pl.act_timer = 2;
            msg("You feed ore into the furnace.");
        } else {
            msg("You need ores to smelt. Copper and tin make bronze.");
        }
        break;
    case OBJ_ANVIL:
        if (!has_item(IT_HAMMER)) {
            msg("You need a hammer to work the anvil.");
        } else if (!has_item(IT_BRONZE_BAR) && !has_item(IT_IRON_BAR) &&
                   !has_item(IT_MITH_BAR)) {
            msg("You need metal bars to smith. Smelt ore at the furnace.");
        } else {
            ui_mode = UI_SMITH;
            smith_cursor = 0;
        }
        break;
    case OBJ_CHEF:
        chef_talk();
        break;
    case OBJ_KNIGHT:
        knight_talk();
        break;
    case OBJ_TUTOR:
        tutor_talk();
        break;
    case OBJ_TANNER:
        tanner_talk();
        break;
    case OBJ_STAIRS_DOWN:
        if (cur_map == MAP_OVERWORLD) {
            if (wquest == WQ_NONE)
                msg("The dungeon is perilous. Speak with Sir Garrick first.");
            else
                enter_dungeon();
        } else if (dungeon_floor == 1) {       /* floor 1 -> floor 2 */
            if (dquest == DQ_NONE)
                msg("A sealed stair. Sir Garrick must know of this.");
            else
                descend_floor();
        } else {                               /* floor 2 -> floor 3 */
            descend_floor();   /* the stair only exists once the Demon is bested */
        }
        break;
    case OBJ_STAIRS_UP:
        ascend_floor();
        break;
    case OBJ_SHOP_GENERAL: case OBJ_SHOP_WEAPON:
    case OBJ_SHOP_ARMOR:   case OBJ_SHOP_MAGIC:
        shop_id = t.obj - OBJ_SHOP_GENERAL;
        shop_mode = 0; shop_cursor = 0;
        ui_mode = UI_SHOP;
        break;
    }
}

/* hint string for the contextual action */
static const char *context_hint(void)
{
    gob_t *am = adjacent_goblin();
    if (am) {
        static char buf[32];
        snprintf(buf, sizeof buf, "A: Attack %s", mobinfo[am->type].name);
        return buf;
    }
    target_t t;
    if (!find_target(&t)) return NULL;
    switch (t.obj) {
    case OBJ_TREE:        return "A: Chop down Tree";
    case OBJ_OAK:         return "A: Chop down Oak";
    case OBJ_ROCK_COPPER: return "A: Mine Copper rock";
    case OBJ_ROCK_TIN:    return "A: Mine Tin rock";
    case OBJ_ROCK_IRON:   return "A: Mine Iron rock";
    case OBJ_ROCK_MITHRIL:return "A: Mine Mithril rock";
    case OBJ_ROCK_COAL:   return "A: Mine Coal rock";
    case OBJ_FISH:        return "A: Fish here";
    case OBJ_BOOTH:       return "A: Use Bank booth";
    case OBJ_FIRE:        return "A: Cook on Fire";
    case OBJ_ESSENCE:     return "A: Mine Essence rock";
    case OBJ_ALTAR_AIR:   return "A: Craft runes at Air altar";
    case OBJ_ALTAR_WATER: return "A: Craft runes at Water altar";
    case OBJ_ALTAR_EARTH: return "A: Craft runes at Earth altar";
    case OBJ_ALTAR_FIRE:  return "A: Craft runes at Fire altar";
    case OBJ_ALTAR_CHAOS: return "A: Craft runes at Chaos altar";
    case OBJ_ALTAR_LAW:   return "A: Craft runes at Law altar";
    case OBJ_FURNACE:     return "A: Smelt ore at Furnace";
    case OBJ_ANVIL:       return "A: Smith at Anvil";
    case OBJ_CHEF:        return "A: Talk to Chef Bouillon";
    case OBJ_KNIGHT:      return "A: Talk to Sir Garrick";
    case OBJ_TUTOR:       return "A: Talk to Sergeant Hardy";
    case OBJ_TANNER:      return "A: Talk to Pelt the Tanner";
    case OBJ_STAIRS_DOWN:
        if (cur_map == MAP_OVERWORLD)
            return wquest == WQ_NONE ? "A: Dungeon (barred - see Sir Garrick)"
                                     : "A: Descend into the dungeon";
        if (dungeon_floor == 1)
            return dquest == DQ_NONE ? "A: Deeper stair (sealed)"
                                     : "A: Descend to the depths";
        return "A: Descend to the Dragon's lair";
    case OBJ_STAIRS_UP:   return "A: Climb up";
    case OBJ_SHOP_GENERAL:return "A: Browse the General Store";
    case OBJ_SHOP_WEAPON: return "A: Browse the Weapon Shop";
    case OBJ_SHOP_ARMOR:  return "A: Browse the Armoury";
    case OBJ_SHOP_MAGIC:  return "A: Browse the Magic Shop";
    }
    return NULL;
}

/* ------------------------------------------------------------ inventory use */

/* ------------------------------------------------------------ equipment */

static void equip_from_inv(int invslot)
{
    int it = inv[invslot];
    int slot = iteminfo[it].slot;
    if (!slot) return;
    int reqsk = (is_bow(it) || is_ranged_armour(it)) ? SK_RANGED
                : (slot == SLOT_WEAPON ? SK_ATT : SK_DEF);
    if (level_of(reqsk) < iteminfo[it].eqlvl) {
        msg("You need %s level %d to wear that.",
            skill_names[reqsk], iteminfo[it].eqlvl);
        return;
    }
    /* swap whatever is worn back into the freed inventory slot */
    int prev = equipped[slot - 1];
    equipped[slot - 1] = it;
    inv[invslot] = prev;
    sfx_ui(SND_CRAFT);
    msg("You equip the %s.", iteminfo[it].name);
}

static void unequip(int slot)
{
    int it = equipped[slot];
    if (it == IT_NONE) return;
    if (inv_full()) { msg("Your inventory is too full to unequip that."); return; }
    add_item(it);
    equipped[slot] = IT_NONE;
    msg("You remove the %s.", iteminfo[it].name);
}

static void use_inv_item(int slot)
{
    int it = inv[slot];
    if (iteminfo[it].slot) { equip_from_inv(slot); return; }
    switch (it) {
    case IT_NONE: break;
    case IT_SHRIMP: case IT_TROUT: case IT_LOBSTER: case IT_SWORDFISH: {
        if (pl.eat_cd > 0) break;
        int food = inv[slot];
        inv[slot] = IT_NONE; inv_qty[slot] = 0;
        pl.eat_cd = 3;
        pl.hp += iteminfo[food].heal;
        int maxhp = level_of(SK_HP);
        if (pl.hp > maxhp) pl.hp = maxhp;
        sfx_ui(SND_EAT);
        msg("You eat the %s. It heals %d HP.", iteminfo[food].name, iteminfo[food].heal);
        break;
    }
    case IT_BONES:
        inv[slot] = IT_NONE;
        add_xp(SK_PRAY, 45, true);
        msg("You bury the bones.");
        break;
    case IT_LOGS: case IT_OAK_LOGS: {
        if (!has_item(IT_TINDER)) { msg("You need a tinderbox to light a fire."); break; }
        int ter = terrain[pl.ty][pl.tx];
        if (object[pl.ty][pl.tx] != OBJ_NONE ||
            (ter != TER_GRASS && ter != TER_PATH && ter != TER_SAND)) {
            msg("You can't light a fire here.");
            break;
        }
        pl.state = ST_LIGHT;
        pl.act_timer = 1;
        ui_mode = UI_NONE;
        msg("You attempt to light the logs.");
        break;
    }
    case IT_RAW_SHRIMP: case IT_RAW_TROUT: case IT_RAW_LOBSTER: case IT_RAW_SWORDFISH:
        msg("You should cook this on a fire first.");
        break;
    case IT_BURNT:
        msg("Ugh, there's nothing left of it. Best discard it.");
        break;
    case IT_ESSENCE:   msg("A chunk of raw magical essence."); break;
    case IT_DRAGONSTONE: msg("A priceless gem from the Dragon's hoard. Sell it for a fortune."); break;
    case IT_DRAGONFIRE: msg("The Dragonfire blade - it burns hot in hand and mind alike."); break;
    case IT_BRONZE_BAR: case IT_IRON_BAR: case IT_MITH_BAR: case IT_STEEL_BAR:
        msg("Take this to the anvil by the mine."); break;
    case IT_COAL: msg("Coal. Smelt iron+coal for steel, ore+2 coal for mithril."); break;
    case IT_MITH_ORE: msg("Mithril ore. Smelt with 2 coal at the furnace (Smithing 30)."); break;
    case IT_COWHIDE: msg("Cowhide. Pelt the Tanner will cure it into leather."); break;
    case IT_DRAGON_HIDE: msg("Dragonhide. The Tanner can cure this into tough leather."); break;
    case IT_LEATHER: case IT_DRAGON_LEATHER:
        msg("Use a needle to stitch this into ranged armour."); break;
    case IT_NEEDLE: ui_mode = UI_CRAFT; craft_cursor = 0; break;
    case IT_THREAD: msg("Thread - for stitching leather with a needle."); break;
    case IT_HAMMER:    msg("For smithing at an anvil."); break;
    case IT_BRONZE_SWORD: case IT_IRON_SWORD:
        msg("You feel mightier just holding it."); break;
    case IT_IRON_AXE:  msg("A woodcutter's even better friend."); break;
    case IT_IRON_PICK: msg("Bites through rock with ease."); break;
    case IT_AIR_RUNE:  msg("Air runes. Pick a spell with C-left."); break;
    case IT_FIRE_RUNE: msg("Fire runes. Pick a spell with C-left."); break;
    case IT_WATER_RUNE:msg("Water runes - for Water Strike."); break;
    case IT_EARTH_RUNE:msg("Earth runes - for Earth spells."); break;
    case IT_LAW_RUNE:  msg("Law runes bend space - they power teleports."); break;
    case IT_CHAOS_RUNE:msg("Chaos runes - bolt spells need one to fire."); break;
    case IT_AXE:    msg("A woodcutter's best friend."); break;
    case IT_PICK:   msg("Used for mining rocks."); break;
    case IT_NET:    msg("Net the shrimp at fishing spots."); break;
    case IT_ROD:    msg("A fishing rod - lands trout at the spots (Fishing 20)."); break;
    case IT_LOBSTER_POT: msg("A lobster pot - catches lobster (Fishing 40)."); break;
    case IT_HARPOON: msg("A harpoon - spears swordfish (Fishing 50)."); break;
    case IT_KNIFE:  ui_mode = UI_FLETCH; fletch_cursor = 0; break;
    case IT_ARROW_SHAFT: msg("Arrow shafts. Add arrowtips to fletch arrows."); break;
    case IT_BRONZE_TIPS: case IT_IRON_TIPS: case IT_MITH_TIPS:
        msg("Arrowtips. Use a knife to fletch them onto shafts."); break;
    case IT_BRONZE_ARROW: case IT_IRON_ARROW:
        msg("Arrows for your bow. Equip a bow, then A to shoot."); break;
    case IT_MITH_ARROW:
        msg("Mithril arrows - need an oak shortbow or better."); break;
    case IT_TINDER: msg("Useful for lighting fires."); break;
    }
}

/* ------------------------------------------------------------ smithing */

static const struct { int item, bar, bars, lvl, xp_x10; } smith_list[] = {
    { IT_BRONZE_SWORD,  IT_BRONZE_BAR, 1,  1, 125 },
    { IT_BRONZE_HELM,   IT_BRONZE_BAR, 1,  4, 125 },
    { IT_BRONZE_SHIELD, IT_BRONZE_BAR, 2,  6, 250 },
    { IT_BRONZE_BODY,   IT_BRONZE_BAR, 3,  8, 375 },
    { IT_IRON_SWORD,    IT_IRON_BAR,   1, 15, 250 },
    { IT_IRON_HELM,     IT_IRON_BAR,   1, 18, 250 },
    { IT_IRON_SHIELD,   IT_IRON_BAR,   2, 20, 500 },
    { IT_IRON_BODY,     IT_IRON_BAR,   3, 22, 750 },
    { IT_IRON_AXE,      IT_IRON_BAR,   1, 16, 250 },
    { IT_IRON_PICK,     IT_IRON_BAR,   1, 17, 250 },
    { IT_STEEL_SWORD,   IT_STEEL_BAR,  1, 20, 375 },
    { IT_STEEL_HELM,    IT_STEEL_BAR,  1, 23, 375 },
    { IT_STEEL_SHIELD,  IT_STEEL_BAR,  2, 25, 750 },
    { IT_STEEL_BODY,    IT_STEEL_BAR,  3, 28, 1125 },
    { IT_BRONZE_TIPS,   IT_BRONZE_BAR, 1,  5, 100 },
    { IT_IRON_TIPS,     IT_IRON_BAR,   1, 20, 250 },
    { IT_MITH_SWORD,    IT_MITH_BAR,   1, 30, 500 },
    { IT_MITH_HELM,     IT_MITH_BAR,   1, 33, 500 },
    { IT_MITH_SHIELD,   IT_MITH_BAR,   2, 35, 1000 },
    { IT_MITH_BODY,     IT_MITH_BAR,   3, 38, 1500 },
    { IT_MITH_TIPS,     IT_MITH_BAR,   1, 35, 500 },
};
#define SMITH_COUNT (int)(sizeof smith_list / sizeof smith_list[0])

static void smith_make(int row)
{
    if (row < 0 || row >= SMITH_COUNT) return;
    int item = smith_list[row].item, bar = smith_list[row].bar;
    int need = smith_list[row].bars;
    if (level_of(SK_SMITH) < smith_list[row].lvl) {
        msg("You need a Smithing level of %d to make that.", smith_list[row].lvl);
        return;
    }
    if (inv_count(bar) < need) {
        msg("You need %d %s for that.", need, iteminfo[bar].name);
        return;
    }
    for (int i = 0; i < need; i++) remove_item(bar);
    /* a bar of metal yields a batch of arrowtips, otherwise a single piece */
    int batch = (item == IT_BRONZE_TIPS || item == IT_IRON_TIPS ||
                 item == IT_MITH_TIPS) ? 10 : 1;
    for (int i = 0; i < batch; i++) add_item(item);
    sfx(SND_SMITH);
    if (batch > 1) msg("You hammer out %d %s.", batch, iteminfo[item].name);
    else           msg("You hammer out a %s.", iteminfo[item].name);
    add_xp(SK_SMITH, smith_list[row].xp_x10, true);
}

/* ------------------------------------------------------------ fletching */

static const struct { int result, qty, in1, n1, in2, n2, lvl, xp_x10; }
fletch_list[] = {
    { IT_ARROW_SHAFT, 10, IT_LOGS,        1, IT_NONE,         0,  1,  50 },
    { IT_SHORTBOW,     1, IT_LOGS,        1, IT_NONE,         0,  5,  80 },
    { IT_OAK_BOW,      1, IT_OAK_LOGS,    1, IT_NONE,         0, 20, 250 },
    { IT_BRONZE_ARROW,10, IT_ARROW_SHAFT,10, IT_BRONZE_TIPS, 10,  1, 130 },
    { IT_IRON_ARROW,  10, IT_ARROW_SHAFT,10, IT_IRON_TIPS,   10, 15, 250 },
    { IT_MITH_ARROW,  10, IT_ARROW_SHAFT,10, IT_MITH_TIPS,   10, 30, 480 },
};
#define FLETCH_COUNT (int)(sizeof fletch_list / sizeof fletch_list[0])

static void fletch_make(int row)
{
    if (row < 0 || row >= FLETCH_COUNT) return;
    int result = fletch_list[row].result, qty = fletch_list[row].qty;
    int in1 = fletch_list[row].in1, n1 = fletch_list[row].n1;
    int in2 = fletch_list[row].in2, n2 = fletch_list[row].n2;
    if (level_of(SK_FLETCH) < fletch_list[row].lvl) {
        msg("You need a Fletching level of %d for that.", fletch_list[row].lvl);
        return;
    }
    if (inv_count(in1) < n1 || (in2 != IT_NONE && inv_count(in2) < n2)) {
        msg("You don't have the materials for that.");
        return;
    }
    for (int i = 0; i < n1; i++) remove_item(in1);
    for (int i = 0; i < n2; i++) remove_item(in2);
    for (int i = 0; i < qty; i++) add_item(result);
    sfx(SND_SMITH);
    if (qty > 1) msg("You fletch %d %s.", qty, iteminfo[result].name);
    else         msg("You fletch a %s.", iteminfo[result].name);
    add_xp(SK_FLETCH, fletch_list[row].xp_x10, true);
}

/* ------------------------------------------------------------ crafting */

/* stitch tanned leather + thread into ranged armour (open with a needle) */
static const struct { int result, hide, n_hide, n_thread, lvl, xp_x10; }
craft_list[] = {
    { IT_LEATHER_COIF, IT_LEATHER,        1, 1,  5,  250 },
    { IT_LEATHER_BODY, IT_LEATHER,        3, 1, 14,  500 },
    { IT_DHIDE_COIF,   IT_DRAGON_LEATHER, 1, 1, 35, 1200 },
    { IT_DHIDE_BODY,   IT_DRAGON_LEATHER, 2, 1, 40, 1800 },
};
#define CRAFT_COUNT (int)(sizeof craft_list / sizeof craft_list[0])

static void craft_make(int row)
{
    if (row < 0 || row >= CRAFT_COUNT) return;
    int result = craft_list[row].result;
    int hide = craft_list[row].hide, nh = craft_list[row].n_hide;
    int nt = craft_list[row].n_thread;
    if (level_of(SK_CRAFT) < craft_list[row].lvl) {
        msg("You need a Crafting level of %d for that.", craft_list[row].lvl);
        return;
    }
    if (inv_count(hide) < nh || inv_count(IT_THREAD) < nt) {
        msg("You need %dx %s and %dx thread for that.",
            nh, iteminfo[hide].name, nt);
        return;
    }
    for (int i = 0; i < nh; i++) remove_item(hide);
    for (int i = 0; i < nt; i++) remove_item(IT_THREAD);
    add_item(result);
    sfx(SND_SMITH);
    msg("You stitch together a %s.", iteminfo[result].name);
    add_xp(SK_CRAFT, craft_list[row].xp_x10, true);
}

/* ------------------------------------------------------------ bank */

static int bank_rows_idx[NUM_ITEMS];

static int bank_rows(void)
{
    int n = 0;
    for (int i = 1; i < NUM_ITEMS; i++)
        if (bank[i] > 0) bank_rows_idx[n++] = i;
    return n;
}

static void bank_deposit_all(void)
{
    int n = 0;
    for (int i = 0; i < INV_SIZE; i++) {
        if (inv[i] == IT_NONE || iteminfo[inv[i]].tool) continue;
        bank[inv[i]] += inv_qty[i];      /* whole stack at once */
        inv[i] = IT_NONE;
        inv_qty[i] = 0;
        n++;
    }
    msg(n ? "You deposit your items." : "You have nothing to deposit.");
}

static void bank_withdraw(int row)
{
    int n = bank_rows();
    if (row < 0 || row >= n) return;
    int item = bank_rows_idx[row];
    if (inv_full()) { msg("Your inventory is too full."); return; }
    bank[item]--;
    add_item(item);
}

/* ------------------------------------------------------------ shops */

enum { SHOP_GENERAL, SHOP_WEAPON, SHOP_ARMOR, SHOP_MAGIC, NUM_SHOPS };
static const char *shop_names[NUM_SHOPS] = {
    "General Store", "Weapon Shop", "Armoury", "Magic Shop"
};
static const int shop_stock[NUM_SHOPS][20] = {
    { IT_AXE, IT_PICK, IT_NET, IT_ROD, IT_LOBSTER_POT, IT_HARPOON, IT_KNIFE,
      IT_NEEDLE, IT_THREAD, IT_TINDER, IT_HAMMER, IT_RAW_SHRIMP, IT_SHRIMP, IT_NONE },
    { IT_BRONZE_SWORD, IT_IRON_SWORD, IT_STEEL_SWORD, IT_MITH_SWORD, IT_RUNE_SWORD, IT_NONE },
    { IT_BRONZE_HELM, IT_BRONZE_SHIELD, IT_BRONZE_BODY,
      IT_IRON_HELM, IT_IRON_SHIELD, IT_IRON_BODY,
      IT_STEEL_HELM, IT_STEEL_SHIELD, IT_STEEL_BODY,
      IT_MITH_HELM, IT_MITH_SHIELD, IT_MITH_BODY,
      IT_RUNE_HELM, IT_RUNE_SHIELD, IT_RUNE_BODY, IT_NONE },
    { IT_AIR_RUNE, IT_WATER_RUNE, IT_EARTH_RUNE, IT_FIRE_RUNE, IT_CHAOS_RUNE,
      IT_LAW_RUNE, IT_STAFF, IT_WIZ_HAT, IT_WIZ_ROBE, IT_NONE },
};
static int shop_sell_list[NUM_ITEMS];

static int shop_stock_count(int s)
{
    int n = 0;
    while (n < 20 && shop_stock[s][n] != IT_NONE) n++;
    return n;
}

static int shop_sell_count(void)
{
    int n = 0;
    for (int it = 1; it < NUM_ITEMS; it++)
        if (it != IT_COINS && item_value[it] > 0 && inv_count(it) > 0)
            shop_sell_list[n++] = it;
    return n;
}

static void shop_buy(int item)
{
    int price = item_value[item];
    if (price <= 0) return;
    if (gp < price) { msg("You don't have enough coins."); return; }
    if (inv_full() && !(iteminfo[item].stackable && has_item(item))) {
        msg("Your inventory is full."); return;
    }
    gp -= price;
    add_item(item);
    msg("Bought %s for %d coins.", iteminfo[item].name, price);
}

static void shop_sell(int item)
{
    int price = sell_price(item);
    if (price <= 0 || inv_count(item) <= 0) return;
    remove_item(item);
    gp += price;
    msg("Sold %s for %d coins.", iteminfo[item].name, price);
}

/* ------------------------------------------------------------ movement */

static void update_projectiles(float dt)
{
    for (int i = 0; i < MAX_PROJ; i++) {
        if (!proj[i].active) continue;
        float dx = proj[i].tx - proj[i].x, dy = proj[i].ty - proj[i].y;
        float dist = sqrtf(dx * dx + dy * dy);
        float step = 360.0f * dt;
        if (step >= dist) { proj[i].active = false; continue; }
        proj[i].x += dx / dist * step;
        proj[i].y += dy / dist * step;
    }
}

static void update_movement(float dt)
{
    update_projectiles(dt);
    float speed = (pl.run_on && pl.run_energy > 0) ? RUN_SPEED : WALK_SPEED;
    if (pl.moving) {
        float gx = pl.mtx * TILE + 8, gy = pl.mty * TILE + 12;
        float dx = gx - pl.px, dy = gy - pl.py;
        float dist = sqrtf(dx * dx + dy * dy);
        float step = speed * dt;
        pl.walk_anim += step;
        if (step >= dist) {
            pl.px = gx; pl.py = gy;
            pl.tx = pl.mtx; pl.ty = pl.mty;
            pl.moving = false;
            if (pl.run_on && pl.run_energy > 0) pl.run_energy--;
        } else {
            pl.px += dx / dist * step;
            pl.py += dy / dist * step;
        }
    }
    if (pl.bump_t > 0) pl.bump_t--;

    /* bot interpolation */
    for (int i = 0; i < MAX_BOT; i++) {
        bot_t *b = &bots[i];
        if (!b->moving) continue;
        float gx = b->mtx * TILE + 8, gy = b->mty * TILE + 12;
        float dx = gx - b->px, dy = gy - b->py;
        float dist = sqrtf(dx * dx + dy * dy);
        float step = WALK_SPEED * dt;
        b->walk_anim += step;
        if (step >= dist) {
            b->px = gx; b->py = gy;
            b->tx = b->mtx; b->ty = b->mty;
            b->moving = false;
        } else {
            b->px += dx / dist * step;
            b->py += dy / dist * step;
        }
    }

    /* goblin interpolation */
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists || g->dead) continue;
        if (g->bump_t > 0) g->bump_t--;
        if (!g->moving) continue;
        float gx = g->mtx * TILE + 8, gy = g->mty * TILE + 12;
        float dx = gx - g->px, dy = gy - g->py;
        float dist = sqrtf(dx * dx + dy * dy);
        float step = WALK_SPEED * dt;
        if (step >= dist) {
            g->px = gx; g->py = gy;
            g->tx = g->mtx; g->ty = g->mty;
            g->moving = false;
        } else {
            g->px += dx / dist * step;
            g->py += dy / dist * step;
        }
    }
}

static void try_walk(int dx, int dy)
{
    if (pl.moving) return;
    if (!dx && !dy) return;

    /* set facing even if blocked */
    if (dx) pl.facing = dx > 0 ? 3 : 2;
    else if (dy) pl.facing = dy > 0 ? 0 : 1;

    if (pl.state != ST_IDLE) stop_action();

    /* no corner cutting */
    if (dx && dy && (!tile_walkable(pl.tx + dx, pl.ty) ||
                     !tile_walkable(pl.tx, pl.ty + dy))) {
        if (tile_walkable(pl.tx + dx, pl.ty)) dy = 0;
        else if (tile_walkable(pl.tx, pl.ty + dy)) dx = 0;
        else return;
    }
    int nx = pl.tx + dx, ny = pl.ty + dy;
    if (!tile_walkable(nx, ny)) return;
    pl.mtx = nx; pl.mty = ny;
    pl.moving = true;
}

/* ------------------------------------------------------------ rendering */

static void draw_panel(int x0, int y0, int x1, int y1)
{
    rdpq_set_mode_fill(RGBA32(62, 53, 41, 255));
    rdpq_fill_rectangle(x0 - 2, y0 - 2, x1 + 2, y1 + 2);
    rdpq_set_mode_fill(RGBA32(45, 38, 30, 255));
    rdpq_fill_rectangle(x0, y0, x1, y1);
}

static void draw_text(int style, int x, int y, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    rdpq_text_print(&(rdpq_textparms_t){ .style_id = style }, FONT_ID, x, y, buf);
}

/* relative sprite index (0-7) within an 8-sprite actor set.
   left-facing uses pre-flipped art: runtime mirroring (flip_x) reverses the
   texture S range, which is undefined in RDP copy mode and crashes it */
static int actor_sprite(int facing, bool moving, float walk_anim)
{
    int frame = ((int)(walk_anim / 8)) & 1;
    int idx;
    switch (facing) {
    case 0:  idx = 0; break;     /* down  */
    case 1:  idx = 2; break;     /* up    */
    case 2:  idx = 6; break;     /* left  */
    default: idx = 4; break;     /* right */
    }
    if (frame && moving) idx++;
    return idx;
}

/* worn-gear overlays: base sprite (the _D facing) for an equipped item, or -1.
   the 4 facings (down,up,side,side-left) are consecutive after the base. */
static int equip_overlay_base(int item)
{
    switch (item) {
    case IT_BRONZE_HELM:   return SPR_EQ_BZ_HELM_D;
    case IT_IRON_HELM:     return SPR_EQ_IR_HELM_D;
    case IT_BRONZE_BODY:   return SPR_EQ_BZ_BODY_D;
    case IT_IRON_BODY:     return SPR_EQ_IR_BODY_D;
    case IT_BRONZE_SWORD:  return SPR_EQ_BZ_WEP_D;
    case IT_IRON_SWORD:    return SPR_EQ_IR_WEP_D;
    case IT_BRONZE_SHIELD: return SPR_EQ_BZ_SHD_D;
    case IT_IRON_SHIELD:   return SPR_EQ_IR_SHD_D;
    case IT_STAFF:         return SPR_EQ_STAFF_D;
    case IT_WIZ_HAT:       return SPR_EQ_HAT_D;
    case IT_WIZ_ROBE:      return SPR_EQ_ROBE_D;
    case IT_STEEL_SWORD:   return SPR_EQ_ST_WEP_D;
    case IT_STEEL_HELM:    return SPR_EQ_ST_HELM_D;
    case IT_STEEL_SHIELD:  return SPR_EQ_ST_SHD_D;
    case IT_STEEL_BODY:    return SPR_EQ_ST_BODY_D;
    case IT_MITH_SWORD:    return SPR_EQ_MI_WEP_D;
    case IT_MITH_HELM:     return SPR_EQ_MI_HELM_D;
    case IT_MITH_SHIELD:   return SPR_EQ_MI_SHD_D;
    case IT_MITH_BODY:     return SPR_EQ_MI_BODY_D;
    case IT_RUNE_SWORD:    return SPR_EQ_RU_WEP_D;
    case IT_RUNE_HELM:     return SPR_EQ_RU_HELM_D;
    case IT_RUNE_SHIELD:   return SPR_EQ_RU_SHD_D;
    case IT_RUNE_BODY:     return SPR_EQ_RU_BODY_D;
    case IT_BANE:          return SPR_EQ_BANE_WEP_D;
    case IT_DRAGONFIRE:    return SPR_EQ_DF_WEP_D;
    case IT_LEATHER_COIF:  return SPR_EQ_LE_HELM_D;
    case IT_LEATHER_BODY:  return SPR_EQ_LE_BODY_D;
    case IT_DHIDE_COIF:    return SPR_EQ_DH_HELM_D;
    case IT_DHIDE_BODY:    return SPR_EQ_DH_BODY_D;
    default:               return -1;
    }
}

/* facing (0 down,1 up,2 left,3 right) -> overlay dir (0 down,1 up,2 side,3 side-left) */
static int overlay_dir(int facing)
{
    switch (facing) {
    case 0:  return 0;
    case 1:  return 1;
    case 2:  return 3;   /* left uses the baked flip */
    default: return 2;   /* right */
    }
}

static void draw_player_equipment(int px, int py, int facing)
{
    /* draw order: body, helm, shield, then weapon on top */
    static const int order[4] = { SLOT_BODY, SLOT_HELM, SLOT_SHIELD, SLOT_WEAPON };
    int dir = overlay_dir(facing);
    for (int i = 0; i < 4; i++) {
        int item = equipped[order[i] - 1];
        if (item == IT_NONE) continue;
        int base = equip_overlay_base(item);
        if (base < 0) continue;
        rdpq_sprite_blit(spr[base + dir], px, py, NULL);
    }
}

static void draw_entity_hpbar(float sx, float top, int hp, int maxhp, int w)
{
    int green = hp * w / maxhp;
    if (green < 0) green = 0;
    rdpq_set_mode_fill(RGBA32(200, 30, 30, 255));
    rdpq_fill_rectangle(sx - w / 2, top, sx - w / 2 + w, top + 3);
    if (green > 0) {
        rdpq_set_mode_fill(RGBA32(40, 180, 40, 255));
        rdpq_fill_rectangle(sx - w / 2, top, sx - w / 2 + green, top + 3);
    }
}

static void draw_hitsplat(float sx, float sy, int dmg)
{
    color_t c = dmg > 0 ? RGBA32(160, 20, 20, 255) : RGBA32(40, 60, 160, 255);
    rdpq_set_mode_fill(c);
    rdpq_fill_rectangle(sx - 6, sy - 14, sx + 6, sy - 4);
    draw_text(0, (int)sx - 3, (int)sy - 5, "%d", dmg);
}

static void render(void)
{
    surface_t *disp = display_get();
    rdpq_attach(disp, NULL);

    /* ---- camera ---- */
    int cam_x = (int)pl.px - SCREEN_W / 2;
    int cam_y = (int)pl.py - SCREEN_H / 2;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > MAP_W * TILE - SCREEN_W) cam_x = MAP_W * TILE - SCREEN_W;
    if (cam_y > MAP_H * TILE - SCREEN_H) cam_y = MAP_H * TILE - SCREEN_H;
    /* copy mode cannot draw 4bpp textures at odd screen X (sub-byte shift
       is undefined on the RDP): keep the camera and every copy-mode blit
       on even X so the whole frame stays aligned */
    cam_x &= ~1;

    int tx0 = cam_x / TILE, ty0 = cam_y / TILE;
    int tx1 = (cam_x + SCREEN_W - 1) / TILE, ty1 = (cam_y + SCREEN_H - 1) / TILE;
    int water_frame = (frame_counter / 24) & 1;
    int anim_frame  = (frame_counter / 16) & 1;

    /* ---- terrain (opaque, copy mode) ----
       batched by tile type: upload each texture once, then stamp cheap
       texture rectangles. One blit per tile re-uploads ~340 textures per
       frame, which overwhelms the RDP. */
    rdpq_set_mode_copy(false);
    uint8_t cell[16][22];
    for (int y = ty0; y <= ty1; y++) {
        for (int x = tx0; x <= tx1; x++) {
            int s;
            switch (terrain[y][x]) {
            case TER_PATH:   s = SPR_PATH; break;
            case TER_SAND:   s = SPR_SAND; break;
            case TER_WATER:  s = water_frame ? SPR_WATER_B : SPR_WATER_A; break;
            case TER_BRIDGE: s = SPR_BRIDGE; break;
            case TER_WALL:   s = SPR_WALL; break;
            case TER_FLOOR:  s = SPR_FLOOR; break;
            case TER_CAVE:   s = SPR_CAVE; break;
            default:
                s = ((x * 31 + y * 17) % 5 == 0) ? SPR_GRASS_B : SPR_GRASS_A;
            }
            cell[y - ty0][x - tx0] = s;
        }
    }
    for (int s = SPR_GRASS_A; s <= SPR_CAVE; s++) {
        bool uploaded = false;
        for (int y = ty0; y <= ty1; y++) {
            for (int x = tx0; x <= tx1; x++) {
                if (cell[y - ty0][x - tx0] != s) continue;
                if (!uploaded) {
                    rdpq_sprite_upload(TILE0, spr[s], NULL);
                    uploaded = true;
                }
                int sx = x * TILE - cam_x, sy = y * TILE - cam_y;
                rdpq_texture_rectangle(TILE0, sx, sy, sx + TILE, sy + TILE, 0, 0);
            }
        }
    }

    /* ---- objects & entities, painter's order by row ---- */
    rdpq_set_mode_copy(true);
    int pl_row = (int)pl.py / TILE;
    for (int y = ty0 - 1; y <= ty1 + 1; y++) {
        if (y < 0 || y >= MAP_H) continue;
        for (int x = tx0; x <= tx1; x++) {
            int o = object[y][x];
            if (o == OBJ_NONE) continue;
            int sx = x * TILE - cam_x, sy = y * TILE - cam_y;
            switch (o) {
            case OBJ_TREE: rdpq_sprite_blit(spr[SPR_TREE], sx, sy - 8, NULL); break;
            case OBJ_OAK:  rdpq_sprite_blit(spr[SPR_OAK],  sx, sy - 8, NULL); break;
            case OBJ_STUMP:rdpq_sprite_blit(spr[SPR_STUMP], sx, sy, NULL); break;
            case OBJ_ROCK_COPPER: rdpq_sprite_blit(spr[SPR_ROCK_C], sx, sy, NULL); break;
            case OBJ_ROCK_TIN:    rdpq_sprite_blit(spr[SPR_ROCK_T], sx, sy, NULL); break;
            case OBJ_ROCK_IRON:   rdpq_sprite_blit(spr[SPR_ROCK_I], sx, sy, NULL); break;
            case OBJ_ROCK_MITHRIL:rdpq_sprite_blit(spr[SPR_ROCK_M], sx, sy, NULL); break;
            case OBJ_ROCK_COAL:   rdpq_sprite_blit(spr[SPR_ROCK_COAL], sx, sy, NULL); break;
            case OBJ_ROCK_EMPTY:  rdpq_sprite_blit(spr[SPR_ROCK_E], sx, sy, NULL); break;
            case OBJ_FISH:
                rdpq_sprite_blit(spr[anim_frame ? SPR_FISH_B : SPR_FISH_A], sx, sy, NULL);
                break;
            case OBJ_BOOTH: rdpq_sprite_blit(spr[SPR_BOOTH], sx, sy, NULL); break;
            case OBJ_FIRE:
                rdpq_sprite_blit(spr[anim_frame ? SPR_FIRE_B : SPR_FIRE_A], sx, sy, NULL);
                break;
            case OBJ_ESSENCE:    rdpq_sprite_blit(spr[SPR_ESSENCE], sx, sy, NULL); break;
            case OBJ_ALTAR_AIR:  rdpq_sprite_blit(spr[SPR_ALTAR_AIR], sx, sy, NULL); break;
            case OBJ_ALTAR_WATER:rdpq_sprite_blit(spr[SPR_ALTAR_WATER], sx, sy, NULL); break;
            case OBJ_ALTAR_EARTH:rdpq_sprite_blit(spr[SPR_ALTAR_EARTH], sx, sy, NULL); break;
            case OBJ_ALTAR_FIRE: rdpq_sprite_blit(spr[SPR_ALTAR_FIRE], sx, sy, NULL); break;
            case OBJ_ALTAR_CHAOS:rdpq_sprite_blit(spr[SPR_ALTAR_CHAOS], sx, sy, NULL); break;
            case OBJ_ALTAR_LAW:  rdpq_sprite_blit(spr[SPR_ALTAR_LAW], sx, sy, NULL); break;
            case OBJ_FURNACE:    rdpq_sprite_blit(spr[SPR_FURNACE], sx, sy, NULL); break;
            case OBJ_ANVIL:      rdpq_sprite_blit(spr[SPR_ANVIL], sx, sy, NULL); break;
            case OBJ_CHEF:       rdpq_sprite_blit(spr[SPR_CHEF], sx, sy - 8, NULL); break;
            case OBJ_KNIGHT:     rdpq_sprite_blit(spr[SPR_KNIGHT], sx, sy - 8, NULL); break;
            case OBJ_FENCE:      rdpq_sprite_blit(spr[SPR_FENCE], sx, sy, NULL); break;
            case OBJ_TUTOR:      rdpq_sprite_blit(spr[SPR_TUTOR], sx, sy - 8, NULL); break;
            case OBJ_TANNER:     rdpq_sprite_blit(spr[SPR_TANNER], sx, sy - 8, NULL); break;
            case OBJ_STAIRS_DOWN:rdpq_sprite_blit(spr[SPR_STAIRS_DOWN], sx, sy, NULL); break;
            case OBJ_STAIRS_UP:  rdpq_sprite_blit(spr[SPR_STAIRS_UP], sx, sy, NULL); break;
            case OBJ_SHOP_GENERAL:rdpq_sprite_blit(spr[SPR_STALL_GENERAL], sx, sy - 4, NULL); break;
            case OBJ_SHOP_WEAPON: rdpq_sprite_blit(spr[SPR_STALL_WEAPON], sx, sy - 4, NULL); break;
            case OBJ_SHOP_ARMOR:  rdpq_sprite_blit(spr[SPR_STALL_ARMOR], sx, sy - 4, NULL); break;
            case OBJ_SHOP_MAGIC:  rdpq_sprite_blit(spr[SPR_STALL_MAGIC], sx, sy - 4, NULL); break;
            }
        }

        /* monsters whose feet are in this row */
        for (int i = 0; i < num_gob; i++) {
            gob_t *g = &gob[i];
            if (!g->exists || g->dead) continue;
            if ((int)g->py / TILE != y) continue;
            const mobinfo_t *mi = &mobinfo[g->type];
            float bk = g->bump_t > 0 ? (float)g->bump_t / BUMP_FRAMES : 0;
            int bx = (int)(g->bump_x * bk), by = (int)(g->bump_y * bk);
            int ex = ((int)(g->px - mi->w / 2 + bx) - cam_x) & ~1;
            if (ex < -32 || ex > SCREEN_W) continue;
            int spr_id = anim_frame ? mi->spr_b : mi->spr_a;
            rdpq_sprite_blit(spr[spr_id], ex,
                             (int)(g->py - (mi->h - 4) + by) - cam_y, NULL);
        }

        /* bots whose feet are in this row (surface only) */
        for (int i = 0; i < MAX_BOT && cur_map == MAP_OVERWORLD; i++) {
            bot_t *b = &bots[i];
            if ((int)b->py / TILE != y) continue;
            int ex = ((int)(b->px - 8) - cam_x) & ~1;
            if (ex < -16 || ex > SCREEN_W) continue;
            int idx = actor_sprite(b->facing, b->moving, b->walk_anim);
            rdpq_sprite_blit(spr[SPR_BOTA_DOWN_A + b->look * 8 + idx],
                             ex, (int)(b->py - 20) - cam_y, NULL);
        }

        /* player */
        if (pl_row == y) {
            int idx = actor_sprite(pl.facing, pl.moving, pl.walk_anim);
            float bk = pl.bump_t > 0 ? (float)pl.bump_t / BUMP_FRAMES : 0;
            int bx = (int)(pl.bump_x * bk), by = (int)(pl.bump_y * bk);
            int px = ((int)(pl.px - 8 + bx) - cam_x) & ~1;
            int py = (int)(pl.py - 20 + by) - cam_y;
            rdpq_sprite_blit(spr[SPR_PL_DOWN_A + idx], px, py, NULL);
            draw_player_equipment(px, py, pl.facing);
        }
    }

    /* ---- spell projectiles ---- */
    rdpq_set_mode_copy(true);
    for (int i = 0; i < MAX_PROJ; i++) {
        if (!proj[i].active) continue;
        int sx = ((int)proj[i].x - 4 - cam_x) & ~1;
        int sy = (int)proj[i].y - 4 - cam_y;
        if (sx < -8 || sx > SCREEN_W) continue;
        rdpq_sprite_blit(spr[proj[i].spr], sx, sy, NULL);
    }

    /* ---- hp bars & hitsplats ---- */
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists || g->dead) continue;
        const mobinfo_t *mi = &mobinfo[g->type];
        float sx = g->px - cam_x, sy = g->py - cam_y;
        int top = (int)sy - (mi->h - 4) - 6;
        /* the boss always shows its bar; lesser mobs only when recently hit */
        if (g->hurt_timer > 0 || g->type == MOB_BOSS)
            draw_entity_hpbar(sx, top, g->hp, mi->max_hp, mi->w + 8);
        if (g->hitsplat_t > 0) {
            g->hitsplat_t--;
            draw_hitsplat(sx, sy, g->hitsplat);
        }
    }
    {
        float sx = pl.px - cam_x, sy = pl.py - cam_y;
        if (pl.hitsplat_t > 0) {
            pl.hitsplat_t--;
            draw_entity_hpbar(sx, sy - 34, pl.hp, level_of(SK_HP), 16);
            draw_hitsplat(sx, sy, pl.hitsplat);
        }
    }

    /* ---- bot overhead chat ---- */
    for (int i = 0; i < MAX_BOT && cur_map == MAP_OVERWORLD; i++) {
        bot_t *b = &bots[i];
        if (b->say_t <= 0) continue;
        b->say_t--;
        int w = strlen(b->say) * 6;
        int bx = (int)(b->px - cam_x) - w / 2;
        int by = (int)(b->py - cam_y) - 26;
        if (bx < 2) bx = 2;
        if (bx > SCREEN_W - w - 2) bx = SCREEN_W - w - 2;
        if (by < 10 || by > SCREEN_H) continue;
        draw_text(1, bx, by, "%s", b->say);
    }

    /* ---- xp drops ---- */
    for (int i = 0; i < MAX_XPDROP; i++) {
        if (!xpdrop[i].active) continue;
        xpdrop[i].age++;
        xpdrop[i].y -= 0.5f;
        if (xpdrop[i].age > 60) { xpdrop[i].active = false; continue; }
        draw_text(5, (int)(pl.px - cam_x) - 12,
                  (int)(pl.py - cam_y) - 30 + (int)xpdrop[i].y, "%s", xpdrop[i].text);
    }

    /* ---- title screen ---- */
    if (game_state == STATE_TITLE) {
        /* gold logo: 4x4 grid of small copy-mode tiles (even X positions) */
        rdpq_set_mode_copy(true);
        int logo_x = (SCREEN_W - 256) / 2;
        for (int i = 0; i < 16; i++)
            rdpq_sprite_blit(spr[SPR_LOGO_0 + i],
                             logo_x + (i % 4) * 64, 28 + (i / 4) * 14, NULL);
        rdpq_textparms_t center = { .style_id = 6, .align = ALIGN_CENTER,
                                    .width = SCREEN_W };
        rdpq_text_print(&center, FONT_ID, 0, 98,
                        "An old-school adventure on 64 bits");
        if (save_found) {
            rdpq_textparms_t opt = { .align = ALIGN_CENTER, .width = SCREEN_W };
            opt.style_id = title_sel == 0 ? 1 : 0;
            rdpq_text_print(&opt, FONT_ID, 0, 148,
                            title_sel == 0 ? "> Continue <" : "Continue");
            opt.style_id = title_sel == 1 ? 1 : 0;
            rdpq_text_print(&opt, FONT_ID, 0, 162,
                            title_sel == 1 ? "> New Game <" : "New Game");
            rdpq_text_print(&center, FONT_ID, 0, 186,
                            "D-Pad: choose   A: select");
        } else if ((frame_counter / 30) & 1) {
            rdpq_textparms_t blink = { .style_id = 1, .align = ALIGN_CENTER,
                                       .width = SCREEN_W };
            rdpq_text_print(&blink, FONT_ID, 0, 156, "PRESS START");
        }
        rdpq_text_print(&center, FONT_ID, 0, 226,
                        "original homebrew built with libdragon");
        rdpq_detach_show();
        return;
    }

    /* ---- HUD ---- */
    rdpq_set_mode_fill(RGBA32(20, 20, 20, 255));
    rdpq_fill_rectangle(4, 4, 90, 48);
    int maxhp = level_of(SK_HP);
    draw_text(pl.hp * 3 <= maxhp ? 3 : 4, 8, 14, "HP  %d/%d", pl.hp, maxhp);
    draw_text(pl.run_on ? 1 : 6, 8, 24, "Run %d%% %s", pl.run_energy,
              pl.run_on ? "(on)" : "");
    if (cast_spell != SPELL_MELEE)
        draw_text(5, 8, 34, "Atk %s", spellinfo[cast_spell].name);
    else if (is_bow(equipped[SLOT_WEAPON - 1]))
        draw_text(5, 8, 34, "Atk Ranged");
    else
        draw_text(6, 8, 34, "Atk Melee");
    bool praying = false;
    for (int i = 0; i < NUM_PRAYERS; i++) if (pray_on[i]) praying = true;
    draw_text(praying ? 4 : 6, 8, 44, "Pray %d/%d", pray_pts, pray_max());

    const char *hint = (ui_mode == UI_NONE) ? context_hint() : NULL;
    if (hint) {
        rdpq_set_mode_fill(RGBA32(20, 20, 20, 255));
        int w = strlen(hint) * 6 + 8;
        rdpq_fill_rectangle(SCREEN_W / 2 - w / 2, 4, SCREEN_W / 2 + w / 2, 16);
        rdpq_text_print(&(rdpq_textparms_t){ .style_id = 1, .align = ALIGN_CENTER,
                        .width = SCREEN_W }, FONT_ID, 0, 13, hint);
    }

    /* ---- chat box ---- */
    {
        int y0 = SCREEN_H - 42;
        rdpq_set_mode_fill(RGBA32(198, 181, 148, 255));
        rdpq_fill_rectangle(2, y0, 254, SCREEN_H - 2);
        rdpq_set_mode_fill(RGBA32(90, 70, 40, 255));
        rdpq_fill_rectangle(2, y0, 254, y0 + 1);
        for (int i = 0; i < 4; i++)
            draw_text(2, 6, y0 + 10 + i * 9, "%s", chat[CHAT_LINES - 4 + i]);
    }

    /* ---- side panels ---- */
    if (ui_mode == UI_INV) {
        int px0 = SCREEN_W - 88, py0 = 30;
        draw_panel(px0, py0, px0 + 82, py0 + 152);
        draw_text(1, px0 + 6, py0 + 12, "Inv");
        draw_text(5, px0 + 34, py0 + 12, "%d gp", gp);
        /* slots: one fill pass, then the cursor highlight */
        rdpq_set_mode_fill(RGBA32(70, 60, 45, 255));
        for (int i = 0; i < INV_SIZE; i++) {
            if (i == inv_cursor) continue;
            int cx = px0 + 6 + (i % 4) * 19;
            int cy = py0 + 18 + (i / 4) * 18;
            rdpq_fill_rectangle(cx, cy, cx + 17, cy + 16);
        }
        rdpq_set_mode_fill(RGBA32(120, 100, 60, 255));
        {
            int cx = px0 + 6 + (inv_cursor % 4) * 19;
            int cy = py0 + 18 + (inv_cursor / 4) * 18;
            rdpq_fill_rectangle(cx, cy, cx + 17, cy + 16);
        }
        rdpq_set_mode_copy(true);
        for (int i = 0; i < INV_SIZE; i++) {
            if (inv[i] == IT_NONE) continue;
            int cx = (px0 + 6 + (i % 4) * 19) & ~1;
            int cy = py0 + 18 + (i / 4) * 18;
            rdpq_sprite_blit(spr[iteminfo[inv[i]].spr], cx, cy, NULL);
        }
        /* stack counts, drawn over the top-left of stackable icons */
        for (int i = 0; i < INV_SIZE; i++) {
            if (inv[i] == IT_NONE || inv_qty[i] <= 1) continue;
            int cx = px0 + 6 + (i % 4) * 19;
            int cy = py0 + 18 + (i / 4) * 18;
            draw_text(1, cx + 1, cy + 7, "%d", inv_qty[i]);
        }
        draw_text(0, px0 + 6, py0 + 150, "%s",
                  inv[inv_cursor] != IT_NONE ? iteminfo[inv[inv_cursor]].name : "-");
    }
    else if (ui_mode == UI_SKILLS) {
        int px0 = SCREEN_W - 130, py0 = 12;
        draw_panel(px0, py0, px0 + 124, py0 + 184);
        draw_text(1, px0 + 6, py0 + 10, "Skills");
        for (int i = 0; i < NUM_SKILLS; i++)
            draw_text(0, px0 + 6, py0 + 22 + i * 9, "%-11s %2d",
                      skill_names[i], level_of(i));
        int att = level_of(SK_ATT), str = level_of(SK_STR), def = level_of(SK_DEF);
        float base = (def + level_of(SK_HP) + level_of(SK_PRAY) / 2) * 0.25f;
        float melee = (att + str) * 0.325f;
        float rng = (level_of(SK_RANGED) * 3 / 2) * 0.325f;
        float mage = (level_of(SK_MAGIC) * 3 / 2) * 0.325f;
        float best = melee; if (rng > best) best = rng; if (mage > best) best = mage;
        int cmb = (int)(base + best);
        draw_text(4, px0 + 6, py0 + 176, "Combat level: %d", cmb);
    }
    else if (ui_mode == UI_BANK) {
        int n = bank_rows();
        int px0 = 60, py0 = 30;
        draw_panel(px0, py0, px0 + 200, py0 + 140);
        draw_text(1, px0 + 6, py0 + 12, "Bank of Rune Valley");
        draw_text(6, px0 + 6, py0 + 22, "A:withdraw Z:deposit all B:close");
        if (n == 0)
            draw_text(0, px0 + 6, py0 + 40, "The bank is empty.");
        if (bank_cursor >= n) bank_cursor = n ? n - 1 : 0;
        for (int i = 0; i < n && i < 11; i++) {
            int it = bank_rows_idx[i];
            draw_text(i == bank_cursor ? 1 : 0, px0 + 6, py0 + 36 + i * 10,
                      "%c %-14s x%d", i == bank_cursor ? '>' : ' ',
                      iteminfo[it].name, bank[it]);
        }
    }
    else if (ui_mode == UI_SHOP) {
        int px0 = 50, py0 = 20;
        draw_panel(px0, py0, px0 + 220, py0 + 188);
        draw_text(1, px0 + 6, py0 + 12, "%s", shop_names[shop_id]);
        draw_text(5, px0 + 150, py0 + 12, "%d gp", gp);
        draw_text(6, px0 + 6, py0 + 22, "%s   A:%s  Z:swap  B:close",
                  shop_mode ? "[SELL]" : "[BUY]", shop_mode ? "sell" : "buy");
        int n = shop_mode ? shop_sell_count() : shop_stock_count(shop_id);
        if (shop_cursor >= n) shop_cursor = n ? n - 1 : 0;
        if (n == 0)
            draw_text(0, px0 + 6, py0 + 40, "Nothing to sell here.");
        /* scroll the list so the cursor stays visible (16 rows fit) */
        int top = shop_cursor - 8; if (top < 0) top = 0;
        if (top > n - 16) top = n - 16 < 0 ? 0 : n - 16;
        for (int i = top; i < n && i < top + 16; i++) {
            int it = shop_mode ? shop_sell_list[i] : shop_stock[shop_id][i];
            int price = shop_mode ? sell_price(it) : item_value[it];
            int ry = py0 + 36 + (i - top) * 9;
            if (shop_mode)
                draw_text(i == shop_cursor ? 1 : 0, px0 + 6, ry,
                          "%c %-14s x%d   %dgp", i == shop_cursor ? '>' : ' ',
                          iteminfo[it].name, inv_count(it), price);
            else
                draw_text(i == shop_cursor ? 1 : (gp >= price ? 0 : 6),
                          px0 + 6, ry, "%c %-16s %dgp",
                          i == shop_cursor ? '>' : ' ', iteminfo[it].name, price);
        }
    }
    else if (ui_mode == UI_SMITH) {
        int px0 = 64, py0 = 16;
        const int VIS = 16;                 /* visible rows; the list scrolls */
        int shown = SMITH_COUNT < VIS ? SMITH_COUNT : VIS;
        draw_panel(px0, py0, px0 + 192, py0 + 30 + shown * 10);
        draw_text(1, px0 + 6, py0 + 8, "Anvil - Smithing");
        draw_text(6, px0 + 6, py0 + 18, "A: smith   B: close");
        int top = smith_cursor - VIS / 2; if (top < 0) top = 0;
        if (top > SMITH_COUNT - VIS) top = SMITH_COUNT - VIS;
        if (top < 0) top = 0;
        for (int i = top; i < SMITH_COUNT && i < top + VIS; i++) {
            bool can = level_of(SK_SMITH) >= smith_list[i].lvl;
            const char *bar = smith_list[i].bar == IT_BRONZE_BAR ? "brz" :
                              smith_list[i].bar == IT_IRON_BAR   ? "irn" :
                              smith_list[i].bar == IT_STEEL_BAR  ? "stl" : "mth";
            draw_text(i == smith_cursor ? 1 : (can ? 0 : 6),
                      px0 + 6, py0 + 30 + (i - top) * 10,
                      "%c %-15s %dx%s L%d",
                      i == smith_cursor ? '>' : ' ',
                      iteminfo[smith_list[i].item].name,
                      smith_list[i].bars, bar, smith_list[i].lvl);
        }
    }
    else if (ui_mode == UI_FLETCH) {
        int px0 = 56, py0 = 48;
        draw_panel(px0, py0, px0 + 208, py0 + 110);
        draw_text(1, px0 + 6, py0 + 12, "Fletching (knife)");
        draw_text(6, px0 + 6, py0 + 22, "A: fletch   B: close");
        for (int i = 0; i < FLETCH_COUNT; i++) {
            bool can = level_of(SK_FLETCH) >= fletch_list[i].lvl;
            draw_text(i == fletch_cursor ? 1 : (can ? 0 : 6),
                      px0 + 6, py0 + 34 + i * 12, "%c %dx %-13s L%d",
                      i == fletch_cursor ? '>' : ' ', fletch_list[i].qty,
                      iteminfo[fletch_list[i].result].name, fletch_list[i].lvl);
        }
        int fc = fletch_cursor;
        if (fletch_list[fc].in2 != IT_NONE)
            draw_text(0, px0 + 6, py0 + 98, "Needs %dx %s + %dx %s",
                      fletch_list[fc].n1, iteminfo[fletch_list[fc].in1].name,
                      fletch_list[fc].n2, iteminfo[fletch_list[fc].in2].name);
        else
            draw_text(0, px0 + 6, py0 + 98, "Needs %dx %s",
                      fletch_list[fc].n1, iteminfo[fletch_list[fc].in1].name);
    }
    else if (ui_mode == UI_CRAFT) {
        int px0 = 56, py0 = 54;
        draw_panel(px0, py0, px0 + 208, py0 + 98);
        draw_text(1, px0 + 6, py0 + 12, "Crafting (needle)");
        draw_text(6, px0 + 6, py0 + 22, "A: make   B: close");
        for (int i = 0; i < CRAFT_COUNT; i++) {
            bool can = level_of(SK_CRAFT) >= craft_list[i].lvl;
            draw_text(i == craft_cursor ? 1 : (can ? 0 : 6),
                      px0 + 6, py0 + 34 + i * 11, "%c %-15s L%d",
                      i == craft_cursor ? '>' : ' ',
                      iteminfo[craft_list[i].result].name, craft_list[i].lvl);
        }
        int cc = craft_cursor;
        draw_text(0, px0 + 6, py0 + 86, "Needs %dx %s + %dx thread",
                  craft_list[cc].n_hide, iteminfo[craft_list[cc].hide].name,
                  craft_list[cc].n_thread);
    }
    else if (ui_mode == UI_EQUIP) {
        int px0 = SCREEN_W - 142, py0 = 30;
        draw_panel(px0, py0, px0 + 134, py0 + 150);
        draw_text(1, px0 + 6, py0 + 12, "Worn Equipment");
        rdpq_set_mode_fill(RGBA32(70, 60, 45, 255));
        for (int i = 0; i < NUM_SLOTS; i++) {
            if (i == equip_cursor) continue;
            int cx = px0 + 16 + (i % 2) * 54, cy = py0 + 22 + (i / 2) * 28;
            rdpq_fill_rectangle(cx, cy, cx + 18, cy + 18);
        }
        rdpq_set_mode_fill(RGBA32(120, 100, 60, 255));
        {
            int cx = px0 + 16 + (equip_cursor % 2) * 54;
            int cy = py0 + 22 + (equip_cursor / 2) * 28;
            rdpq_fill_rectangle(cx, cy, cx + 18, cy + 18);
        }
        rdpq_set_mode_copy(true);
        for (int i = 0; i < NUM_SLOTS; i++) {
            if (equipped[i] == IT_NONE) continue;
            int cx = (px0 + 17 + (i % 2) * 54) & ~1;
            int cy = py0 + 23 + (i / 2) * 28;
            rdpq_sprite_blit(spr[iteminfo[equipped[i]].spr], cx, cy, NULL);
        }
        int sel = equipped[equip_cursor];
        draw_text(1, px0 + 6, py0 + 84, "%s", slot_names[equip_cursor]);
        draw_text(0, px0 + 6, py0 + 94, "%s",
                  sel != IT_NONE ? iteminfo[sel].name : "(empty)");
        draw_text(4, px0 + 6, py0 + 106, "Attack   +%d", equip_atk());
        draw_text(4, px0 + 6, py0 + 115, "Strength +%d", equip_str());
        draw_text(4, px0 + 6, py0 + 124, "Defence  +%d", equip_def());
        draw_text(5, px0 + 6, py0 + 133, "Magic    +%d", equip_mag());
        draw_text(6, px0 + 6, py0 + 144, "A: remove   B: close");
    }
    else if (ui_mode == UI_SPELL) {
        int px0 = SCREEN_W - 166, py0 = 18;
        draw_panel(px0, py0, px0 + 158, py0 + 172);
        draw_text(1, px0 + 6, py0 + 12, "Spellbook");
        draw_text(6, px0 + 6, py0 + 22, "A: select   B: close");
        for (int i = 0; i < NUM_SPELLS; i++) {
            bool can = level_of(SK_MAGIC) >= spellinfo[i].lvl;
            int style = (i == spell_cursor) ? 1 : (i == cast_spell ? 4 : (can ? 0 : 6));
            char mark = (i == spell_cursor) ? '>' : (i == cast_spell ? '*' : ' ');
            if (i == SPELL_MELEE)
                draw_text(style, px0 + 6, py0 + 34 + i * 11, "%c %s", mark,
                          spellinfo[i].name);
            else
                draw_text(style, px0 + 6, py0 + 34 + i * 11, "%c %s  L%d", mark,
                          spellinfo[i].name, spellinfo[i].lvl);
        }
        if (cast_spell == SPELL_MELEE)
            draw_text(0, px0 + 6, py0 + 150, "Style: melee");
        else {
            draw_text(0, px0 + 6, py0 + 150, "Need %dx %s",
                      spellinfo[cast_spell].runes,
                      iteminfo[spellinfo[cast_spell].rune].name);
            if (spellinfo[cast_spell].runes2 > 0)
                draw_text(0, px0 + 6, py0 + 160, " + %dx %s",
                          spellinfo[cast_spell].runes2,
                          iteminfo[spellinfo[cast_spell].rune2].name);
        }
    }
    else if (ui_mode == UI_PRAYER) {
        static const char *catname[NUM_PCAT] = { "Def", "Str", "Atk", "Mag" };
        int px0 = SCREEN_W - 168, py0 = 22;
        draw_panel(px0, py0, px0 + 160, py0 + 152);
        draw_text(1, px0 + 6, py0 + 12, "Prayers");
        draw_text(6, px0 + 6, py0 + 22, "A: toggle   B: close");
        for (int i = 0; i < NUM_PRAYERS; i++) {
            bool can = level_of(SK_PRAY) >= prayerinfo[i].level;
            int style = (i == pray_cursor) ? 1 : (pray_on[i] ? 4 : (can ? 0 : 6));
            char mark = (i == pray_cursor) ? '>' : (pray_on[i] ? '*' : ' ');
            draw_text(style, px0 + 6, py0 + 36 + i * 11, "%c %s (L%d)",
                      mark, prayerinfo[i].name, prayerinfo[i].level);
        }
        draw_text(5, px0 + 6, py0 + 128, "Prayer pts: %d/%d", pray_pts, pray_max());
        draw_text(0, px0 + 6, py0 + 140, "Boost: +%d%% %s",
                  prayerinfo[pray_cursor].pct, catname[prayerinfo[pray_cursor].cat]);
    }
    else if (ui_mode == UI_DIALOG) {
        int px0 = 36, py0 = 64;
        draw_panel(px0, py0, px0 + 248, py0 + 92);
        draw_text(1, px0 + 8, py0 + 14, "%s", dlg_title);
        for (int i = 0; i < dlg_count; i++)
            draw_text(0, px0 + 8, py0 + 30 + i * 12, "%s", dlg_buf[i]);
        draw_text(6, px0 + 8, py0 + 86, "(A) close");
    }
    else if (ui_mode == UI_HELP) {
        int px0 = 40, py0 = 24;
        draw_panel(px0, py0, px0 + 240, py0 + 158);
        draw_text(1, px0 + 8, py0 + 14, "RUNE VALLEY 64");
        draw_text(0, px0 + 8, py0 + 28, "Stick/D-Pad: walk   R: toggle run");
        draw_text(0, px0 + 8, py0 + 40, "A: interact with the world");
        draw_text(0, px0 + 8, py0 + 52, "B: inventory (A: use/equip, C-dn: drop)");
        draw_text(0, px0 + 8, py0 + 64, "C-rt:skills C-up:worn C-lf:spells");
        draw_text(0, px0 + 8, py0 + 76, "C-dn: prayers (altars restore points)");
        draw_text(0, px0 + 8, py0 + 88, "Chop, mine, fish, cook, smith, pray,");
        draw_text(0, px0 + 8, py0 + 98, "wear gear, sling spells, shop, quest.");
        draw_text(3, px0 + 8, py0 + 112, "A three-floor dungeon lurks SW: Warlord,");
        draw_text(3, px0 + 8, py0 + 122, "Demon, then the Ancient Dragon below!");
        draw_text(1, px0 + 8, py0 + 142, "(A) Journal, then Almanac   (B) close");
    }
    else if (ui_mode == UI_QUEST) {
        int px0 = 36, py0 = 24;
        draw_panel(px0, py0, px0 + 248, py0 + 164);
        draw_text(1, px0 + 8, py0 + 12, "Quest Journal");

        draw_text(4, px0 + 8, py0 + 28, "Basic Training");
        if (cquest == CQ_NONE)
            draw_text(0, px0 + 14, py0 + 38, "Seek Sergeant Hardy at the pasture.");
        else if (cquest == CQ_STARTED)
            draw_text(0, px0 + 14, py0 + 38, "Fell cows for Hardy: %d/%d.",
                      cow_kills, CQ_COWS_NEEDED);
        else
            draw_text(6, px0 + 14, py0 + 38, "Complete - you've earned your stripes.");

        draw_text(4, px0 + 8, py0 + 54, "The Chef's Little Problem");
        if (quest_state == QUEST_NONE)
            draw_text(0, px0 + 14, py0 + 64, "Seek out Chef Bouillon by the bank.");
        else if (quest_state == QUEST_ACTIVE)
            draw_text(0, px0 + 14, py0 + 64, "Bash goblins %d/%d, cook 2 shrimp.",
                      quest_kills, QUEST_KILLS_NEEDED);
        else
            draw_text(6, px0 + 14, py0 + 64, "Complete - the valley eats well.");

        draw_text(4, px0 + 8, py0 + 80, "The Warlord's Bane");
        const char *wl;
        switch (wquest) {
        case WQ_NONE:    wl = "Seek Sir Garrick by the cave mouth."; break;
        case WQ_STARTED: wl = "Bring Sir Garrick 5 bones."; break;
        case WQ_SCOUTED: wl = "Bring 2 iron bars + 300 coins."; break;
        case WQ_ARMED:   wl = "Slay the Warlord with the Bane."; break;
        case WQ_SLAIN:   wl = "Return to Sir Garrick for reward."; break;
        default:         wl = "Complete - the Warlord is slain!"; break;
        }
        draw_text(wquest >= WQ_DONE ? 6 : 0, px0 + 14, py0 + 90, "%s", wl);
        if (wquest == WQ_NONE)
            draw_text(6, px0 + 14, py0 + 100, "(needs Att %d, Def %d, Smith %d)",
                      WQ_REQ_ATT, WQ_REQ_DEF, WQ_REQ_SMITH);

        if (wquest >= WQ_DONE) {
            draw_text(4, px0 + 8, py0 + 116, "The Demon Below");
            const char *dl;
            switch (dquest) {
            case DQ_NONE:    dl = "Sir Garrick has a deeper task..."; break;
            case DQ_STARTED: dl = "Slay the Demon on dungeon floor 2."; break;
            case DQ_SLAIN:   dl = "Return to Sir Garrick for reward."; break;
            default:         dl = "Complete - the Demon is destroyed!"; break;
            }
            draw_text(dquest >= DQ_DONE ? 6 : 0, px0 + 14, py0 + 126, "%s", dl);
            if (dquest == DQ_NONE)
                draw_text(6, px0 + 14, py0 + 136, "(needs Att %d, Def %d, HP %d)",
                          DQ_REQ_ATT, DQ_REQ_DEF, DQ_REQ_HP);
        }

        draw_text(6, px0 + 8, py0 + 152, "(A) Almanac   (B) close");
    }
    else if (ui_mode == UI_ALMANAC) {
        int px0 = 16, py0 = 16;
        draw_panel(px0, py0, px0 + 288, py0 + 208);
        draw_text(1, px0 + 8, py0 + 12, "Almanac");
        draw_text(6, px0 + 132, py0 + 12, "D-pad: scroll/page");
        for (int i = 0; i < ALMANAC_VIS; i++) {
            int li = almanac_scroll + i;
            if (li >= almanac_count) break;
            draw_text(almanac_style[li], px0 + 8, py0 + 28 + i * 11,
                      "%s", almanac_buf[li]);
        }
        /* scroll position indicator + controls */
        int shown = almanac_scroll + ALMANAC_VIS;
        if (shown > almanac_count) shown = almanac_count;
        draw_text(6, px0 + 8, py0 + 198, "%d-%d of %d   (A) Help   (B) close",
                  almanac_count ? almanac_scroll + 1 : 0, shown, almanac_count);
    }

    rdpq_detach_show();
}

/* ------------------------------------------------------------ input */

static void start_play(void)
{
    game_state = STATE_PLAY;
    msg("Welcome to Rune Valley 64.");
    msg("Press Start for help.");
}

static void handle_input(void)
{
    joypad_poll();
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);

    if (game_state == STATE_TITLE) {
        if (save_found) {
            if (pressed.d_up || pressed.d_down) title_sel ^= 1;
            if (pressed.a || pressed.start) {
                if (title_sel == 0) load_game();
                start_play();
            }
        } else if (pressed.start || pressed.a) {
            start_play();
        }
        return;
    }

    /* global toggles */
    if (pressed.start) ui_mode = (ui_mode == UI_HELP) ? UI_NONE : UI_HELP;
    if (pressed.l) {
        music_on = !music_on;
        mixer_ch_set_vol(CH_MUSIC, music_on ? 0.5f : 0.0f,
                         music_on ? 0.5f : 0.0f);
        msg(music_on ? "Music on." : "Music off.");
    }
    if (pressed.r) {
        pl.run_on = !pl.run_on;
        msg(pl.run_on ? "Run mode on." : "Run mode off.");
    }
    if (pressed.c_right)
        ui_mode = (ui_mode == UI_SKILLS) ? UI_NONE : UI_SKILLS;
    if (pressed.c_up)
        ui_mode = (ui_mode == UI_EQUIP) ? UI_NONE : UI_EQUIP;
    if (pressed.c_left) {
        ui_mode = (ui_mode == UI_SPELL) ? UI_NONE : UI_SPELL;
        spell_cursor = cast_spell;
    }
    /* C-down opens the prayer book (it doubles as "drop" inside the inventory) */
    if (pressed.c_down && ui_mode != UI_INV)
        ui_mode = (ui_mode == UI_PRAYER) ? UI_NONE : UI_PRAYER;

    if (ui_mode == UI_INV) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_left  && inv_cursor % 4 > 0) inv_cursor--;
        if (pressed.d_right && inv_cursor % 4 < 3) inv_cursor++;
        if (pressed.d_up    && inv_cursor >= 4) inv_cursor -= 4;
        if (pressed.d_down  && inv_cursor < INV_SIZE - 4) inv_cursor += 4;
        if (pressed.a) use_inv_item(inv_cursor);
        if (pressed.c_down && inv[inv_cursor] != IT_NONE) {
            if (iteminfo[inv[inv_cursor]].tool)
                msg("You'd best hold onto that.");
            else {
                msg("You discard the %s.", iteminfo[inv[inv_cursor]].name);
                inv[inv_cursor] = IT_NONE;
                inv_qty[inv_cursor] = 0;
            }
        }
        return;
    }
    if (ui_mode == UI_BANK) {
        if (pressed.b) {
            ui_mode = UI_NONE;
            save_game();
            if (eeprom_present()) msg("Your progress is saved.");
            return;
        }
        int n = bank_rows();
        if (pressed.d_up && bank_cursor > 0) bank_cursor--;
        if (pressed.d_down && bank_cursor < n - 1) bank_cursor++;
        if (pressed.a) bank_withdraw(bank_cursor);
        if (pressed.z) bank_deposit_all();
        return;
    }
    if (ui_mode == UI_SHOP) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.z) { shop_mode ^= 1; shop_cursor = 0; }
        int n = shop_mode ? shop_sell_count() : shop_stock_count(shop_id);
        if (pressed.d_up && shop_cursor > 0) shop_cursor--;
        if (pressed.d_down && shop_cursor < n - 1) shop_cursor++;
        if (pressed.a && n > 0) {
            if (shop_mode) shop_sell(shop_sell_list[shop_cursor]);
            else           shop_buy(shop_stock[shop_id][shop_cursor]);
        }
        return;
    }
    if (ui_mode == UI_SMITH) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_up && smith_cursor > 0) smith_cursor--;
        if (pressed.d_down && smith_cursor < SMITH_COUNT - 1) smith_cursor++;
        if (pressed.a) smith_make(smith_cursor);
        return;
    }
    if (ui_mode == UI_FLETCH) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_up && fletch_cursor > 0) fletch_cursor--;
        if (pressed.d_down && fletch_cursor < FLETCH_COUNT - 1) fletch_cursor++;
        if (pressed.a) fletch_make(fletch_cursor);
        return;
    }
    if (ui_mode == UI_CRAFT) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_up && craft_cursor > 0) craft_cursor--;
        if (pressed.d_down && craft_cursor < CRAFT_COUNT - 1) craft_cursor++;
        if (pressed.a) craft_make(craft_cursor);
        return;
    }
    if (ui_mode == UI_EQUIP) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_left  && (equip_cursor & 1)) equip_cursor--;
        if (pressed.d_right && !(equip_cursor & 1)) equip_cursor++;
        if (pressed.d_up    && equip_cursor >= 2) equip_cursor -= 2;
        if (pressed.d_down  && equip_cursor < 2) equip_cursor += 2;
        if (pressed.a) unequip(equip_cursor);
        return;
    }
    if (ui_mode == UI_SPELL) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_up && spell_cursor > 0) spell_cursor--;
        if (pressed.d_down && spell_cursor < NUM_SPELLS - 1) spell_cursor++;
        if (pressed.a) {
            if (spell_cursor != SPELL_MELEE &&
                level_of(SK_MAGIC) < spellinfo[spell_cursor].lvl) {
                msg("You need a Magic level of %d for that spell.",
                    spellinfo[spell_cursor].lvl);
            } else if (spell_cursor == SPELL_HOME) {
                do_teleport(SPELL_HOME, pl.spawn_x, pl.spawn_y, "Rune Valley");
                ui_mode = UI_NONE;
                return;
            } else if (spell_cursor == SPELL_BANK) {
                do_teleport(SPELL_BANK, 24, 7, "the bank");
                ui_mode = UI_NONE;
                return;
            } else if (spell_cursor == SPELL_CAVE) {
                do_teleport(SPELL_CAVE, 7, 29, "the cave mouth");
                ui_mode = UI_NONE;
                return;
            } else {
                cast_spell = spell_cursor;
                msg(cast_spell == SPELL_MELEE ? "You ready your weapon."
                    : "You will now cast %s.", spellinfo[cast_spell].name);
            }
        }
        return;
    }
    if (ui_mode == UI_PRAYER) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.d_up && pray_cursor > 0) pray_cursor--;
        if (pressed.d_down && pray_cursor < NUM_PRAYERS - 1) pray_cursor++;
        if (pressed.a) toggle_prayer(pray_cursor);
        return;
    }
    if (ui_mode == UI_DIALOG) {
        if (pressed.a || pressed.b) ui_mode = UI_NONE;
        return;
    }
    if (ui_mode == UI_QUEST) {
        if (pressed.a) { build_almanac(); almanac_scroll = 0; ui_mode = UI_ALMANAC; return; }
        if (pressed.b) { ui_mode = UI_NONE; return; }
        return;
    }
    if (ui_mode == UI_ALMANAC) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (pressed.a) { ui_mode = UI_HELP; return; }
        int maxs = almanac_count - ALMANAC_VIS;
        if (maxs < 0) maxs = 0;
        if (pressed.d_up)    almanac_scroll -= 1;
        if (pressed.d_down)  almanac_scroll += 1;
        if (pressed.d_left)  almanac_scroll -= ALMANAC_VIS;
        if (pressed.d_right) almanac_scroll += ALMANAC_VIS;
        if (almanac_scroll < 0) almanac_scroll = 0;
        if (almanac_scroll > maxs) almanac_scroll = maxs;
        return;
    }
    if (ui_mode == UI_SKILLS || ui_mode == UI_HELP) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        if (ui_mode == UI_HELP && pressed.a) { ui_mode = UI_QUEST; return; }
        /* world input continues while these are open */
    }

    if (pressed.b && ui_mode == UI_NONE) { ui_mode = UI_INV; return; }

    if (pressed.a && ui_mode == UI_NONE) interact();

    /* movement: d-pad or stick */
    int dx = 0, dy = 0;
    if (held.d_left)  dx = -1;
    if (held.d_right) dx = 1;
    if (held.d_up)    dy = -1;
    if (held.d_down)  dy = 1;
    if (!dx && !dy) {
        if (in.stick_x < -20) dx = -1;
        if (in.stick_x >  20) dx = 1;
        if (in.stick_y >  20) dy = -1;   /* stick up is positive */
        if (in.stick_y < -20) dy = 1;
    }
    if (dx || dy) try_walk(dx, dy);
}

/* ------------------------------------------------------------ main */

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    dfs_init(DFS_DEFAULT_LOCATION);
    rdpq_init();
    joypad_init();
    timer_init();

    audio_init(22050, 4);
    mixer_init(8);

    srand(get_ticks());

    /* font + styles */
    rdpq_font_t *font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO);
    static const color_t style_colors[] = {
        { 255, 255, 255, 255 },   /* 0 white  */
        { 255, 222,  60, 255 },   /* 1 yellow */
        {  60,  40,  20, 255 },   /* 2 chat brown */
        { 230,  50,  50, 255 },   /* 3 red    */
        {  90, 220,  90, 255 },   /* 4 green  */
        { 120, 220, 255, 255 },   /* 5 cyan   */
        { 200, 200, 200, 255 },   /* 6 gray   */
    };
    for (int i = 0; i < 7; i++)
        rdpq_font_style(font, i, &(rdpq_fontstyle_t){ .color = style_colors[i] });
    rdpq_text_register_font(FONT_ID, font);

    /* assets */
    char path[64];
    for (int i = 0; i < NUM_SPR; i++) {
        snprintf(path, sizeof path, "rom:/%s.sprite", spr_files[i]);
        spr[i] = sprite_load(path);
    }
    for (int i = 0; i < NUM_SND; i++) {
        snprintf(path, sizeof path, "rom:/%s.wav64", snd_files[i]);
        wav64_open(&snd[i], path);
    }
    wav64_open(&music, "rom:/music.wav64");
    wav64_set_loop(&music, true);
    wav64_play(&music, CH_MUSIC);
    mixer_ch_set_vol(CH_MUSIC, 0.5f, 0.5f);

    /* game state */
    init_xp_table();
    load_map(MAP_OVERWORLD);
    init_bots();
    for (int i = 0; i < NUM_SKILLS; i++) xp[i] = 0;
    xp[SK_HP] = xp_table[10];                  /* hitpoints starts at 10 */
    pl.hp = 10;
    pl.tx = pl.spawn_x; pl.ty = pl.spawn_y;
    pl.mtx = pl.tx; pl.mty = pl.ty;
    pl.px = pl.tx * TILE + 8; pl.py = pl.ty * TILE + 12;
    pl.facing = 0;
    pl.run_energy = 100;
    pl.state = ST_IDLE;
    for (int i = 0; i < INV_SIZE; i++) { inv[i] = IT_NONE; inv_qty[i] = 0; }
    inv[0] = IT_AXE; inv[1] = IT_PICK; inv[2] = IT_NET; inv[3] = IT_TINDER;
    inv[4] = IT_HAMMER; inv[5] = IT_BRONZE_SWORD;
    inv[6] = IT_AIR_RUNE; inv_qty[6] = 25;     /* a starter stack for Magic */
    for (int i = 0; i < INV_SIZE; i++)
        if (inv[i] != IT_NONE && inv_qty[i] == 0) inv_qty[i] = 1;
    for (int i = 0; i < NUM_SLOTS; i++) equipped[i] = IT_NONE;
    cast_spell = SPELL_MELEE;
    deactivate_all_prayers();
    pray_pts = pray_max();
    pray_drain_acc = pray_regen = 0;
    gp = 25;                      /* a few starter coins */
    quest_state = QUEST_NONE; quest_kills = 0; wquest = WQ_NONE; dquest = DQ_NONE;
    cquest = CQ_NONE; cow_kills = 0;
    dungeon_floor = 1;
    memset(bank, 0, sizeof bank);
    for (int i = 0; i < CHAT_LINES; i++) chat[i][0] = 0;

    /* the save probe happens a few frames into the main loop: probing the
       EEPROM in the middle of subsystem init destabilizes the joybus */

    uint64_t last_ms = get_ticks_ms();
    int tick_acc = 0;

    while (1) {
        uint64_t now = get_ticks_ms();
        int dt_ms = (int)(now - last_ms);
        if (dt_ms > 100) dt_ms = 100;
        last_ms = now;

        handle_input();

        tick_acc += dt_ms;
        while (tick_acc >= TICK_MS) {
            tick_acc -= TICK_MS;
            game_tick();
        }

        update_movement(dt_ms / 1000.0f);

        frame_counter++;
        if (frame_counter == 10) save_found = save_present();
        render();

        if (audio_can_write()) {
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }
    }
}
