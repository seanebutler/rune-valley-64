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
#define MAX_GOB        8
#define MAX_XPDROP     8
#define CHAT_LINES     6

#define WALK_SPEED     (TILE / 0.6f)        /* 1 tile per tick  */
#define RUN_SPEED      (2 * TILE / 0.6f)    /* 2 tiles per tick */

/* ------------------------------------------------------------ terrain & map */

enum { TER_GRASS, TER_PATH, TER_SAND, TER_WATER, TER_BRIDGE, TER_WALL, TER_FLOOR };
enum { OBJ_NONE, OBJ_TREE, OBJ_OAK, OBJ_STUMP, OBJ_ROCK_COPPER, OBJ_ROCK_TIN,
       OBJ_ROCK_IRON, OBJ_ROCK_EMPTY, OBJ_FISH, OBJ_BOOTH, OBJ_FIRE,
       OBJ_ESSENCE, OBJ_ALTAR_AIR, OBJ_ALTAR_FIRE };

static const char *map_rows[MAP_H] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT~~~~TTTTTT",
    "T.....................................~~~~.....T",
    "T.................wwwwwwwwwwwww.......~~~~.....T",
    "T...E.E...........wfffffffffffw.......~~~~.....T",
    "T.................wfffffffffffw.......~~~~.....T",
    "T....A............wfffffffffffw.......~~~~.....T",
    "T.................wfBBBBBBBBBfw.......~~~~.....T",
    "T.................wfffffffffffw.......~~~~.....T",
    "T.................wwwwwpppwwwww.......~~~~.....T",
    "T......................ppp............~~~~.....T",
    "T....T.................ppp............~~~~.....T",
    "T.............T........ppp.....T......~~~~.....T",
    "T......................p@p............~~~~.....T",
    "T..T...................ppp.......T....~~~~.....T",
    "T............O.........ppp............~~~~.....T",
    "T.......pppppppppppppppppp............~~~~.....T",
    "T.......pp.............ppp............~~~~.....T",
    "T.......pp.............ppp......T.....~~~~.G...T",
    "T.......pp.............ppppppppppppppp====ppp..T",
    "T.......pp............................~~~~.....T",
    "T.......pp..........................ss~~~~..G..T",
    "T.......pp..........................ss~~~~.....T",
    "T.......pp..........................ssF~~~..T..T",
    "T.....C.pp...N......................ss~~~~.G...T",
    "T.......pp..........................ss~~~~.....T",
    "T.......C...N.......................ss~~~~.....T",
    "T...................................ss~~~~...G.T",
    "T...................................ssF~~~.....T",
    "T......I..I......O..................ss~~~~.....T",
    "T...................................ss~~~~.....T",
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

enum { SK_ATT, SK_STR, SK_DEF, SK_HP, SK_WC, SK_MINE, SK_FISH, SK_FM, SK_COOK,
       SK_PRAY, SK_RC, NUM_SKILLS };

static const char *skill_names[NUM_SKILLS] = {
    "Attack", "Strength", "Defence", "Hitpoints", "Woodcutting",
    "Mining", "Fishing", "Firemaking", "Cooking", "Prayer", "Runecraft"
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
       IT_TINDER, IT_BONES, IT_ESSENCE, IT_AIR_RUNE, IT_FIRE_RUNE, NUM_ITEMS };

typedef struct { const char *name; int spr; int heal; bool tool; } iteminfo_t;

/* ------------------------------------------------------------ sprites */

enum {
    SPR_GRASS_A, SPR_GRASS_B, SPR_PATH, SPR_SAND, SPR_WATER_A, SPR_WATER_B,
    SPR_FLOOR, SPR_WALL, SPR_BRIDGE,
    SPR_TREE, SPR_OAK, SPR_STUMP, SPR_ROCK_C, SPR_ROCK_T, SPR_ROCK_I,
    SPR_ROCK_E, SPR_FISH_A, SPR_FISH_B, SPR_BOOTH, SPR_FIRE_A, SPR_FIRE_B,
    SPR_PL_DOWN_A, SPR_PL_DOWN_B, SPR_PL_UP_A, SPR_PL_UP_B,
    SPR_PL_SIDE_A, SPR_PL_SIDE_B, SPR_GOB_A, SPR_GOB_B,
    SPR_I_LOGS, SPR_I_OAK_LOGS, SPR_I_COPPER, SPR_I_TIN, SPR_I_IRON,
    SPR_I_RAW_SHRIMP, SPR_I_SHRIMP, SPR_I_BURNT, SPR_I_AXE, SPR_I_PICK,
    SPR_I_NET, SPR_I_TINDER, SPR_I_BONES,
    SPR_ESSENCE, SPR_ALTAR_AIR, SPR_ALTAR_FIRE,
    SPR_I_ESSENCE, SPR_I_AIR_RUNE, SPR_I_FIRE_RUNE,
    SPR_BOTA_DOWN_A, SPR_BOTA_DOWN_B, SPR_BOTA_UP_A, SPR_BOTA_UP_B,
    SPR_BOTA_SIDE_A, SPR_BOTA_SIDE_B,
    SPR_BOTB_DOWN_A, SPR_BOTB_DOWN_B, SPR_BOTB_UP_A, SPR_BOTB_UP_B,
    SPR_BOTB_SIDE_A, SPR_BOTB_SIDE_B,
    SPR_BOTC_DOWN_A, SPR_BOTC_DOWN_B, SPR_BOTC_UP_A, SPR_BOTC_UP_B,
    SPR_BOTC_SIDE_A, SPR_BOTC_SIDE_B,
    NUM_SPR
};

static const char *spr_files[NUM_SPR] = {
    "tile_grass_a", "tile_grass_b", "tile_path", "tile_sand", "tile_water_a",
    "tile_water_b", "tile_floor", "tile_wall", "tile_bridge",
    "obj_tree", "obj_oak", "obj_stump", "obj_rock_copper", "obj_rock_tin",
    "obj_rock_iron", "obj_rock_empty", "obj_fish_a", "obj_fish_b", "obj_booth",
    "obj_fire_a", "obj_fire_b",
    "pl_down_a", "pl_down_b", "pl_up_a", "pl_up_b", "pl_side_a", "pl_side_b",
    "goblin_a", "goblin_b",
    "item_logs", "item_oak_logs", "item_copper_ore", "item_tin_ore",
    "item_iron_ore", "item_raw_shrimp", "item_shrimp", "item_burnt_shrimp",
    "item_axe", "item_pickaxe", "item_net", "item_tinderbox", "item_bones",
    "obj_essence_rock", "obj_altar_air", "obj_altar_fire",
    "item_essence", "item_air_rune", "item_fire_rune",
    "bota_down_a", "bota_down_b", "bota_up_a", "bota_up_b",
    "bota_side_a", "bota_side_b",
    "botb_down_a", "botb_down_b", "botb_up_a", "botb_up_b",
    "botb_side_a", "botb_side_b",
    "botc_down_a", "botc_down_b", "botc_up_a", "botc_up_b",
    "botc_side_a", "botc_side_b"
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
    [IT_AIR_RUNE]   = { "Air rune",    SPR_I_AIR_RUNE,   0, false },
    [IT_FIRE_RUNE]  = { "Fire rune",   SPR_I_FIRE_RUNE,  0, false },
};

/* ------------------------------------------------------------ audio */

enum { SND_CHOP, SND_MINE, SND_SPLASH, SND_HIT, SND_EAT, SND_COOK, SND_FIRE,
       SND_LEVELUP, SND_DEATH, SND_CRAFT, NUM_SND };
static const char *snd_files[NUM_SND] = {
    "chop", "mine", "splash", "hit", "eat", "cook", "fire", "levelup", "death",
    "craft"
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

typedef enum { ST_IDLE, ST_CHOP, ST_MINE, ST_FISH, ST_COOK, ST_LIGHT, ST_FIGHT }
    pstate_t;

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
} pl;

static int inv[INV_SIZE];
static int bank[NUM_ITEMS];

/* ------------------------------------------------------------ goblins */

typedef struct {
    bool  exists;
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
} gob_t;

static gob_t gob[MAX_GOB];
static int num_gob = 0;
#define GOB_MAX_HP 5

/* ------------------------------------------------------------ UI state */

typedef enum { UI_NONE, UI_INV, UI_SKILLS, UI_BANK, UI_HELP } ui_t;
static ui_t ui_mode = UI_NONE;
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

static int inv_count(int item)
{
    int n = 0;
    for (int i = 0; i < INV_SIZE; i++) if (inv[i] == item) n++;
    return n;
}

static bool has_item(int item) { return inv_count(item) > 0; }

static bool add_item(int item)
{
    for (int i = 0; i < INV_SIZE; i++)
        if (inv[i] == IT_NONE) { inv[i] = item; return true; }
    msg("Your inventory is too full to carry any more.");
    return false;
}

static bool remove_item(int item)
{
    for (int i = 0; i < INV_SIZE; i++)
        if (inv[i] == item) { inv[i] = IT_NONE; return true; }
    return false;
}

static bool inv_full(void)
{
    for (int i = 0; i < INV_SIZE; i++) if (inv[i] == IT_NONE) return false;
    return true;
}

static int chance(int percent) { return (rand() % 100) < percent; }

static int cheb(int x0, int y0, int x1, int y1)
{
    int dx = abs(x0 - x1), dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

/* ------------------------------------------------------------ map */

static void init_map(void)
{
    num_gob = 0;
    pl.spawn_x = 24; pl.spawn_y = 12;
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
            case 'F': ter = TER_WATER; obj = OBJ_FISH; break;
            case 'E': obj = OBJ_ESSENCE;    break;
            case 'A': obj = OBJ_ALTAR_AIR;  break;
            case 'R': obj = OBJ_ALTAR_FIRE; break;
            case 'G':
                if (num_gob < MAX_GOB) {
                    gob_t *g = &gob[num_gob++];
                    memset(g, 0, sizeof *g);
                    g->exists = true;
                    g->tx = g->sx = g->mtx = x;
                    g->ty = g->sy = g->mty = y;
                    g->px = x * TILE + 8; g->py = y * TILE + 12;
                    g->hp = GOB_MAX_HP;
                }
                break;
            case '@': ter = TER_PATH; pl.spawn_x = x; pl.spawn_y = y; break;
            }
            terrain[y][x] = ter;
            object[y][x] = obj;
            obj_orig[y][x] = obj;
            obj_timer[y][x] = 0;
        }
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

/* ------------------------------------------------------------ combat & death */

static void player_die(void)
{
    msg("Oh dear, you are dead!");
    sfx_ui(SND_DEATH);
    pl.hp = level_of(SK_HP);
    pl.tx = pl.spawn_x; pl.ty = pl.spawn_y;
    pl.mtx = pl.tx; pl.mty = pl.ty;
    pl.px = pl.tx * TILE + 8; pl.py = pl.ty * TILE + 12;
    pl.moving = false;
    pl.state = ST_IDLE;
    ui_mode = UI_NONE;
    for (int i = 0; i < num_gob; i++) {
        gob[i].aggro = false;
        gob[i].hurt_timer = 0;
    }
}

static void hurt_player(int dmg)
{
    pl.hp -= dmg;
    pl.hitsplat = dmg;
    pl.hitsplat_t = 40;
    if (pl.hp <= 0) player_die();
}

static void goblin_die(gob_t *g)
{
    g->dead = true;
    g->respawn = 25;
    g->aggro = false;
    g->hurt_timer = 0;
    msg("You have defeated the goblin!");
    if (!inv_full()) {
        add_item(IT_BONES);
        msg("The goblin drops some bones. You take them.");
    }
    if (pl.state == ST_FIGHT) pl.state = ST_IDLE;
}

static void player_attack(gob_t *g)
{
    int att = level_of(SK_ATT), str = level_of(SK_STR);
    int max_hit = 1 + str / 8;
    int p_hit = 100 * (att + 8) / (att + 18);
    int dmg = chance(p_hit) ? (rand() % (max_hit + 1)) : 0;
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
    if (g->hp <= 0) goblin_die(g);
}

static void goblin_attack(gob_t *g)
{
    int def = level_of(SK_DEF);
    int p_hit = 55 - def;
    if (p_hit < 15) p_hit = 15;
    int dmg = chance(p_hit) ? (rand() % 2) : 0;
    sfx(SND_HIT);
    hurt_player(dmg);
    /* auto-retaliate */
    if (pl.state == ST_IDLE && pl.hp > 0) {
        pl.state = ST_FIGHT;
        pl.fight_target = (int)(g - gob);
        pl.atk_cd = 2;
    }
}

/* ------------------------------------------------------------ skilling ticks */

static void stop_action(void) { pl.state = ST_IDLE; }

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
        int p = oak ? (10 + wc * 3 / 2) : (25 + wc * 2);
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
            o != OBJ_ESSENCE) {
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
        bool iron = (o == OBJ_ROCK_IRON);
        if (iron && mn < 15) { msg("You need a Mining level of 15 to mine iron."); stop_action(); break; }
        if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
        int p = iron ? (8 + mn * 3 / 2) : (25 + mn * 2);
        if (p > 90) p = 90;
        if (chance(p)) {
            sfx(SND_MINE);
            int item = iron ? IT_IRON : (o == OBJ_ROCK_COPPER ? IT_COPPER : IT_TIN);
            add_item(item);
            msg("You manage to mine some %s.",
                iron ? "iron" : (item == IT_COPPER ? "copper" : "tin"));
            add_xp(SK_MINE, iron ? 350 : 175, true);
            object[ay][ax] = OBJ_ROCK_EMPTY;
            obj_timer[ay][ax] = iron ? 9 : 4;
            stop_action();
        } else {
            sfx(SND_MINE);
        }
        break;
    }
    case ST_FISH: {
        if (object[ay][ax] != OBJ_FISH) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 5;
        if (inv_full()) { msg("Your inventory is too full to carry any more."); stop_action(); break; }
        int fs = level_of(SK_FISH);
        int p = 20 + fs * 2;
        if (p > 85) p = 85;
        if (chance(p)) {
            sfx(SND_SPLASH);
            add_item(IT_RAW_SHRIMP);
            msg("You catch some shrimp.");
            add_xp(SK_FISH, 100, true);
        } else {
            sfx(SND_SPLASH);
        }
        break;
    }
    case ST_COOK: {
        if (object[ay][ax] != OBJ_FIRE) { stop_action(); break; }
        if (!has_item(IT_RAW_SHRIMP)) { stop_action(); break; }
        if (--pl.act_timer > 0) break;
        pl.act_timer = 4;
        remove_item(IT_RAW_SHRIMP);
        int ck = level_of(SK_COOK);
        int p_burn = 45 - ck;
        if (ck >= 34) p_burn = 0;
        if (p_burn < 5 && ck < 34) p_burn = 5;
        sfx(SND_COOK);
        if (chance(p_burn)) {
            add_item(IT_BURNT);
            msg("You accidentally burn the shrimp.");
        } else {
            add_item(IT_SHRIMP);
            msg("You successfully cook some shrimp.");
            add_xp(SK_COOK, 300, true);
        }
        if (!has_item(IT_RAW_SHRIMP)) stop_action();
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
    case ST_FIGHT: {
        gob_t *g = &gob[pl.fight_target];
        if (!g->exists || g->dead) { stop_action(); break; }
        int d = cheb(pl.tx, pl.ty, g->tx, g->ty);
        if (d <= 1) {
            if (--pl.atk_cd <= 0) {
                pl.atk_cd = 4;
                player_attack(g);
            }
        } else if (d > 6) {
            stop_action();
        } else if (!pl.moving) {
            /* chase one tile */
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

static void tick_goblins(void)
{
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists) continue;
        if (g->dead) {
            if (--g->respawn <= 0 && gob_tile_free(g->sx, g->sy, g)) {
                g->dead = false;
                g->hp = GOB_MAX_HP;
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
        if (!g->aggro && dp <= 3) g->aggro = true;

        if (g->aggro) {
            if (cheb(g->tx, g->ty, g->sx, g->sy) > 8) {
                g->aggro = false;
            } else if (dp <= 1) {
                if (--g->atk_cd <= 0) {
                    g->atk_cd = 4;
                    goblin_attack(g);
                }
            } else {
                gob_step(g, pl.tx, pl.ty);
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
    int   goal_x, goal_y, goal_obj;
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
    /* 1 in 4: just wander somewhere walkable nearby */
    if (chance(25)) {
        for (int tries = 0; tries < 8; tries++) {
            int x = b->tx + rand() % 13 - 6, y = b->ty + rand() % 13 - 6;
            if (tile_walkable(x, y)) {
                b->goal_x = x; b->goal_y = y; b->goal_obj = OBJ_NONE;
                b->state = BS_GOTO;
                return;
            }
        }
        b->state = BS_IDLE; b->idle_ticks = 2 + rand() % 4;
        return;
    }
    /* otherwise find something to harvest */
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
        case BS_GOTO:
            if (b->goal_obj != OBJ_NONE && !bot_goal_valid(b)) { bot_pick_goal(b); break; }
            if (b->goal_obj != OBJ_NONE
                    ? cheb(b->tx, b->ty, b->goal_x, b->goal_y) <= 1
                    : (b->tx == b->goal_x && b->ty == b->goal_y)) {
                if (b->goal_obj == OBJ_NONE) {
                    b->state = BS_IDLE;
                    b->idle_ticks = 2 + rand() % 5;
                } else {
                    b->state = BS_WORK;
                    bot_face_goal(b);
                    b->act_timer = 4;
                    b->work_left = 5 + rand() % 10;
                }
                break;
            }
            bot_step(b, b->goal_x, b->goal_y);
            if (b->stuck > 4) bot_pick_goal(b);
            break;
        case BS_WORK: {
            if (!bot_goal_valid(b)) {
                b->state = BS_IDLE; b->idle_ticks = 1 + rand() % 3;
                break;
            }
            if (--b->act_timer > 0) break;
            b->act_timer = 4;
            if (chance(45)) {
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
    tick_bots();

    /* hp regen: 1 per minute */
    if (pl.hp < level_of(SK_HP) && ++pl.regen >= 100) {
        pl.regen = 0;
        pl.hp++;
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

static void interact(void)
{
    gob_t *g = adjacent_goblin();
    if (g) {
        pl.state = ST_FIGHT;
        pl.fight_target = (int)(g - gob);
        pl.atk_cd = 2;
        msg("You attack the goblin!");
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
        if (!has_item(IT_AXE)) { msg("You need an axe to chop down this tree."); return; }
        pl.state = ST_CHOP; pl.act_timer = 2;
        msg("You swing your axe at the tree.");
        break;
    case OBJ_ROCK_COPPER: case OBJ_ROCK_TIN: case OBJ_ROCK_IRON: case OBJ_ESSENCE:
        if (!has_item(IT_PICK)) { msg("You need a pickaxe to mine this rock."); return; }
        pl.state = ST_MINE; pl.act_timer = 2;
        msg("You swing your pick at the rock.");
        break;
    case OBJ_ALTAR_AIR: case OBJ_ALTAR_FIRE: {
        bool fire = (t.obj == OBJ_ALTAR_FIRE);
        if (fire && level_of(SK_RC) < 14) {
            msg("You need a Runecraft level of 14 to bind fire runes.");
            return;
        }
        int n = 0;
        while (remove_item(IT_ESSENCE)) {
            add_item(fire ? IT_FIRE_RUNE : IT_AIR_RUNE);
            n++;
        }
        if (!n) { msg("You need some rune essence to craft runes."); return; }
        sfx_ui(SND_CRAFT);
        msg("You bind the temple's power into %s rune%s.",
            fire ? "fire" : "air", n > 1 ? "s" : "");
        add_xp(SK_RC, n * (fire ? 70 : 50), true);
        break;
    }
    case OBJ_ROCK_EMPTY:
        msg("There is no ore currently available in this rock.");
        break;
    case OBJ_FISH:
        if (!has_item(IT_NET)) { msg("You need a net to catch these fish."); return; }
        pl.state = ST_FISH; pl.act_timer = 3;
        msg("You cast out your net...");
        break;
    case OBJ_BOOTH:
        ui_mode = UI_BANK;
        bank_cursor = 0;
        break;
    case OBJ_FIRE:
        if (has_item(IT_RAW_SHRIMP)) {
            pl.state = ST_COOK; pl.act_timer = 2;
            msg("You cook the shrimp on the fire.");
        } else {
            msg("The fire is nice and warm.");
        }
        break;
    }
}

/* hint string for the contextual action */
static const char *context_hint(void)
{
    if (adjacent_goblin()) return "A: Attack Goblin (level 2)";
    target_t t;
    if (!find_target(&t)) return NULL;
    switch (t.obj) {
    case OBJ_TREE:        return "A: Chop down Tree";
    case OBJ_OAK:         return "A: Chop down Oak";
    case OBJ_ROCK_COPPER: return "A: Mine Copper rock";
    case OBJ_ROCK_TIN:    return "A: Mine Tin rock";
    case OBJ_ROCK_IRON:   return "A: Mine Iron rock";
    case OBJ_FISH:        return "A: Net Fishing spot";
    case OBJ_BOOTH:       return "A: Use Bank booth";
    case OBJ_FIRE:        return "A: Cook on Fire";
    case OBJ_ESSENCE:     return "A: Mine Essence rock";
    case OBJ_ALTAR_AIR:   return "A: Craft runes at Air altar";
    case OBJ_ALTAR_FIRE:  return "A: Craft runes at Fire altar";
    }
    return NULL;
}

/* ------------------------------------------------------------ inventory use */

static void use_inv_item(int slot)
{
    int it = inv[slot];
    switch (it) {
    case IT_NONE: break;
    case IT_SHRIMP:
        if (pl.eat_cd > 0) break;
        inv[slot] = IT_NONE;
        pl.eat_cd = 3;
        pl.hp += iteminfo[IT_SHRIMP].heal;
        int maxhp = level_of(SK_HP);
        if (pl.hp > maxhp) pl.hp = maxhp;
        sfx_ui(SND_EAT);
        msg("You eat the shrimp. It heals some health.");
        break;
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
    case IT_RAW_SHRIMP:
        msg("You should cook this on a fire first.");
        break;
    case IT_BURNT:
        msg("Ugh, there's nothing left of it. Best discard it.");
        break;
    case IT_ESSENCE:   msg("A chunk of raw magical essence."); break;
    case IT_AIR_RUNE:  msg("A rune imbued with the power of air."); break;
    case IT_FIRE_RUNE: msg("A rune imbued with the power of fire."); break;
    case IT_AXE:    msg("A woodcutter's best friend."); break;
    case IT_PICK:   msg("Used for mining rocks."); break;
    case IT_NET:    msg("Used to catch shrimp at fishing spots."); break;
    case IT_TINDER: msg("Useful for lighting fires."); break;
    }
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
        bank[inv[i]]++;
        inv[i] = IT_NONE;
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

/* ------------------------------------------------------------ movement */

static void update_movement(float dt)
{
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

/* relative sprite index (0-5) within a 6-sprite actor set, + flip flag */
static int actor_sprite(int facing, bool moving, float walk_anim, bool *flip)
{
    int frame = ((int)(walk_anim / 8)) & 1;
    *flip = (facing == 2);
    int idx;
    switch (facing) {
    case 0:  idx = 0; break;     /* down */
    case 1:  idx = 2; break;     /* up   */
    default: idx = 4; break;     /* side */
    }
    if (frame && moving) idx++;
    return idx;
}

static void draw_entity_hpbar(float sx, float sy, int hp, int maxhp)
{
    int w = 16;
    int green = hp * w / maxhp;
    rdpq_set_mode_fill(RGBA32(200, 30, 30, 255));
    rdpq_fill_rectangle(sx - 8, sy - 26, sx - 8 + w, sy - 23);
    if (green > 0) {
        rdpq_set_mode_fill(RGBA32(40, 180, 40, 255));
        rdpq_fill_rectangle(sx - 8, sy - 26, sx - 8 + green, sy - 23);
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

    int tx0 = cam_x / TILE, ty0 = cam_y / TILE;
    int tx1 = (cam_x + SCREEN_W - 1) / TILE, ty1 = (cam_y + SCREEN_H - 1) / TILE;
    int water_frame = (frame_counter / 24) & 1;
    int anim_frame  = (frame_counter / 16) & 1;

    /* ---- terrain (opaque, copy mode) ---- */
    rdpq_set_mode_copy(false);
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
            default:
                s = ((x * 31 + y * 17) % 5 == 0) ? SPR_GRASS_B : SPR_GRASS_A;
            }
            rdpq_sprite_blit(spr[s], x * TILE - cam_x, y * TILE - cam_y, NULL);
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
            case OBJ_ALTAR_FIRE: rdpq_sprite_blit(spr[SPR_ALTAR_FIRE], sx, sy, NULL); break;
            }
        }

        /* goblins whose feet are in this row */
        for (int i = 0; i < num_gob; i++) {
            gob_t *g = &gob[i];
            if (!g->exists || g->dead) continue;
            if ((int)g->py / TILE != y) continue;
            rdpq_sprite_blit(spr[anim_frame ? SPR_GOB_B : SPR_GOB_A],
                             g->px - 8 - cam_x, g->py - 12 - cam_y, NULL);
        }

        /* bots whose feet are in this row */
        for (int i = 0; i < MAX_BOT; i++) {
            bot_t *b = &bots[i];
            if ((int)b->py / TILE != y) continue;
            bool flip;
            int idx = actor_sprite(b->facing, b->moving, b->walk_anim, &flip);
            rdpq_sprite_blit(spr[SPR_BOTA_DOWN_A + b->look * 6 + idx],
                             b->px - 8 - cam_x, b->py - 20 - cam_y,
                             &(rdpq_blitparms_t){ .flip_x = flip });
        }

        /* player */
        if (pl_row == y) {
            bool flip;
            int idx = actor_sprite(pl.facing, pl.moving, pl.walk_anim, &flip);
            rdpq_sprite_blit(spr[SPR_PL_DOWN_A + idx],
                             pl.px - 8 - cam_x, pl.py - 20 - cam_y,
                             &(rdpq_blitparms_t){ .flip_x = flip });
        }
    }

    /* ---- hp bars & hitsplats ---- */
    for (int i = 0; i < num_gob; i++) {
        gob_t *g = &gob[i];
        if (!g->exists || g->dead) continue;
        float sx = g->px - cam_x, sy = g->py - cam_y;
        if (g->hurt_timer > 0)
            draw_entity_hpbar(sx, sy, g->hp, GOB_MAX_HP);
        if (g->hitsplat_t > 0) {
            g->hitsplat_t--;
            draw_hitsplat(sx, sy, g->hitsplat);
        }
    }
    {
        float sx = pl.px - cam_x, sy = pl.py - cam_y;
        if (pl.hitsplat_t > 0) {
            pl.hitsplat_t--;
            draw_entity_hpbar(sx, sy - 8, pl.hp, level_of(SK_HP));
            draw_hitsplat(sx, sy, pl.hitsplat);
        }
    }

    /* ---- bot overhead chat ---- */
    for (int i = 0; i < MAX_BOT; i++) {
        bot_t *b = &bots[i];
        if (b->say_t <= 0) continue;
        b->say_t--;
        int w = strlen(b->say) * 6;
        draw_text(1, (int)(b->px - cam_x) - w / 2, (int)(b->py - cam_y) - 26,
                  "%s", b->say);
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

    /* ---- HUD ---- */
    rdpq_set_mode_fill(RGBA32(20, 20, 20, 255));
    rdpq_fill_rectangle(4, 4, 84, 28);
    int maxhp = level_of(SK_HP);
    draw_text(pl.hp * 3 <= maxhp ? 3 : 4, 8, 14, "HP  %d/%d", pl.hp, maxhp);
    draw_text(pl.run_on ? 1 : 6, 8, 24, "Run %d%% %s", pl.run_energy,
              pl.run_on ? "(on)" : "");

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
        draw_text(1, px0 + 6, py0 + 12, "Inventory");
        /* slots */
        for (int i = 0; i < INV_SIZE; i++) {
            int cx = px0 + 6 + (i % 4) * 19;
            int cy = py0 + 18 + (i / 4) * 18;
            rdpq_set_mode_fill(i == inv_cursor ? RGBA32(120, 100, 60, 255)
                                               : RGBA32(70, 60, 45, 255));
            rdpq_fill_rectangle(cx, cy, cx + 17, cy + 16);
        }
        rdpq_set_mode_copy(true);
        for (int i = 0; i < INV_SIZE; i++) {
            if (inv[i] == IT_NONE) continue;
            int cx = px0 + 6 + (i % 4) * 19;
            int cy = py0 + 18 + (i / 4) * 18;
            rdpq_sprite_blit(spr[iteminfo[inv[i]].spr], cx, cy, NULL);
        }
        draw_text(0, px0 + 6, py0 + 150, "%s",
                  inv[inv_cursor] != IT_NONE ? iteminfo[inv[inv_cursor]].name : "-");
    }
    else if (ui_mode == UI_SKILLS) {
        int px0 = SCREEN_W - 130, py0 = 24;
        draw_panel(px0, py0, px0 + 124, py0 + 152);
        draw_text(1, px0 + 6, py0 + 12, "Skills");
        for (int i = 0; i < NUM_SKILLS; i++)
            draw_text(0, px0 + 6, py0 + 24 + i * 10, "%-11s %2d",
                      skill_names[i], level_of(i));
        int att = level_of(SK_ATT), str = level_of(SK_STR), def = level_of(SK_DEF);
        int cmb = (int)((def + level_of(SK_HP) + level_of(SK_PRAY) / 2) * 0.25f
                        + (att + str) * 0.325f);
        draw_text(4, px0 + 6, py0 + 147, "Combat level: %d", cmb);
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
    else if (ui_mode == UI_HELP) {
        int px0 = 40, py0 = 26;
        draw_panel(px0, py0, px0 + 240, py0 + 150);
        draw_text(1, px0 + 8, py0 + 14, "RUNE VALLEY 64");
        draw_text(0, px0 + 8, py0 + 30, "Stick/D-Pad: walk   R: toggle run");
        draw_text(0, px0 + 8, py0 + 42, "A: interact with the world");
        draw_text(0, px0 + 8, py0 + 54, "B: inventory (A: use, C-down: drop)");
        draw_text(0, px0 + 8, py0 + 66, "C-right: skills    L: music on/off");
        draw_text(0, px0 + 8, py0 + 82, "Chop trees, mine rocks, catch shrimp,");
        draw_text(0, px0 + 8, py0 + 94, "light fires, cook food, slay goblins,");
        draw_text(0, px0 + 8, py0 + 106, "bury bones and bank your riches.");
        draw_text(6, px0 + 8, py0 + 126, "An original homage to old-school");
        draw_text(6, px0 + 8, py0 + 138, "adventures. Press Start to close.");
    }

    rdpq_detach_show();
}

/* ------------------------------------------------------------ input */

static void handle_input(void)
{
    joypad_poll();
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);

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
            }
        }
        return;
    }
    if (ui_mode == UI_BANK) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
        int n = bank_rows();
        if (pressed.d_up && bank_cursor > 0) bank_cursor--;
        if (pressed.d_down && bank_cursor < n - 1) bank_cursor++;
        if (pressed.a) bank_withdraw(bank_cursor);
        if (pressed.z) bank_deposit_all();
        return;
    }
    if (ui_mode == UI_SKILLS || ui_mode == UI_HELP) {
        if (pressed.b) { ui_mode = UI_NONE; return; }
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
    init_map();
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
    for (int i = 0; i < INV_SIZE; i++) inv[i] = IT_NONE;
    inv[0] = IT_AXE; inv[1] = IT_PICK; inv[2] = IT_NET; inv[3] = IT_TINDER;
    memset(bank, 0, sizeof bank);
    for (int i = 0; i < CHAT_LINES; i++) chat[i][0] = 0;

    msg("Welcome to Rune Valley 64.");
    msg("Press Start for help.");

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
        render();

        if (audio_can_write()) {
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }
    }
}
