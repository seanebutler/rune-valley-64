# Regenerates ALMANAC.md from the data tables in src/main.c (single source of
# truth), so the out-of-game reference can never drift from the game.
#   pwsh assets/gen_almanac.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$src  = Get-Content (Join-Path $root 'src\main.c') -Raw
$outPath = Join-Path $root 'ALMANAC.md'

# --- item id -> display name (every item) ---
$itemName = @{}
foreach ($m in [regex]::Matches($src, '\[IT_(\w+)\]\s*=\s*\{\s*"([^"]*)"')) {
    $itemName[$m.Groups[1].Value] = $m.Groups[2].Value
}
function Item-Label($id) {
    if ($id -eq 'IT_NONE') { return 'Nothing' }
    $k = $id -replace '^IT_',''
    if ($itemName.ContainsKey($k) -and $itemName[$k]) { return $itemName[$k] }
    return $k
}

# --- equippable items: [IT_X]={ "name", SPR, heal, bool, SLOT_Y, atk,str,def,lvl [,bool,mag] } ---
$equip = @()
$rxE = '\[IT_(\w+)\]\s*=\s*\{\s*"([^"]+)"\s*,\s*\w+\s*,\s*\d+\s*,\s*(?:true|false)\s*,\s*(SLOT_\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*(?:true|false)\s*,\s*(\d+)\s*)?\}'
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
    }
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
foreach ($w in ($equip | Where-Object { $_.Slot -eq 'SLOT_WEAPON' } | Sort-Object Lvl, Atk)) {
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
    foreach ($a in ($equip | Where-Object { $_.Slot -eq $slot } | Sort-Object Lvl, Name)) {
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
W '## Notes'
W ''
W '- **Dragonstone** is a trophy gem worth ~1000 coins at any shop''s Sell tab.'
W '- The **Dragonfire blade** is unique and cannot be sold.'
W '- The XP table is the real Old School one (level 99 = 13,034,431 xp).'

$text = ($o -join "`n") + "`n"
[System.IO.File]::WriteAllText($outPath, $text, [System.Text.UTF8Encoding]::new($false))
Write-Host ("Wrote {0} ({1} lines, {2} weapons+armour, {3} monsters)" -f $outPath, $o.Count, $equip.Count, $mobs.Count)
