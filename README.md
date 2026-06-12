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
  the goblin camp to reach the Fire altar (level 14)
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
public chat, and say "gz" when you level up near them.

## Controls

| Input | Action |
|---|---|
| Stick / D-Pad | Walk |
| R | Toggle run |
| A | Interact: chop / mine / fish / cook / bank / attack |
| B | Inventory (A: use/eat/bury/light, C-down: drop) |
| C-right | Skills panel |
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
