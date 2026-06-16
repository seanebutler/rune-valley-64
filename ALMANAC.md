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
| ![](docs/almanac/item_frostmaul.png) | Frostmaul | +20 | +24 | +0 | +0 | 45 |

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
| ![](docs/almanac/item_coins.png) | Coins | 3-12 | 25% |
| ![](docs/almanac/item_raw_shrimp.png) | Raw shrimp | 1 | 8% |
|  | Nothing | - | 12% |

### Goblin - east of the bridge
![](docs/almanac/goblin_a.png)

Hitpoints **5** | max hit **1** | defence **1**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 5-25 | 28% |
| ![](docs/almanac/item_air_rune.png) | Air rune | 2-6 | 22% |
| ![](docs/almanac/item_raw_shrimp.png) | Raw shrimp | 1 | 14% |
| ![](docs/almanac/item_copper_ore.png) | Copper ore | 1-2 | 10% |
| ![](docs/almanac/item_tin_ore.png) | Tin ore | 1-2 | 8% |
|  | Nothing | - | 18% |

### Skeleton - dungeon floor 1
![](docs/almanac/skeleton_a.png)

Hitpoints **9** | max hit **2** | defence **6**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 20-60 | 22% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 2-5 | 18% |
| ![](docs/almanac/item_air_rune.png) | Air rune | 3-8 | 12% |
| ![](docs/almanac/item_iron_ore.png) | Iron ore | 1-2 | 11% |
| ![](docs/almanac/item_coal.png) | Coal | 1-2 | 12% |
| ![](docs/almanac/item_mith_arrow.png) | Mithril arrow | 5-12 | 8% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 1 | 8% |
| ![](docs/almanac/item_wizard_hat.png) | Wizard hat | 1 | 4% |
|  | Nothing | - | 5% |

### Goblin Warlord - boss, dungeon floor 1
![](docs/almanac/boss.png)

Hitpoints **45** | max hit **5** | defence **15**.

**Always pays out (no "nothing").**

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 250-500 | 18% |
| ![](docs/almanac/item_staff.png) | Wizard staff | 1 | 12% |
| ![](docs/almanac/item_wizard_robe.png) | Wizard robe | 1 | 10% |
| ![](docs/almanac/item_wizard_hat.png) | Wizard hat | 1 | 8% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 10-25 | 9% |
| ![](docs/almanac/item_st_body.png) | Steel body | 1 | 11% |
| ![](docs/almanac/item_steel_bar.png) | Steel bar | 2-4 | 10% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 10% |
| ![](docs/almanac/item_mi_sword.png) | Mithril sword | 1 | 6% |
| ![](docs/almanac/item_gold_ore.png) | Gold ore | 2-4 | 6% |

### Wight - dungeon floor 2
![](docs/almanac/wight_a.png)

Hitpoints **14** | max hit **3** | defence **10**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 40-90 | 24% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 4-8 | 16% |
| ![](docs/almanac/item_steel_bar.png) | Steel bar | 1 | 12% |
| ![](docs/almanac/item_coal.png) | Coal | 1-3 | 12% |
| ![](docs/almanac/item_mith_ore.png) | Mithril ore | 1-2 | 10% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 1-2 | 10% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 4% |
|  | Nothing | - | 12% |

### Demon - boss, dungeon floor 2
![](docs/almanac/demon.png)

Hitpoints **80** | max hit **8** | defence **25**.

**Always pays out (no "nothing").**

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 600-1000 | 20% |
| ![](docs/almanac/item_ru_sword.png) | Rune sword | 1 | 13% |
| ![](docs/almanac/item_ru_helm.png) | Rune helm | 1 | 13% |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | 1 | 11% |
| ![](docs/almanac/item_ru_body.png) | Rune body | 1 | 7% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 25-40 | 12% |
| ![](docs/almanac/item_mi_body.png) | Mithril body | 1 | 14% |
| ![](docs/almanac/item_gold_bar.png) | Gold bar | 1-3 | 10% |

### Whelp - dungeon floor 3 (the Dragon's brood)
![](docs/almanac/mob_whelp_a.png)

Hitpoints **18** | max hit **4** | defence **12**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 60-120 | 26% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 5-10 | 20% |
| ![](docs/almanac/item_shrimp.png) | Shrimp | 2-3 | 16% |
| ![](docs/almanac/item_coal.png) | Coal | 1-3 | 12% |
| ![](docs/almanac/item_mith_ore.png) | Mithril ore | 1-2 | 8% |
| ![](docs/almanac/item_mi_helm.png) | Mithril helm | 1 | 4% |
|  | Nothing | - | 14% |

### Ancient Dragon - boss, dungeon floor 3
![](docs/almanac/mob_dragon.png)

Hitpoints **150** | max hit **12** | defence **35**.

**Always drops a 2000-3000 coin hoard (Dragonstone no longer guaranteed).**

- Fire breath: a ranged attack (up to 5 tiles, ignores armour) for 5 damage, rising to 8 when enraged.
- Enrages at half health: attacks and breathes faster and hits harder.
- On enraging it summons 2 whelps (they vanish and drop nothing on death).

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 800-2000 | 14% |
| ![](docs/almanac/item_ru_body.png) | Rune body | 1 | 11% |
| ![](docs/almanac/item_ru_sword.png) | Rune sword | 1 | 10% |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | 1 | 8% |
| ![](docs/almanac/item_ru_helm.png) | Rune helm | 1 | 8% |
| ![](docs/almanac/item_fire_rune.png) | Fire rune | 40-80 | 9% |
| ![](docs/almanac/item_dragon_hide.png) | Dragonhide | 1-3 | 12% |
| ![](docs/almanac/item_dragon_leather.png) | Dragon leather | 1-2 | 4% |
| ![](docs/almanac/item_gold_bar.png) | Gold bar | 2-5 | 6% |
| ![](docs/almanac/item_dragonstone.png) | Dragonstone | 1-2 | 11% |
| ![](docs/almanac/item_glory_amulet.png) | Amulet of Glory | 1 | 3% |
| ![](docs/almanac/item_dragonfire.png) | Dragonfire blade | 1 | 5% |

### Frost wolf - Frostmere snowfield
![](docs/almanac/mob_wolf_a.png)

Hitpoints **16** | max hit **4** | defence **10**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 30-60 | 30% |
| ![](docs/almanac/item_raw_trout.png) | Raw trout | 1-2 | 16% |
| ![](docs/almanac/item_coal.png) | Coal | 1-2 | 14% |
| ![](docs/almanac/item_leather.png) | Leather | 1 | 8% |
| ![](docs/almanac/item_pine_logs.png) | Pine logs | 1-3 | 10% |
|  | Nothing | - | 22% |

### Ice warrior - Frostmere, the north
![](docs/almanac/mob_icew_a.png)

Hitpoints **30** | max hit **6** | defence **20**.

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 120-220 | 24% |
| ![](docs/almanac/item_steel_bar.png) | Steel bar | 1-2 | 16% |
| ![](docs/almanac/item_law_rune.png) | Law rune | 2-5 | 12% |
| ![](docs/almanac/item_mith_ore.png) | Mithril ore | 1-3 | 12% |
| ![](docs/almanac/item_gold_ore.png) | Gold ore | 1-2 | 10% |
| ![](docs/almanac/item_mith_bar.png) | Mithril bar | 1 | 7% |
| ![](docs/almanac/item_mith_arrow.png) | Mithril arrow | 8-15 | 8% |
|  | Nothing | - | 11% |

### Yeti - boss, Frostmere
![](docs/almanac/mob_yeti.png)

Hitpoints **120** | max hit **11** | defence **32**.

**Always pays out (no "nothing").**

| | Drop | Qty | Chance |
|:-:|---|--:|--:|
| ![](docs/almanac/item_coins.png) | Coins | 800-1500 | 21% |
| ![](docs/almanac/item_ru_body.png) | Rune body | 1 | 11% |
| ![](docs/almanac/item_ru_shield.png) | Rune shield | 1 | 11% |
| ![](docs/almanac/item_gold_ore.png) | Gold ore | 3-6 | 13% |
| ![](docs/almanac/item_gold_bar.png) | Gold bar | 1-3 | 10% |
| ![](docs/almanac/item_dragonstone.png) | Dragonstone | 1-2 | 10% |
| ![](docs/almanac/item_frostmaul.png) | Frostmaul | 1 | 11% |
| ![](docs/almanac/item_fury_ring.png) | Ring of Fury | 1 | 3% |
| ![](docs/almanac/item_pine_logs.png) | Pine logs | 4-10 | 10% |

---

## Spells

Cast from the spellbook (C-left). Bolts need a Chaos rune on top of their
element; each teleport takes its own mix of runes (Law, plus elemental runes).

| Spell | Magic level | Runes | Max hit |
|---|--:|---|--:|
| Wind Strike | 1 | 1x Air rune | 2 |
| Water Strike | 5 | 1x Water rune | 3 |
| Earth Strike | 9 | 1x Earth rune | 4 |
| Fire Strike | 13 | 1x Fire rune | 5 |
| Earth Bolt | 29 | 3x Earth rune + 1x Chaos rune | 7 |
| Fire Bolt | 35 | 3x Fire rune + 1x Chaos rune | 8 |
| Home Teleport | 1 | 1x Air rune + 1x Law rune | teleport |
| Bank Teleport | 20 | 1x Law rune + 1x Fire rune + 1x Air rune | teleport |
| Cave Teleport * | 25 | 1x Law rune + 1x Water rune + 1x Fire rune | teleport |
| Enchant Jewel | 30 | 1x Law rune + 1x Fire rune | enchant |

*Cave Teleport is locked until you complete **The Warlord's Bane** quest.

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
Crafting level but gives the best Ranged bonuses in the valley. The same
needle also mounts **dragonstone** gems on **gold bars** into jewelry (see
the Jewellery section).

| | Craft | Materials | Crafting level |
|:-:|---|---|--:|
| ![](docs/almanac/item_leather_coif.png) | Leather coif | 1x Leather + 1x Thread | 5 |
| ![](docs/almanac/item_leather_body.png) | Leather body | 3x Leather + 1x Thread | 14 |
| ![](docs/almanac/item_dhide_coif.png) | D'hide coif | 1x Dragon leather + 1x Thread | 35 |
| ![](docs/almanac/item_dhide_body.png) | D'hide body | 2x Dragon leather + 1x Thread | 40 |
| ![](docs/almanac/item_dstone_ring.png) | Dstone ring | 1x Dragonstone + 1x Gold bar | 45 |
| ![](docs/almanac/item_dstone_bracelet.png) | Dstone bracelet | 1x Dragonstone + 1x Gold bar | 48 |
| ![](docs/almanac/item_dstone_amulet.png) | Dstone amulet | 1x Dragonstone + 1x Gold bar | 50 |
| ![](docs/almanac/item_dstone_necklace.png) | Dstone necklace | 1x Dragonstone + 1x Gold bar | 55 |

---

## Jewellery

Mine **gold** (Mining 40) and smelt it into **gold bars** (Smithing 40). Craft
a **dragonstone + gold bar** into a plain piece (Crafting), then cast **Enchant
Jewel** (Magic 30, 1 Law + 1 Fire rune) on it for a combat boost. Wear one
**neck** piece (amulet or necklace) and one **hand** piece (ring or bracelet).

| | Jewellery | Slot | Attack | Strength | Defence | Magic | Ranged |
|:-:|---|---|--:|--:|--:|--:|--:|
| ![](docs/almanac/item_guard_bracelet.png) | Bracelet of Guard | Hand | +6 | +0 | +6 | +0 | +0 |
| ![](docs/almanac/item_fury_ring.png) | Ring of Fury | Hand | +0 | +8 | +0 | +0 | +0 |
| ![](docs/almanac/item_glory_amulet.png) | Amulet of Glory | Neck | +8 | +8 | +8 | +0 | +0 |
| ![](docs/almanac/item_power_necklace.png) | Necklace of Power | Neck | +0 | +0 | +0 | +10 | +8 |

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
