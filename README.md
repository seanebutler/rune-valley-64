# Rune Valley 64

An original Nintendo 64 homage to Old School RuneScape, built with
[libdragon](https://libdragon.dev/). All code, pixel art, sound effects and
music are original — no Jagex assets are used.

![Rune Valley 64 running in ares](screenshot.png)

## The game

You wash up in Rune Valley with an axe, a pickaxe, a small net and a
tinderbox. The world runs on a **600 ms game tick**, just like the classic:

- **Woodcutting** — chop trees (and oaks at level 15); trees fall to stumps
  and respawn
- **Mining** — copper and tin near the west path, iron (level 15) further out;
  rocks deplete and respawn
- **Fishing** — net shrimp at the rippling spots along the river
- **Firemaking** — use logs from the inventory with a tinderbox in your pack;
  your character steps politely to the west as the fire catches
- **Cooking** — cook raw shrimp on a fire; burn rates drop as you level
  (never burn again at 34)
- **Combat** — level-2 goblins roam east of the bridge; they are aggressive,
  they hit 1s, and they drop bones
- **Prayer** — bury those bones
- **Runecraft** — mine rune essence from the glittering rock in the northwest
  ruins (it never depletes) and bind it at the Air altar beside it, or brave
  the goblin camp to reach the Fire altar (level 14). Runes **stack** in a
  single inventory slot, with the count drawn over the icon
- **Magic** — open the spellbook (C-left) and pick Wind Strike (1 air rune) or
  Fire Strike (1 fire rune, Magic 13). With a spell chosen, A flings a bolt at
  the nearest goblin from up to five tiles away, spending a rune and earning
  Magic xp. You start with 25 air runes to get going; craft more at the altar
- **Smithing** — smelt copper + tin into bronze bars at the furnace by the
  mine (iron smelts at 15, and fails half the time, as is tradition), then
  hammer out swords, helms, shields, and platebodies at the anvil. Iron tools
  speed up gathering when carried
- **Equipment** — open the worn-equipment screen (C-up), or press A on gear in
  your inventory to equip it. Four slots — weapon, shield, helm, body — each
  with Attack/Strength/Defence bonuses that feed straight into the combat
  formulas. Bronze gear wears at level 1, iron at Defence 10. You start with a
  bronze sword in your pack to wield. **Worn gear is drawn on your character** —
  helm, platebody, sword and shield all show on the sprite in every facing
- **A quest!** — *The Chef's Little Problem*: Chef Bouillon by the bank path
  needs 3 goblins bashed and 2 cooked shrimp delivered. Chunky xp rewards,
  as is right and proper
- **A dungeon** — find the cave stairs in the open field southwest of the
  mine and descend into the gloom. Tougher **skeletons** roam the hall, and
  the hulking **Goblin Warlord** (45 HP, hits hard) waits in the chamber
  beyond the doorway. Slaying him pours out combat xp. Come armed, armoured,
  and fed — and if you fall, you wake safely back on the surface
- **Banking** — a bank with deposit-all and per-item withdrawal sits at the
  north end of the path
- **Hitpoints** start at level 10, regenerate 1 per minute, and shrimp heal 3
- The **XP table is the real one** (level 99 = 13,034,431 xp — good luck on a
  cartridge)
- Run energy drains while running and recharges while you walk or stand

Level-ups play an original fanfare and print the classic congratulations to
the parchment chatbox.

The valley is populated by five **player bots** — fellow adventurers with
era-appropriate names who wander between trees, rocks and fishing spots,
deplete resources you were walking toward, advertise dubious services in
public chat, and say "gz" when you level up near them. They also make bank
runs through the front door and light campfires you can cook on.

The game opens on a **title screen** (the valley lives behind it) and saves
to **cartridge EEPROM**: silent autosave every minute, plus saves on bank
visits and death. The title offers Continue when a valid save exists.

## Controls

| Input | Action |
|---|---|
| Stick / D-Pad | Walk |
| R | Toggle run |
| A | Interact: chop / mine / fish / cook / bank / attack |
| B | Inventory (A: use/eat/bury/light/equip, C-down: drop) |
| C-right | Skills panel |
| C-up | Worn equipment (A: unequip) |
| C-left | Spellbook (A: choose spell / melee) |
| Start | Help |
| L | Music on/off |

## Building

Prebuilt toolchain + MSYS2 live in `tools/` (not checked in). To reproduce:

1. Install the [libdragon prebuilt MIPS64 toolchain](https://github.com/DragonMinded/libdragon/releases/tag/toolchain-continuous-prerelease)
   and set `N64_INST` to its root.
2. Build & install libdragon (`make install tools-install` in the libdragon
   repo, under an MSYS2/MinGW64 or Linux shell).
3. Generate assets (Windows PowerShell):
   `assets/genart.ps1` (pixel art) and `assets/gensfx.ps1` (audio synth).
4. `make` — produces `runevalley.z64`.

The ROM runs in [ares](https://ares-emu.net/), simple64, or on real hardware
via an EverDrive/SC64 flashcart.

## Layout

```
src/main.c        the whole game: map, ticks, skills, combat, UI, renderer
assets/genart.ps1 ASCII-grid -> PNG pixel art generator (System.Drawing)
assets/gensfx.ps1 SFX + 16-bar music loop synthesizer (embedded C#)
Makefile          libdragon n64.mk build: PNG->sprite, WAV->wav64, DFS, Z64
```
