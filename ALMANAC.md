# Rune Valley 64 — Almanac

A quick-reference for all equipment stats and monster stats/loot. This mirrors
the **in-game Almanac** (open the help screen with **Start**, then press **A**
twice), which is generated live from the game's own data.

> Maintainer note: the in-game Almanac is built from `iteminfo`, `mobinfo` and
> the `*_drops` tables in `src/main.c`. If you change gear, monsters or loot
> there, update this file to match.

---

## Weapons

Worn in the weapon slot. The Magic bonus raises spell accuracy and max hit, so
the Dragonfire blade is best-in-slot for *both* melee and casting.

| Weapon | Attack | Strength | Magic | Defence | Wield level |
|---|--:|--:|--:|--:|--:|
| Bronze sword | +4 | +3 | +0 | +0 | 1 |
| Iron sword | +7 | +6 | +0 | +0 | 1 |
| Wizard staff | +1 | +2 | +12 | +0 | 1 |
| Steel sword | +10 | +9 | +0 | +0 | 20 |
| Warlord's Bane | +18 | +17 | +0 | +0 | 20 |
| Mithril sword | +14 | +13 | +0 | +0 | 30 |
| Rune sword | +20 | +18 | +0 | +0 | 40 |
| **Dragonfire blade** | **+22** | **+21** | **+18** | **+2** | 40 |

## Armour

Worn in the helm, shield and body slots. Shields carry a small Attack bonus;
wizard gear carries a Magic bonus.

| Item | Slot | Attack | Defence | Magic | Wield level |
|---|---|--:|--:|--:|--:|
| Bronze helm | Helm | +0 | +3 | +0 | 1 |
| Wizard hat | Helm | +0 | +1 | +3 | 1 |
| Iron helm | Helm | +0 | +5 | +0 | 10 |
| Steel helm | Helm | +0 | +7 | +0 | 20 |
| Mithril helm | Helm | +0 | +10 | +0 | 30 |
| Rune helm | Helm | +0 | +14 | +0 | 40 |
| Bronze shield | Shield | +1 | +5 | +0 | 1 |
| Iron shield | Shield | +2 | +8 | +0 | 10 |
| Steel shield | Shield | +2 | +11 | +0 | 20 |
| Mithril shield | Shield | +3 | +15 | +0 | 30 |
| Rune shield | Shield | +4 | +20 | +0 | 40 |
| Bronze body | Body | +0 | +8 | +0 | 1 |
| Wizard robe | Body | +0 | +2 | +5 | 1 |
| Iron body | Body | +0 | +14 | +0 | 10 |
| Steel body | Body | +0 | +20 | +0 | 20 |
| Mithril body | Body | +0 | +28 | +0 | 30 |
| Rune body | Body | +0 | +40 | +0 | 40 |

---

## Monsters & loot

Every monster always drops **bones**. The percentages below are a single
weighted roll for the rest of the drop (they sum to 100% per monster).

### Cow — overworld pasture
Hitpoints **8** · max hit **1** · defence **0**. Placid; never aggressive.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 6 | 42% |
| Raw shrimp | 1 | 12% |
| Nothing | — | 46% |

### Goblin — east of the bridge
Hitpoints **5** · max hit **1** · defence **1**. Aggressive.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 12 | 28% |
| Air rune | 3 | 24% |
| Raw shrimp | 1 | 16% |
| Copper ore | 1 | 12% |
| Nothing | — | 20% |

### Skeleton — dungeon floor 1
Hitpoints **9** · max hit **2** · defence **6**. Aggressive.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 30 | 25% |
| Fire rune | 2 | 20% |
| Air rune | 4 | 15% |
| Iron ore | 1 | 13% |
| Shrimp | 1 | 12% |
| Wizard hat | 1 | 5% |
| Nothing | — | 10% |

### Goblin Warlord — boss, dungeon floor 1
Hitpoints **45** · max hit **5** · defence **15**. Always pays out (no "nothing").

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 350 | 20% |
| Wizard staff | 1 | 14% |
| Steel body | 1 | 14% |
| Wizard robe | 1 | 12% |
| Mithril helm | 1 | 12% |
| Wizard hat | 1 | 10% |
| Fire rune | 15 | 10% |
| Mithril sword | 1 | 8% |

### Wight — dungeon floor 2
Hitpoints **14** · max hit **3** · defence **10**. Aggressive.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 60 | 26% |
| Fire rune | 5 | 18% |
| Iron bar | 1 | 14% |
| Shrimp | 2 | 14% |
| Mithril helm | 1 | 4% |
| Nothing | — | 24% |

### Demon — boss, dungeon floor 2
Hitpoints **80** · max hit **8** · defence **25**. Always pays out.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 800 | 22% |
| Mithril body | 1 | 16% |
| Rune sword | 1 | 14% |
| Rune helm | 1 | 14% |
| Fire rune | 30 | 14% |
| Rune shield | 1 | 12% |
| Rune body | 1 | 8% |

### Whelp — dungeon floor 3 (the Dragon's brood)
Hitpoints **18** · max hit **4** · defence **12**. Aggressive.

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 90 | 26% |
| Fire rune | 6 | 20% |
| Shrimp | 2 | 16% |
| Mithril helm | 1 | 4% |
| Nothing | — | 34% |

### Ancient Dragon — boss, dungeon floor 3
Hitpoints **150** · melee max hit **12** · defence **35**.

**Special mechanics:**
- **Fire breath** — a ranged attack (up to 5 tiles, ignores armour) for **5**
  damage, rising to **8** when enraged. Bring food.
- **Enrage** — at half health it attacks and breathes faster and hits harder.
- **Brood summon** — on enraging it summons **2 whelps** (they vanish and drop
  nothing on death).

**Always drops 2500 coins + a Dragonstone**, plus one weighted roll below:

| Drop | Qty | Chance |
|---|--:|--:|
| Coins | 1500 | 18% |
| Dragonstone | 1 | 16% |
| Rune body | 1 | 13% |
| Rune sword | 1 | 12% |
| Fire rune | 60 | 12% |
| Rune shield | 1 | 11% |
| Rune helm | 1 | 11% |
| **Dragonfire blade** (unique) | 1 | 7% |

---

## Notes

- **Dragonstone** is a trophy gem worth ~1000 coins at any shop's Sell tab.
- The **Dragonfire blade** is unique and cannot be sold — the valley's finest
  weapon and the only hybrid melee/magic arm.
- The XP table is the real Old School one (level 99 = 13,034,431 xp).
