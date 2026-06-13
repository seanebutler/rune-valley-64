# Regenerates ALMANAC.md from the data tables in src/main.c (single source of
# truth), so the out-of-game reference can never drift from the game.
#   pwsh assets/gen_almanac.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$src  = Get-Content (Join-Path $root 'src\main.c') -Raw
$outPath = Join-Path $root 'ALMANAC.md'

# --- item id -> display name + heal (every item) ---
$itemName = @{}
$itemHeal = @{}
foreach ($m in [regex]::Matches($src, '\[IT_(\w+)\]\s*=\s*\{\s*"([^"]*)"\s*,\s*\w+\s*,\s*(\d+)')) {
    $itemName[$m.Groups[1].Value] = $m.Groups[2].Value
    $itemHeal[$m.Groups[1].Value] = [int]$m.Groups[3].Value
}
function Item-Label($id) {
    if ($id -eq 'IT_NONE') { return 'Nothing' }
    $k = $id -replace '^IT_',''
    if ($itemName.ContainsKey($k) -and $itemName[$k]) { return $itemName[$k] }
    return $k
}

# --- equippable items: [IT_X]={ "name", SPR, heal, bool, SLOT_Y, atk,str,def,lvl [,bool,mag[,rng]] } ---
$equip = @()
$rxE = '\[IT_(\w+)\]\s*=\s*\{\s*"([^"]+)"\s*,\s*\w+\s*,\s*\d+\s*,\s*(?:true|false)\s*,\s*(SLOT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*(?:true|false)\s*,\s*(\d+)\s*(?:,\s*(\d+)\s*)?)?\}'
foreach ($m in [regex]::Matches($src, $rxE)) {
    if ($m.Groups[3].Value -eq 'SLOT_NONE') { continue }
    $equip += [pscustomobject]@{
        Name = $m.Groups[2].Value
        Slot = $m.Groups[3].Value
        Atk  = [int]$m.Groups[4].Value
        Str  = [int]$m.Groups[5].Value
        Def  = [int]$m.Groups[6].Value
        Lvl  = [int]$m.Groups[7].Value
        Mag  = $(if ($m.Groups[8].Success) { [int]$m.Groups[8].Value } else { 0 })
        Rng  = $(if ($m.Groups[9].Success) { [int]$m.Groups[9].Value } else { 0 })
    }
}

# --- arrows: stackable, slot 0, [...,bool,mag,rng] (a Ranged bonus, no slot) ---
$arrows = @()
$rxA = '\[IT_(\w+)\]\s*=\s*\{\s*"([^"]+)"\s*,\s*\w+\s*,\s*\d+\s*,\s*(?:true|false)\s*,\s*0\s*,\s*0\s*,\s*0\s*,\s*0\s*,\s*0\s*,\s*(?:true|false)\s*,\s*\d+\s*,\s*(\d+)\s*\}'
foreach ($m in [regex]::Matches($src, $rxA)) {
    $arrows += [pscustomobject]@{ Name = $m.Groups[2].Value; Rng = [int]$m.Groups[3].Value }
}

# --- monster stats: [MOB_X] = { "name", hp, dmg, def, ... } ---
$mobs = @{}
foreach ($m in [regex]::Matches($src, '\[MOB_(\w+)\]\s*=\s*\{\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,')) {
    $mobs[$m.Groups[1].Value] = [pscustomobject]@{
        Name = $m.Groups[2].Value
        HP   = [int]$m.Groups[3].Value
        Hit  = [int]$m.Groups[4].Value
        Def  = [int]$m.Groups[5].Value
    }
}

# --- spells: { "name", rune, runes, rune2, runes2, maxhit, lvl, xp } ---
$spells = @()
$mSpellBlock = [regex]::Match($src, 'spellinfo\[NUM_SPELLS\]\s*=\s*\{(.*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if ($mSpellBlock.Success) {
    $rxS = '\{\s*"([^"]+)"\s*,\s*(IT_\w+)\s*,\s*(\d+)\s*,\s*(IT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}'
    foreach ($m in [regex]::Matches($mSpellBlock.Groups[1].Value, $rxS)) {
        if ($m.Groups[1].Value -eq 'Melee') { continue }
        $spells += [pscustomobject]@{
            Name=$m.Groups[1].Value; Rune=$m.Groups[2].Value; Runes=[int]$m.Groups[3].Value
            Rune2=$m.Groups[4].Value; Runes2=[int]$m.Groups[5].Value
            MaxHit=[int]$m.Groups[6].Value; Lvl=[int]$m.Groups[7].Value
        }
    }
}

# --- fishing tiers: { "name", raw, cooked, tool, fishLvl, cookLvl, ... } ---
$fish = @()
$mFishBlock = [regex]::Match($src, 'fishinfo\[NUM_FISH\]\s*=\s*\{(.*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if ($mFishBlock.Success) {
    $rxF = '\{\s*"([^"]+)"\s*,\s*(IT_\w+)\s*,\s*(IT_\w+)\s*,\s*(IT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,'
    foreach ($m in [regex]::Matches($mFishBlock.Groups[1].Value, $rxF)) {
        $cooked = $m.Groups[3].Value -replace '^IT_',''
        $fish += [pscustomobject]@{
            Name=$m.Groups[1].Value; Tool=$m.Groups[4].Value
            FishLvl=[int]$m.Groups[5].Value; CookLvl=[int]$m.Groups[6].Value
            Heal=$(if ($itemHeal.ContainsKey($cooked)) { $itemHeal[$cooked] } else { 0 })
        }
    }
}

# --- crafting recipes: { result, hide, n_hide, n_thread, lvl, xp } ---
$crafts = @()
$mCraftBlock = [regex]::Match($src, 'craft_list\[\]\s*=\s*\{(.*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if ($mCraftBlock.Success) {
    $rxC = '\{\s*(IT_\w+)\s*,\s*(IT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,'
    foreach ($m in [regex]::Matches($mCraftBlock.Groups[1].Value, $rxC)) {
        $crafts += [pscustomobject]@{
            Result=$m.Groups[1].Value; Hide=$m.Groups[2].Value
            NHide=[int]$m.Groups[3].Value; NThread=[int]$m.Groups[4].Value
            Lvl=[int]$m.Groups[5].Value
        }
    }
}

# --- mob -> drop-table name (parsed from the mob_drops() switch) ---
$mobTable = @{}
foreach ($m in [regex]::Matches($src, 'case\s+MOB_(\w+):\s*\*n\s*=\s*NDROPS\((\w+)\)')) {
    $mobTable[$m.Groups[1].Value] = $m.Groups[2].Value
}

# --- drop tables: name -> list of {item, qty, weight} ---
$drops = @{}
foreach ($m in [regex]::Matches($src, 'static const drop_t (\w+)\[\]\s*=\s*\{(.*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
    $list = @()
    foreach ($d in [regex]::Matches($m.Groups[2].Value, '\{\s*(IT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}')) {
        $list += [pscustomobject]@{ Item = $d.Groups[1].Value; Qty = [int]$d.Groups[2].Value; Weight = [int]$d.Groups[3].Value }
    }
    $drops[$m.Groups[1].Value] = $list
}

# --- stable, hand-kept prose (location/flavour the data tables don't carry) ---
$mobOrder  = 'COW','GOBLIN','SKELETON','BOSS','WIGHT','DEMON','WHELP','DRAGON'
$mobNote   = @{
    COW='overworld pasture'; GOBLIN='east of the bridge'; SKELETON='dungeon floor 1'
    BOSS='boss, dungeon floor 1'; WIGHT='dungeon floor 2'; DEMON='boss, dungeon floor 2'
    WHELP='dungeon floor 3 (the Dragon''s brood)'; DRAGON='boss, dungeon floor 3'
}
$mobAlways = @{ DRAGON='Always drops 2500 coins + a Dragonstone.'
                BOSS='Always pays out (no "nothing").'; DEMON='Always pays out (no "nothing").' }
$mobExtra  = @{ DRAGON=@(
    'Fire breath: a ranged attack (up to 5 tiles, ignores armour) for 5 damage, rising to 8 when enraged.',
    'Enrages at half health: attacks and breathes faster and hits harder.',
    'On enraging it summons 2 whelps (they vanish and drop nothing on death).') }
$slotLabel = @{ SLOT_WEAPON='Weapon'; SLOT_HELM='Helm'; SLOT_SHIELD='Shield'; SLOT_BODY='Body' }

# --- emit markdown ---
$o = [System.Collections.Generic.List[string]]::new()
function W($s='') { $o.Add($s) }

W '# Rune Valley 64 - Almanac'
W ''
W 'A quick-reference for all equipment stats and monster stats/loot. This mirrors'
W 'the **in-game Almanac** (open the help screen with **Start**, then press **A**'
W 'twice).'
W ''
W '> Auto-generated from `src/main.c` by `assets/gen_almanac.ps1` - do not edit by'
W '> hand; re-run the script after changing gear, monsters or loot.'
W ''
W '---'
W ''
W '## Weapons'
W ''
W 'Worn in the weapon slot. The Magic bonus raises spell accuracy and max hit.'
W ''
W '| Weapon | Attack | Strength | Magic | Defence | Wield level |'
W '|---|--:|--:|--:|--:|--:|'
foreach ($w in ($equip | Where-Object { $_.Slot -eq 'SLOT_WEAPON' -and $_.Rng -eq 0 } | Sort-Object Lvl, Atk)) {
    W ("| {0} | +{1} | +{2} | +{3} | +{4} | {5} |" -f $w.Name, $w.Atk, $w.Str, $w.Mag, $w.Def, $w.Lvl)
}
W ''
W '## Armour'
W ''
W 'Shields carry a small Attack bonus; wizard gear carries a Magic bonus.'
W ''
W '| Item | Slot | Attack | Defence | Magic | Wield level |'
W '|---|---|--:|--:|--:|--:|'
foreach ($slot in 'SLOT_HELM','SLOT_SHIELD','SLOT_BODY') {
    foreach ($a in ($equip | Where-Object { $_.Slot -eq $slot -and $_.Rng -eq 0 } | Sort-Object Lvl, Name)) {
        W ("| {0} | {1} | +{2} | +{3} | +{4} | {5} |" -f $a.Name, $slotLabel[$slot], $a.Atk, $a.Def, $a.Mag, $a.Lvl)
    }
}
W ''
W '---'
W ''
W '## Monsters & loot'
W ''
W 'Every monster always drops **bones**. The percentages are a single weighted'
W 'roll for the rest of the drop.'
W ''
foreach ($id in $mobOrder) {
    if (-not $mobs.ContainsKey($id)) { continue }
    $mob = $mobs[$id]
    $disp = $mob.Name.Substring(0,1).ToUpper() + $mob.Name.Substring(1)
    W ("### {0} - {1}" -f $disp, $mobNote[$id])
    W ("Hitpoints **{0}** | max hit **{1}** | defence **{2}**." -f $mob.HP, $mob.Hit, $mob.Def)
    if ($mobAlways.ContainsKey($id)) { W ''; W ("**{0}**" -f $mobAlways[$id]) }
    if ($mobExtra.ContainsKey($id)) {
        W ''
        foreach ($line in $mobExtra[$id]) { W ("- {0}" -f $line) }
    }
    W ''
    $tbl = $drops[$mobTable[$id]]
    $total = ($tbl | Measure-Object Weight -Sum).Sum
    W '| Drop | Qty | Chance |'
    W '|---|--:|--:|'
    foreach ($d in $tbl) {
        $pct = [math]::Round($d.Weight * 100.0 / $total)
        $qty = if ($d.Item -eq 'IT_NONE') { '-' } else { "$($d.Qty)" }
        W ("| {0} | {1} | {2}% |" -f (Item-Label $d.Item), $qty, $pct)
    }
    W ''
}
W '---'
W ''
W '## Spells'
W ''
W 'Cast from the spellbook (C-left). Bolts need a Chaos rune on top of their'
W 'element; teleports are powered by Law runes (Home uses an Air rune).'
W ''
W '| Spell | Magic level | Runes | Max hit |'
W '|---|--:|---|--:|'
foreach ($sp in $spells) {
    $cost = "$($sp.Runes)x $(Item-Label $sp.Rune)"
    if ($sp.Runes2 -gt 0) { $cost += " + $($sp.Runes2)x $(Item-Label $sp.Rune2)" }
    $hit = if ($sp.MaxHit -gt 0) { "$($sp.MaxHit)" } else { 'teleport' }
    W ("| {0} | {1} | {2} | {3} |" -f $sp.Name, $sp.Lvl, $cost, $hit)
}
W ''
W '---'
W ''
W '## Ranged'
W ''
W 'Equip a bow in the weapon slot and carry arrows, then fire at a goblin from up'
W 'to five tiles away. Each shot spends one arrow and trains Ranged; accuracy and'
W 'max hit add the bow''s, the arrow''s and your ranged armour''s Ranged bonuses.'
W 'Bows and arrows are fletched (use a knife on logs). **Mithril arrows need an'
W 'oak shortbow or better** - a plain shortbow can''t draw them.'
W ''
W '| Bow | Ranged | Wield level |'
W '|---|--:|--:|'
foreach ($b in ($equip | Where-Object { $_.Slot -eq 'SLOT_WEAPON' -and $_.Rng -gt 0 } | Sort-Object Lvl)) {
    W ("| {0} | +{1} | {2} |" -f $b.Name, $b.Rng, $b.Lvl)
}
W ''
W '| Arrow | Ranged |'
W '|---|--:|'
foreach ($a in ($arrows | Sort-Object Rng)) {
    W ("| {0} | +{1} |" -f $a.Name, $a.Rng)
}
W ''
W 'Ranged armour is crafted (see below) and worn for a Ranged bonus; it needs the'
W 'listed Ranged level to wear.'
W ''
W '| Ranged armour | Slot | Ranged | Defence | Ranged level |'
W '|---|---|--:|--:|--:|'
foreach ($a in ($equip | Where-Object { $_.Rng -gt 0 -and $_.Slot -ne 'SLOT_WEAPON' } | Sort-Object Lvl, Name)) {
    W ("| {0} | {1} | +{2} | +{3} | {4} |" -f $a.Name, $slotLabel[$a.Slot], $a.Rng, $a.Def, $a.Lvl)
}
W ''
W '---'
W ''
W '## Crafting'
W ''
W 'Slay cows for **cowhide** (and the Ancient Dragon for **dragonhide**), then'
W 'have **Pelt the Tanner** by the cow pasture cure them into leather for a few'
W 'coins. Use a **needle** on the leather (with **thread** in your pack) to stitch'
W 'ranged armour - both sold at the General Store. Dragonhide gear demands a high'
W 'Crafting level but gives the best Ranged bonuses in the valley.'
W ''
W '| Stitch | Materials | Crafting level |'
W '|---|---|--:|'
foreach ($c in $crafts) {
    W ("| {0} | {1}x {2} + {3}x Thread | {4} |" -f (Item-Label $c.Result), $c.NHide, (Item-Label $c.Hide), $c.NThread, $c.Lvl)
}
W ''
W '---'
W ''
W '## Fishing & food'
W ''
W 'The tackle you carry decides the catch at a fishing spot. Cook the catch on'
W 'a fire (it burns until your Cooking level reaches the listed level); eating'
W 'the cooked fish restores the listed Hitpoints.'
W ''
W '| Fish | Fishing level | Tackle | Cooking level | Heals |'
W '|---|--:|---|--:|--:|'
foreach ($fi in $fish) {
    $disp = $fi.Name.Substring(0,1).ToUpper() + $fi.Name.Substring(1)
    W ("| {0} | {1} | {2} | {3} | {4} |" -f $disp, $fi.FishLvl, (Item-Label $fi.Tool), $fi.CookLvl, $fi.Heal)
}
W ''
W '---'
W ''
W '## Notes'
W ''
W '- **Dragonstone** is a trophy gem worth ~1000 coins at any shop''s Sell tab.'
W '- The **Dragonfire blade** is unique and cannot be sold.'
W '- The XP table is the real Old School one (level 99 = 13,034,431 xp).'

$text = ($o -join "`n") + "`n"
[System.IO.File]::WriteAllText($outPath, $text, [System.Text.UTF8Encoding]::new($false))
Write-Host ("Wrote {0} ({1} lines, {2} weapons+armour, {3} monsters)" -f $outPath, $o.Count, $equip.Count, $mobs.Count)
