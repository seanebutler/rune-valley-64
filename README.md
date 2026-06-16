# Rune Valley 64

An original Nintendo 64 homage to Old School RuneScape, built with
[libdragon](https://libdragon.dev/). Every line of code, pixel, sound effect and
note is original — no Jagex assets are used.

![Rune Valley 64 running in ares](screenshot.png)

## Overview

You wash up on the shore of **Oakhaven** with an axe, a pickaxe, a small net and
a tinderbox. The world runs on a **600 ms game tick**, just like the classic:
**16 skills**, all three combat styles, two regions, three bosses, a clutch of
quests, and a cartridge save that survives updates. Exact numbers for every
weapon, monster and recipe live in the in-game **Almanac** (and
[ALMANAC.md](ALMANAC.md)), so this README stays high-level.

## Skills

**Gathering**

- **Woodcutting** — fell trees, oaks (15) and Frostmere pines (35); stumps regrow.
- **Mining** — copper/tin, iron (15), coal (20), mithril (30), gold (40); rocks
  deplete and respawn.
- **Fishing** — the **tackle you carry** sets the catch at a spot: net → shrimp,
  rod → trout (20), lobster pot → lobster (40), harpoon → swordfish (50).

**Production**

- **Smithing** — smelt ore at the furnace (copper+tin → bronze; iron → iron;
  iron+coal → steel at 20; mithril ore + 2 coal → mithril at 30; gold at 40),
  then hammer bars into weapons, armour and arrowtips at the anvil.
- **Firemaking & Cooking** — light logs with a tinderbox, then cook your catch on
  the flame; the better the fish, the more it heals (3–16 HP).
- **Crafting** — have **Pelt the Tanner** cure cowhide/dragonhide into leather,
  then stitch **ranged armour** with a needle + thread; also mounts dragonstones
  on gold bars into **jewelry** (see *Equipment*).
- **Fletching** — use a knife on logs for arrow shafts and bows, and join shafts
  with smithed tips into arrows.
- **Runecraft** — mine ever-lasting rune essence in the north-west ruins and bind
  it at the altars: Air (1), Water (5), Earth (9), Fire (14), and — hidden in the
  dungeon — Chaos (20) and Law (25). Runes stack in one slot.

**Combat** — Attack, Strength, Defence, Hitpoints, Magic, Ranged, and Prayer.

## Combat

The classic **triangle** of melee, **Magic** and **Ranged**. Pick a spell or
draw a bow and **A** strikes the nearest foe from up to five tiles; otherwise you
swing in melee. All three share one accuracy curve, so none out-lands another at
equal investment — they differ in damage, speed and cost: melee hits hardest and
free, ranged matches it from safety but burns arrows, magic trades raw damage for
range and utility. Every exchange shows on screen — the attacker **lunges**, the
defender **recoils**.

- **Magic** — elemental strikes (Wind → Water → Earth → Fire) build to Earth and
  Fire **bolts** (each needs a Chaos rune). The spellbook also holds three
  **teleports** (Home, Bank, and the quest-locked Cave) and **Enchant Jewel**.
- **Ranged** — equip a bow and carry arrows; accuracy and max hit add the bow's,
  arrow's and ranged armour's bonuses. Mithril arrows need an oak shortbow+.
- **Prayer** — bury bones for xp (tougher foes drop **big bones** and the
  dragon **dragon bones**, worth far more), then toggle boosts across Attack/
  Strength/Defence/Magic (+10%, or +20% at higher levels). Active prayers drain
  points; recharge at any altar (carry no essence).
- **Potions** — buy brews at the General Store and drink them for a timed boost:
  Attack, Strength or Defence singly, a **Combat** potion that lifts all three,
  or a **Prayer** potion that restores prayer points. A renewable way to spend
  your coin hoard on the harder fights.
- **Upgrades** — open the **Start** menu and page through to **Upgrades** (Z to
  buy): spend coins on extra inventory slots and a **run boost** (faster, fuller
  run energy and a little more speed). Rising one-time sinks for a swollen purse.

## Equipment

Six worn slots — **weapon, shield, helm, body, neck, hand** — each feeding
Attack/Strength/Defence/Magic/Ranged straight into the formulas. Worn gear is
drawn on your character.

- **Gear tiers** — bronze, iron, steel and mithril are smithed from your own
  bars. **Rune** can't be bought — it drops only from the **Demon, Dragon and
  Yeti** (needs level 40 to wield), so the top tier stays rare. Carried iron
  tools speed up gathering.
- **Special weapons** — the Warlord's **Bane**, the dragon's hybrid **Dragonfire
  blade** (top melee *and* Magic), and the Yeti's two-handed **Frostmaul**.
- **Ranged armour** — leather (novice) and dragonhide (high Crafting) coif + body.
- **Jewelry** — craft a ring/bracelet/amulet/necklace from a dragonstone + gold
  bar, then **Enchant Jewel** (Magic 30) makes it combat gear: Amulet of Glory
  (Atk/Str/Def +8), Necklace of Power (Mag +10, Rng +8), Ring of Fury (Str +8),
  Bracelet of Guard (Atk/Def +6).

## The world

- **Oakhaven** — the starting region: the mine and furnace/anvil, the **Bazaar**
  (General Store, Weapon Shop, Armoury, Magic Shop), a **bank**, the cow pasture,
  and the rune ruins. Buy with coins from drops, or sell loot (**Z**) for half.
- **The dungeon** (three floors, SW of the mine) — skeletons and the **Goblin
  Warlord**, then wights and the **Demon**, then the **Ancient Dragon**, which
  breathes fire across the room, enrages at half health, and summons whelps. It
  always drops a fat coin hoard and **dragon bones**, often a Dragonstone, and
  rarely the **Dragonfire blade**.
- **Frostmere** — sail the **boat** from Oakhaven's beach to a frozen isle of
  pine groves and a frozen lake: frost wolves, ice warriors, and the **Yeti**
  mini-boss. Its own bank and boat home sit at the landing.
- Banking offers deposit-all and per-item withdrawal; fall in battle and you wake
  safely on the surface.

## Quests & goals

- **Quests** (tracked in the Journal): *Basic Training*, *The Chef's Little
  Problem*, the multi-stage *The Warlord's Bane*, and the follow-up *The Demon
  Below*.
- **Achievement diaries** — a per-area checklist (Oakhaven and Frostmere). Tasks
  tick off the moment you earn them and pay coins, with a bounty for clearing an
  area.
- **The Almanac** — an in-game reference for every weapon, monster and loot table
  (**Start → Journal → Diary → Almanac**), mirrored in [ALMANAC.md](ALMANAC.md).

## Details

- Levelling uses the **real OSRS XP table** (99 = 13,034,431 xp); each level-up
  plays an original fanfare.
- Five **player bots** roam the valley — gathering, chatting, banking, lighting
  campfires you can cook on, and saying "gz" when you level up nearby.
- Saves to **cartridge EEPROM** (silent autosave each minute, plus on bank visits
  and death). The format is forward-compatible — updates slot into reserved space
  instead of wiping progress — and remembers which region you logged out in.

## Controls

| Input | Action |
|---|---|
| Stick / D-Pad | Walk |
| R | Toggle run |
| A | Interact: chop / mine / fish / cook / bank / sail / attack |
| B | Inventory (A: use/eat/bury/light/equip, C-down: drop) |
| C-right | Skills panel |
| C-up | Worn equipment (A: unequip) |
| C-left | Spellbook (A: cast spell / teleport / enchant) |
| C-down | Prayer book (A: toggle prayer) |
| Start | Help → Quest Journal → Diary → Almanac |
| L | Music on/off |

## Building

Prebuilt toolchain + MSYS2 live in `tools/` (not checked in). To reproduce:

1. Install the [libdragon prebuilt MIPS64 toolchain](https://github.com/DragonMinded/libdragon/releases/tag/toolchain-continuous-prerelease)
   and set `N64_INST` to its root.
2. Build & install libdragon (`make install tools-install` in the libdragon
   repo, under an MSYS2/MinGW64 or Linux shell).
3. Generate assets (Windows PowerShell): `assets/genart.ps1` (pixel art) and
   `assets/gensfx.ps1` (audio synth).
4. `make` — produces `runevalley.z64`.

The ROM runs in [ares](https://ares-emu.net/), simple64, or on real hardware via
an EverDrive/SC64 flashcart.

## Layout

```
src/main.c              the whole game: map, ticks, skills, combat, UI, renderer
assets/genart.ps1       ASCII-grid -> PNG pixel art generator (System.Drawing)
assets/gensfx.ps1       SFX + 16-bar music loop synthesizer (embedded C#)
assets/gen_almanac.ps1  regenerates ALMANAC.md from the data tables in main.c
Makefile                libdragon n64.mk build: PNG -> sprite, WAV -> wav64, DFS, Z64
```
