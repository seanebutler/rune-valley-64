# Rune Valley 64 - Almanac

A quick-reference for all equipment stats and monster stats/loot. This mirrors
the **in-game Almanac** (open the help screen with **Start**, then press **A**
twice).

> Auto-generated from `src/main.c` by `assets/gen_almanac.ps1` - do not edit by
> hand; re-run the script after changing gear, monsters or loot.

---

## Weapons

Worn in the weapon slot. The Magic bonus raises spell accuracy and max hit.

| | Weapon | Attack | Strength | Magic | Defence | Wield level |
|:-:|---|--:|--:|--:|--:|--:|
| ![](docs/almanac/item_staff.png) | Wizard staff | +1 | +2 | +12 | +0 | 1 |
| ![](docs/almanac/item_bronze_sword.png) | Bronze sword | +4 | +3 | +0 | +0 | 1 |
| ![](docs/almanac/item_iron_sword.png) | Iron sword | +7 | +6 | +0 | +0 | 1 |
| ![](docs/almanac/item_st_sword.png) | Steel sword | +10 | +9 | +0 | +0 | 20 |
| ![](docs/almanac/item_bane.png) | Warlord's Bane | +18 | +17 | +0 | +0 | 20 |
| ![](docs/almanac/item_mi_sword.png) | Mithril sword | +14 | +13 | +0 | +0 | 30 |
| ![](docs/almanac/item_ru_sword.png) | Rune sword | +20 | +18 | +0 | +0 | 40 |
| ![](docs/almanac/item_dragonfire.png) | Dragonfire blade | +22 | +21 | +18 | +2 | 40 |

## Armour

Shields carry a small Attack bonus; wizard gear carries a Magic bonus.

| | Item | Slot | Attack | Defence | Magic | Wield level |
|:-:|---|---|--:|--:|--:|--:|
| ![](docs/almanac/item_bronze_helm.png) | Bronze helm | Helm | +0 | +3 | +0 | 1 |
| ![](docs/almanac/item_wizard_hat.png) | Wizard hat | Helm | +0 | +1 | +3 | 1 |
| ![](docs/almanac/item_iron_helm.png) | Iron helm | Helm | +0 | +5 | +0 | 10 |
| ![](docs/almanac/item_st_helm.png) | Steel helm | Helm | +0 | +7 | +0 | 20 |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | Helm | +0 | +10 | +0 | 30 |
| ![](docs/almanac/item_ru_helm.png) | Rune helm | Helm | +0 | +14 | +0 | 40 |
| ![](docs/almanac/item_bronze_shield.png) | Bronze shield | Shield | +1 | +5 | +0 | 1 |
| ![](docs/almanac/item_iron_shield.png) | Iron shield | Shield | +2 | +8 | +0 | 10 |
| ![](docs/almanac/item_st_shield.png) | Steel shield | Shield | +2 | +11 | +0 | 20 |
| ![](docs/almanac/item_mi_shield.png) | Mithril shield | Shield | +3 | +15 | +0 | 30 |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | Shield | +4 | +20 | +0 | 40 |
| ![](docs/almanac/item_bronze_body.png) | Bronze body | Body | +0 | +8 | +0 | 1 |
| ![](docs/almanac/item_wizard_robe.png) | Wizard robe | Body | +0 | +2 | +5 | 1 |
| ![](docs/almanac/item_iron_body.png) | Iron body | Body | +0 | +14 | +0 | 10 |
| ![](docs/almanac/item_st_body.png) | Steel body | Body | +0 | +20 | +0 | 20 |
| ![](docs/almanac/item_mi_body.png) | Mithril body | Body | +0 | +28 | +0 | 30 |
| ![](docs/almanac/item_ru_body.png) | Rune body | Body | +0 | +40 | +0 | 40 |

---

## Monsters & loot

Every monster always drops **bones**. The percentages are a single weighted
roll for the rest of the drop.

### Cow - overworld pasture
![](docs/almanac/mob_cow_a.png)

Hitpoints **8** | max hit **1** | defence **0**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_cowhide.png) | Cowhide | 1 | 55% |
| ![](docs/almanac/item_coins.png) | Coins | 6 | 25% |
| ![](docs/almanac/item_raw_shrimp.png) | Raw shrimp | 1 | 8% |
|  | Nothing | - | 12% |

### Goblin - east of the bridge
![](docs/almanac/goblin_a.png)

Hitpoints **5** | max hit **1** | defence **1**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 12 | 28% |
| ![](docs/almanac/item_air_rune.png) | Air rune | 3 | 24% |
| ![](docs/almanac/item_raw_shrimp.png) | Raw shrimp | 1 | 16% |
| ![](docs/almanac/item_copper_ore.png) | Copper ore | 1 | 12% |
|  | Nothing | - | 20% |

### Skeleton - dungeon floor 1
![](docs/almanac/skeleton_a.png)

Hitpoints **9** | max hit **2** | defence **6**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 30 | 25% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 2 | 20% |
| ![](docs/almanac/item_air_rune.png) | Air rune | 4 | 15% |
| ![](docs/almanac/item_iron_ore.png) | Iron ore | 1 | 13% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 1 | 12% |
| ![](docs/almanac/item_wizard_hat.png) | Wizard hat | 1 | 5% |
|  | Nothing | - | 10% |

### Goblin Warlord - boss, dungeon floor 1
![](docs/almanac/boss.png)

Hitpoints **45** | max hit **5** | defence **15**.

**Always pays out (no "nothing").**

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 350 | 20% |
| ![](docs/almanac/item_staff.png) | Wizard staff | 1 | 14% |
| ![](docs/almanac/item_wizard_robe.png) | Wizard robe | 1 | 12% |
| ![](docs/almanac/item_wizard_hat.png) | Wizard hat | 1 | 10% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 15 | 10% |
| ![](docs/almanac/item_st_body.png) | Steel body | 1 | 14% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 12% |
| ![](docs/almanac/item_mi_sword.png) | Mithril sword | 1 | 8% |

### Wight - dungeon floor 2
![](docs/almanac/wight_a.png)

Hitpoints **14** | max hit **3** | defence **10**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 60 | 26% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 5 | 18% |
| ![](docs/almanac/item_iron_bar.png) | Iron bar | 1 | 14% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 2 | 14% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 4% |
|  | Nothing | - | 24% |

### Demon - boss, dungeon floor 2
![](docs/almanac/demon.png)

Hitpoints **80** | max hit **8** | defence **25**.

**Always pays out (no "nothing").**

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 800 | 22% |
| ![](docs/almanac/item_ru_sword.png) | Rune sword | 1 | 14% |
| ![](docs/almanac/item_ru_helm.png) | Rune helm | 1 | 14% |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | 1 | 12% |
| ![](docs/almanac/item_ru_body.png) | Rune body | 1 | 8% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 30 | 14% |
| ![](docs/almanac/item_mi_body.png) | Mithril body | 1 | 16% |

### Whelp - dungeon floor 3 (the Dragon's brood)
![](docs/almanac/mob_whelp_a.png)

Hitpoints **18** | max hit **4** | defence **12**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 90 | 26% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 6 | 20% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 2 | 16% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 4% |
|  | Nothing | - | 34% |

### Ancient Dragon - boss, dungeon floor 3
![](docs/almanac/mob_dragon.png)

Hitpoints **150** | max hit **12** | defence **35**.

**Always drops 2500 coins + a Dragonstone.**

- Fire breath: a ranged attack (up to 5 tiles, ignores armour) for 5 damage, rising to 8 when enraged.
- Enrages at half health: attacks and breathes faster and hits harder.
- On enraging it summons 2 whelps (they vanish and drop nothing on death).

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 1500 | 16% |
| ![](docs/almanac/item_ru_body.png) | Rune body | 1 | 12% |
| ![](docs/almanac/item_ru_sword.png) | Rune sword | 1 | 11% |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | 1 | 10% |
| ![](docs/almanac/item_ru_helm.png) | Rune helm | 1 | 10% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 60 | 11% |
| ![](docs/almanac/item_dragon_hide.png) | Dragonhide | 2 | 13% |
| ![](docs/almanac/item_dragonstone.png) | Dragonstone | 1 | 10% |
| ![](docs/almanac/item_dragonfire.png) | Dragonfire blade | 1 | 7% |

---

## Spells

Cast from the spellbook (C-left). Bolts need a Chaos rune on top of their
element; teleports are powered by Law runes (Home uses an Air rune).

| Spell | Magic level | Runes | Max hit |
|---|--:|---|--:|
| Wind Strike | 1 | 1x Air rune | 2 |
| Water Strike | 5 | 1x Water rune | 3 |
| Earth Strike | 9 | 1x Earth rune | 4 |
| Fire Strike | 13 | 1x Fire rune | 5 |
| Earth Bolt | 29 | 3x Earth rune + 1x Chaos rune | 7 |
| Fire Bolt | 35 | 3x Fire rune + 1x Chaos rune | 8 |
| Home Teleport | 1 | 1x Air rune | teleport |
| Bank Teleport | 20 | 1x Law rune | teleport |
| Cave Teleport | 25 | 1x Law rune | teleport |

---

## Ranged

Equip a bow in the weapon slot and carry arrows, then fire at a goblin from up
to five tiles away. Each shot spends one arrow and trains Ranged; accuracy and
max hit add the bow's, the arrow's and your ranged armour's Ranged bonuses.
Bows and arrows are fletched (use a knife on logs). **Mithril arrows need an
oak shortbow or better** - a plain shortbow can't draw them.

| | Bow | Ranged | Wield level |
|:-:|---|--:|--:|
| ![](docs/almanac/item_shortbow.png) | Shortbow | +8 | 1 |
| ![](docs/almanac/item_oak_bow.png) | Oak shortbow | +14 | 20 |

| | Arrow | Ranged |
|:-:|---|--:|
| ![](docs/almanac/item_bronze_arrow.png) | Bronze arrow | +3 |
| ![](docs/almanac/item_iron_arrow.png) | Iron arrow | +7 |
| ![](docs/almanac/item_mith_arrow.png) | Mithril arrow | +12 |

Ranged armour is crafted (see below) and worn for a Ranged bonus; it needs the
listed Ranged level to wear.

| | Ranged armour | Slot | Ranged | Defence | Ranged level |
|:-:|---|---|--:|--:|--:|
| ![](docs/almanac/item_leather_body.png) | Leather body | Body | +4 | +4 | 1 |
| ![](docs/almanac/item_leather_coif.png) | Leather coif | Helm | +2 | +2 | 1 |
| ![](docs/almanac/item_dhide_coif.png) | D'hide coif | Helm | +5 | +5 | 20 |
| ![](docs/almanac/item_dhide_body.png) | D'hide body | Body | +8 | +8 | 30 |

---

## Crafting

Slay cows for **cowhide** (and the Ancient Dragon for **dragonhide**), then
have **Pelt the Tanner** by the cow pasture cure them into leather for a few
coins. Use a **needle** on the leather (with **thread** in your pack) to stitch
ranged armour - both sold at the General Store. Dragonhide gear demands a high
Crafting level but gives the best Ranged bonuses in the valley.

| | Stitch | Materials | Crafting level |
|:-:|---|---|--:|
| ![](docs/almanac/item_leather_coif.png) | Leather coif | 1x Leather + 1x Thread | 5 |
| ![](docs/almanac/item_leather_body.png) | Leather body | 3x Leather + 1x Thread | 14 |
| ![](docs/almanac/item_dhide_coif.png) | D'hide coif | 1x Dragon leather + 1x Thread | 35 |
| ![](docs/almanac/item_dhide_body.png) | D'hide body | 2x Dragon leather + 1x Thread | 40 |

---

## Fishing & food

The tackle you carry decides the catch at a fishing spot. Cook the catch on
a fire (it burns until your Cooking level reaches the listed level); eating
the cooked fish restores the listed Hitpoints.

| | Fish | Fishing level | Tackle | Cooking level | Heals |
|:-:|---|--:|---|--:|--:|
| ![](docs/almanac/item_shrimp.png) | Shrimp | 1 | Small net | 34 | 3 |
| ![](docs/almanac/item_trout.png) | Trout | 20 | Fishing rod | 40 | 7 |
| ![](docs/almanac/item_lobster.png) | Lobster | 40 | Lobster pot | 55 | 12 |
| ![](docs/almanac/item_swordfish.png) | Swordfish | 50 | Harpoon | 70 | 16 |

---

## Notes

- **Dragonstone** is a trophy gem worth ~1000 coins at any shop's Sell tab.
- The **Dragonfire blade** is unique and cannot be sold.
- The XP table is the real Old School one (level 99 = 13,034,431 xp).
