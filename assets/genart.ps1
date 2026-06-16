# Generates all pixel-art PNGs for Rune Valley 64 (OSRS-inspired, original art).
# ASCII grids -> PNG via System.Drawing. '.' = transparent.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot 'png'
New-Item -ItemType Directory -Force $outDir | Out-Null

function Save-Sprite([string]$name, [hashtable]$pal, [string[]]$rows) {
    $h = $rows.Count
    $w = $rows[0].Length
    foreach ($r in $rows) { if ($r.Length -ne $w) { throw "$name : row length mismatch ($($r.Length) vs $w): '$r'" } }
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $c = $rows[$y][$x]
            if ($c -eq '.') {
                $col = [System.Drawing.Color]::FromArgb(0, 0, 0, 0)
            } else {
                $hex = $pal["$c"]
                if (-not $hex) { throw "$name : no palette entry for '$c'" }
                $r8 = [Convert]::ToInt32($hex.Substring(1,2),16)
                $g8 = [Convert]::ToInt32($hex.Substring(3,2),16)
                $b8 = [Convert]::ToInt32($hex.Substring(5,2),16)
                $col = [System.Drawing.Color]::FromArgb(255, $r8, $g8, $b8)
            }
            $bmp.SetPixel($x, $y, $col)
        }
    }
    $bmp.Save((Join-Path $outDir "$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "  $name ($w x $h)"
}

# Deterministic noise tile: base char + sprinkles
function New-NoiseTile([string]$name, [hashtable]$pal, [char]$base, [object[]]$sprinkles, [int]$seed) {
    $rng = New-Object System.Random($seed)
    $grid = @()
    for ($y = 0; $y -lt 16; $y++) { $grid += ,([char[]]("$base" * 16)) }
    foreach ($sp in $sprinkles) {
        $ch = $sp[0]; $count = $sp[1]
        for ($i = 0; $i -lt $count; $i++) {
            $x = $rng.Next(16); $y = $rng.Next(16)
            $grid[$y][$x] = $ch
        }
    }
    $rows = $grid | ForEach-Object { -join $_ }
    Save-Sprite $name $pal $rows
}

Write-Host "Generating terrain tiles..."

$grassPal = @{ 'a'='#4f7a32'; 'b'='#446b2a'; 'c'='#5b8c3a'; 'd'='#3a5c24' }
New-NoiseTile 'tile_grass_a' $grassPal 'a' @(,@('b',26),@('c',18),@('d',8)) 1001
New-NoiseTile 'tile_grass_b' $grassPal 'a' @(,@('b',22),@('c',22),@('d',6)) 2002

$pathPal = @{ 'a'='#a8895a'; 'b'='#94774d'; 'c'='#b89a6c'; 'd'='#86683f' }
New-NoiseTile 'tile_path' $pathPal 'a' @(,@('b',24),@('c',16),@('d',10)) 3003

$sandPal = @{ 'a'='#c9b673'; 'b'='#b8a462'; 'c'='#d8c685' }
New-NoiseTile 'tile_sand' $sandPal 'a' @(,@('b',20),@('c',16)) 4004

# Water: two frames, wave highlights shift
$waterPal = @{ 'a'='#33558e'; 'b'='#4a6fae'; 'c'='#6288c4'; 'd'='#2a4878' }
Save-Sprite 'tile_water_a' $waterPal @(
    'aaaaaaaaaaaaaaaa',
    'aabbcbbaaaaaabba',
    'aaaaaaaaaaaaaaaa',
    'daaaaaaaabbcbbaa',
    'aaaaaaaaaaaaaaaa',
    'aaabbbaaaaaaaaad',
    'aaaaaaaaaaaaaaaa',
    'aaaaaaaabbcbbaaa',
    'aaaaaaaaaaaaaaaa',
    'abbcbbaaaaaaaaaa',
    'aaaaaaaaaaaadaaa',
    'aaaaaaabbbaaaaaa',
    'aaadaaaaaaaaaaaa',
    'aaaaaaaaaaabbcba',
    'aabbbaaaaaaaaaaa',
    'aaaaaaaaaaaaaaaa')
Save-Sprite 'tile_water_b' $waterPal @(
    'aaaaaaaaaaaaaaaa',
    'aaaaaabbcbbaaaaa',
    'aaaaaaaaaaaaaaaa',
    'aabbcbbaaaaaaada',
    'aaaaaaaaaaaaaaaa',
    'aaaaaaaaaabbbaaa',
    'adaaaaaaaaaaaaaa',
    'aaaabbcbbaaaaaaa',
    'aaaaaaaaaaaaaaaa',
    'aaaaaaaaaabbcbba',
    'aaadaaaaaaaaaaaa',
    'aaaaaaaaaaaaabbb',
    'aaaaaaaaadaaaaaa',
    'abbcbaaaaaaaaaaa',
    'aaaaaaaaabbbaaaa',
    'aaaaaaaaaaaaaaaa')

# Stone floor (bank interior) - warm tan so it reads differently from walls
Save-Sprite 'tile_floor' @{ 'a'='#a09078'; 'b'='#847660'; 'c'='#b0a088' } @(
    'aaaaaaabaaaaaaab',
    'acaaaaabaaacaaab',
    'aaaaaaabaaaaaaab',
    'aaacaaabaaaaacab',
    'aaaaaaabaaaaaaab',
    'aaaaaaabacaaaaab',
    'aaaaaaabaaaaaaab',
    'bbbbbbbbbbbbbbbb',
    'aaabaaaaaaabaaaa',
    'aaabacaaaaabaaca',
    'acabaaaaaaabaaaa',
    'aaabaaaaacabaaaa',
    'aaabaaaaaaabaaaa',
    'aaabaaaaaaabacaa',
    'aaabaaaaaaabaaaa',
    'bbbbbbbbbbbbbbbb')

# Stone wall
Save-Sprite 'tile_wall' @{ 'a'='#7d7d78'; 'b'='#55554f'; 'c'='#92928c'; 'd'='#68685f' } @(
    'cccccccccccccccc',
    'aaaaaaabaaaaaaab',
    'aaaaaaabaaaaaaab',
    'adaaaaabaadaaaab',
    'bbbbbbbbbbbbbbbb',
    'aaabaaaaaaabaaaa',
    'aaabaaadaaabaaaa',
    'aaabaaaaaaabadaa',
    'bbbbbbbbbbbbbbbb',
    'aaaaaaabaaaaaaab',
    'adaaaaabaaaaadab',
    'aaaaaaabaaaaaaab',
    'bbbbbbbbbbbbbbbb',
    'aaabaaaaaaabaaaa',
    'aaabaadaaaabaaaa',
    'bbbbbbbbbbbbbbbb')

# Bridge planks (walkable over water)
Save-Sprite 'tile_bridge' @{ 'a'='#8a6a40'; 'b'='#6e5230'; 'c'='#9c7c50'; 'd'='#5a4226' } @(
    'dddddddddddddddd',
    'aaaaaaaaaaaaaaaa',
    'acaaaaacaaaaacaa',
    'bbbbbbbbbbbbbbbb',
    'aaaaaaaaaaaaaaaa',
    'aacaaaaaacaaaaaa',
    'bbbbbbbbbbbbbbbb',
    'aaaaaaaaaaaaaaaa',
    'acaaaacaaaaacaaa',
    'bbbbbbbbbbbbbbbb',
    'aaaaaaaaaaaaaaaa',
    'aaaacaaaaacaaaca',
    'bbbbbbbbbbbbbbbb',
    'aaaaaaaaaaaaaaaa',
    'aacaaaaacaaaaaca',
    'dddddddddddddddd')

Write-Host "Generating objects..."

# Tree: 16x24, canopy + trunk
$treePal = @{ 'c'='#2e5c20'; 'e'='#3f7a2c'; 'f'='#4f9438'; 't'='#6b4a2a'; 'u'='#553a20' }
Save-Sprite 'obj_tree' $treePal @(
    '.....ccccc......',
    '...ccceeeccc....',
    '..cceeefeeecc...',
    '.cceefffeeeecc..',
    '.ceeeffeeeeeec..',
    'cceefffeeeeeecc.',
    'ceeeffeeeefeeec.',
    'ceefffeeeffeeec.',
    'ceeeeeeeeffeeec.',
    'cceeeeeeeeeeecc.',
    '.ceeeeeeeeeeec..',
    '.cceeeeeeeeecc..',
    '..cceeeeeeecc...',
    '...ccceeeccc....',
    '....cctucc......',
    '......tu........',
    '......tu........',
    '......tu........',
    '......tu........',
    '.....ttuu.......',
    '.....ttuu.......',
    '....tttuuu......',
    '................',
    '................')

# Oak: wider, darker canopy
$oakPal = @{ 'c'='#23491a'; 'e'='#306325'; 'f'='#3f7a2c'; 't'='#5e4024'; 'u'='#48301a' }
Save-Sprite 'obj_oak' $oakPal @(
    '....ccccccc.....',
    '..ccceeeeeccc...',
    '.cceeefffeeecc..',
    '.ceeeffffeeeec..',
    'cceefffeeeeeecc.',
    'ceeeffeeeeefeec.',
    'ceefffeeeeffeec.',
    'ceeeeeeeefffeec.',
    'ceeeeeeeeeffeec.',
    'cceeeeeeeeeeecc.',
    'cceeeeeeeeeeecc.',
    '.cceeeeeeeeecc..',
    '..cceeeeeeecc...',
    '...ccceeeccc....',
    '....ccttucc.....',
    '.....ttuu.......',
    '.....ttuu.......',
    '.....ttuu.......',
    '.....ttuu.......',
    '....tttuuu......',
    '....tttuuu......',
    '...ttttuuuu.....',
    '................',
    '................')

# Stump
Save-Sprite 'obj_stump' @{ 't'='#6b4a2a'; 'u'='#553a20'; 'r'='#8a6a44'; 's'='#9c7c54' } @(
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '.....rrrrrr.....',
    '....rsssssr.....',
    '....rsrrssrr....',
    '....tssrsstu....',
    '....ttuuuuuu....',
    '....ttuuuuuu....',
    '...ttttuuuuuu...',
    '................',
    '................')

# Rocks: boulder with ore flecks
function New-Rock([string]$name, [string]$fleck) {
    $pal = @{ 'a'='#7a7a74'; 'b'='#5e5e58'; 'c'='#92928a'; 'o'=$fleck }
    Save-Sprite $name $pal @(
        '................',
        '................',
        '....bbbbbb......',
        '..bbacccabbb....',
        '.baccccccaabb...',
        '.bacccoccccab...',
        'bacccccccccaab..',
        'baccoccccoccab..',
        'baccccccccccab..',
        'bacccccoccccaab.',
        'baacccccccccaab.',
        'bbaaccocccaaabb.',
        'bbaaaaaaaaaaabb.',
        '.bbbaaaaaaabbb..',
        '..bbbbbbbbbbb...',
        '................')
}
New-Rock 'obj_rock_copper' '#c06a2e'
New-Rock 'obj_rock_tin'    '#c8c8d2'
New-Rock 'obj_rock_iron'   '#8a3c2e'

Save-Sprite 'obj_rock_empty' @{ 'a'='#7a7a74'; 'b'='#5e5e58'; 'c'='#92928a' } @(
    '................',
    '................',
    '....bbbbbb......',
    '..bbacccabbb....',
    '.baccccccaabb...',
    '.baccccccccab...',
    'bacccccccccaab..',
    'baccccccccccab..',
    'baccccccccccab..',
    'baccccccccccaab.',
    'baacccccccccaab.',
    'bbaaccccccaaabb.',
    'bbaaaaaaaaaaabb.',
    '.bbbaaaaaaabbb..',
    '..bbbbbbbbbbb...',
    '................')

# Fishing spot ripples (drawn over water), 2 frames
$ripPal = @{ 'w'='#b8d4f0'; 'x'='#88aede' }
Save-Sprite 'obj_fish_a' $ripPal @(
    '................',
    '................',
    '.....wwww.......',
    '...ww....ww.....',
    '..w........w....',
    '..w........w....',
    '...ww....ww.....',
    '.....wwww.......',
    '................',
    '..........xx....',
    '.........x..x...',
    '..........xx....',
    '....x...........',
    '...x.x..........',
    '....x...........',
    '................')
Save-Sprite 'obj_fish_b' $ripPal @(
    '................',
    '......xx........',
    '.....x..x.......',
    '......xx........',
    '................',
    '...........ww...',
    '..........w..w..',
    '..........w..w..',
    '...........ww...',
    '................',
    '...wwww.........',
    '..w....w........',
    '..w....w........',
    '...wwww.........',
    '................',
    '................')

# Bank booth
Save-Sprite 'obj_booth' @{ 'a'='#5a3c22'; 'b'='#46301c'; 'c'='#6e4c2c'; 'g'='#c8a23c'; 's'='#8a8a85' } @(
    'ssssssssssssssss',
    'sggggggggggggggs',
    'saaaaaaaaaaaaaas',
    'sabbaabbaabbaabs',
    'sabbaabbaabbaabs',
    'saaaaaaaaaaaaaas',
    'scaaccaaccaaccas',
    'scaaccaaccaaccas',
    'saaaaaaaaaaaaaas',
    'sabbaabbaabbaabs',
    'sabbaabbaabbaabs',
    'saaaaaaaaaaaaaas',
    'scaaccaaccaaccas',
    'scaaccaaccaaccas',
    'sggggggggggggggs',
    'ssssssssssssssss')

# Fire, 2 frames
$firePal = @{ 'r'='#d83c14'; 'o'='#f07820'; 'y'='#f8c83c'; 'w'='#fff0a0'; 't'='#5a3c22' }
Save-Sprite 'obj_fire_a' $firePal @(
    '................',
    '......r.........',
    '.....rr...r.....',
    '....rro..rr.....',
    '....roo.rro.....',
    '...rrooorroo....',
    '...roooyyooo....',
    '..rrooyyyyoor...',
    '..rooyywwyyor...',
    '..rooyywwyyoor..',
    '.rrooyywwwyyor..',
    '.roooyywwyyyoor.',
    '.rooyyywwwyyoor.',
    '..ttttttttttt...',
    '.ttttttttttttt..',
    '................')
Save-Sprite 'obj_fire_b' $firePal @(
    '................',
    '.........r......',
    '....r...rr......',
    '....rr..orr.....',
    '....orr.oor.....',
    '...oorrooorr....',
    '...oooyyooor....',
    '..rooyyyyoorr...',
    '..royyywwyyor...',
    '.rrooyywwyyoor..',
    '.rooyywwwyyoor..',
    '.rooyyywwyyyor..',
    '.rooyywwwyyyor..',
    '..ttttttttttt...',
    '.ttttttttttttt..',
    '................')

Write-Host "Generating characters..."

# Player: 16x24. h=hair s=skin e=eye t=tunic(steel blue) l=legs b=boots m=belt
$plPal = @{ 'h'='#4a3320'; 's'='#d9a066'; 'e'='#1a1a1a'; 't'='#3e5e7e'; 'l'='#4a4a52'; 'b'='#3a2a18'; 'm'='#6b4a2a' }
Save-Sprite 'pl_down_a' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hssssssh....',
    '....ssessess....',
    '....ssssssss....',
    '.....ssssss.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '....lll..lll....',
    '....lll..lll....',
    '....lll..lll....',
    '....lll..lll....',
    '....bbb..bbb....',
    '....bbb..bbb....',
    '................')
Save-Sprite 'pl_down_b' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hssssssh....',
    '....ssessess....',
    '....ssssssss....',
    '.....ssssss.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '....lll..lll....',
    '....lll..bbb....',
    '....lll..bbb....',
    '....lll.........',
    '....bbb.........',
    '....bbb.........',
    '................')
Save-Sprite 'pl_up_a' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....shhhhhhs....',
    '.....hhhhhh.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '....lll..lll....',
    '....lll..lll....',
    '....lll..lll....',
    '....lll..lll....',
    '....bbb..bbb....',
    '....bbb..bbb....',
    '................')
Save-Sprite 'pl_up_b' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....shhhhhhs....',
    '.....hhhhhh.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '..ssttttttttss..',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '....bbb..lll....',
    '....bbb..lll....',
    '.........lll....',
    '.........lll....',
    '.........bbb....',
    '.........bbb....',
    '................')
Save-Sprite 'pl_side_a' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhsssss....',
    '....hhsssess....',
    '....hhssssss....',
    '.....hsssss.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '...ttttttttts...',
    '...ttttttttts...',
    '...ttttttttts...',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '.....llllll.....',
    '.....llllll.....',
    '.....llllll.....',
    '.....llllll.....',
    '.....bbbbbb.....',
    '.....bbbbbb.....',
    '................')
Save-Sprite 'pl_side_b' $plPal @(
    '................',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hhhhhhhh....',
    '....hhhsssss....',
    '....hhsssess....',
    '....hhssssss....',
    '.....hsssss.....',
    '......ssss......',
    '....tttttttt....',
    '...tttttttttt...',
    '...ttttttttts...',
    '...ttttttttts...',
    '...ttttttttts...',
    '...tttttttttt...',
    '....mmmmmmmm....',
    '....llllllll....',
    '....llll.lll....',
    '...llll...lll...',
    '...lll....lll...',
    '..blll.....lll..',
    '..bbb......bbb..',
    '...........bbb..',
    '................')

# Goblin: 16x16, 2 frames
$gobPal = @{ 'g'='#5a8a32'; 'd'='#46702a'; 'e'='#c83c1e'; 'l'='#7a5c34'; 'k'='#1a1a1a' }
Save-Sprite 'goblin_a' $gobPal @(
    '................',
    '.gg..........gg.',
    '.ggg.gggggg.ggg.',
    '.gggggggggggggg.',
    '..ggdggggggdgg..',
    '...gekggggekg...',
    '...gggggggggg...',
    '....ggdddggg....',
    '.....gggggg.....',
    '...gggggggggg...',
    '..gg.gggggg.gg..',
    '.....llllll.....',
    '.....llllll.....',
    '....ggg..ggg....',
    '....gg....gg....',
    '................')
Save-Sprite 'goblin_b' $gobPal @(
    '.gg..........gg.',
    '.ggg.gggggg.ggg.',
    '.gggggggggggggg.',
    '..ggdggggggdgg..',
    '...gekggggekg...',
    '...gggggggggg...',
    '....ggdddggg....',
    '.....gggggg.....',
    '...gggggggggg...',
    '..gg.gggggg.gg..',
    '.....llllll.....',
    '.....llllll.....',
    '....ggg.ggg.....',
    '....gg...gg.....',
    '................',
    '................')

Write-Host "Generating item icons..."

# Items are 16x16
Save-Sprite 'item_logs' @{ 'a'='#8a6a40'; 'b'='#6e5230'; 'c'='#b89468'; 'd'='#5a4226' } @(
    '................',
    '................',
    '................',
    '....aaaaaaaaab..',
    '..caaaaaaaaaab..',
    '..cbbbbbbbbbbb..',
    '................',
    '..aaaaaaaaab....',
    'caaaaaaaaaab....',
    'cbbbbbbbbbbb....',
    '................',
    '....aaaaaaaaab..',
    '..caaaaaaaaaab..',
    '..cbbbbbbbbbbb..',
    '................',
    '................')
Save-Sprite 'item_oak_logs' @{ 'a'='#6e5230'; 'b'='#553e22'; 'c'='#947452'; 'd'='#42301a' } @(
    '................',
    '................',
    '................',
    '....aaaaaaaaab..',
    '..caaaaaaaaaab..',
    '..cbbbbbbbbbbb..',
    '................',
    '..aaaaaaaaab....',
    'caaaaaaaaaab....',
    'cbbbbbbbbbbb....',
    '................',
    '....aaaaaaaaab..',
    '..caaaaaaaaaab..',
    '..cbbbbbbbbbbb..',
    '................',
    '................')

function New-OreIcon([string]$name, [string]$fleck) {
    $pal = @{ 'a'='#6e6e68'; 'b'='#54544e'; 'o'=$fleck }
    Save-Sprite $name $pal @(
        '................',
        '................',
        '................',
        '.....bbbbb......',
        '....baaaaab.....',
        '...baaoaaaab....',
        '...baaaaoaab....',
        '..baaoaaaaaab...',
        '..baaaaaoaaab...',
        '..baaoaaaaoab...',
        '..bbaaaoaaabb...',
        '...bbaaaaabb....',
        '....bbbbbbb.....',
        '................',
        '................',
        '................')
}
New-OreIcon 'item_copper_ore' '#c06a2e'
New-OreIcon 'item_tin_ore'    '#c8c8d2'
New-OreIcon 'item_iron_ore'   '#8a3c2e'

function New-ShrimpIcon([string]$name, [string]$body, [string]$dark) {
    $pal = @{ 'p'=$body; 'q'=$dark }
    Save-Sprite $name $pal @(
        '................',
        '................',
        '................',
        '......qppq......',
        '....qpppppq.....',
        '...qppqqpppq....',
        '...ppq...qppq...',
        '..qpq.....qpp...',
        '..qpq......pp...',
        '..qppq....qpq...',
        '...qppq..qppq...',
        '....qppqqppq....',
        '......qppq......',
        '................',
        '................',
        '................')
}
New-ShrimpIcon 'item_raw_shrimp'   '#e8b8b0' '#c08a84'
New-ShrimpIcon 'item_shrimp'       '#e87a5a' '#b85a40'
New-ShrimpIcon 'item_burnt_shrimp' '#3a3a3a' '#222222'

# Fish: a generic fish shape (trout); raw vs cooked differ by palette
function New-FishIcon([string]$name, [string]$body, [string]$dark) {
    Save-Sprite $name @{ 'p'=$body; 'q'=$dark } @(
        '................',
        '................',
        '.......pp.......',
        '.....pppppp.....',
        '...pppppppppp...',
        '..qpppppppppppq.',
        '.qpppppppppppqqq',
        '..qpppppppppppq.',
        '...pppppppppp...',
        '.....pppppp.....',
        '.......pp.......',
        '................',
        '................',
        '................',
        '................',
        '................')
}
New-FishIcon 'item_raw_trout' '#9aa6b0' '#5a6670'
New-FishIcon 'item_trout'     '#c88838' '#8a5a20'

# Lobster: claws up top, body, tail fan
function New-LobsterIcon([string]$name, [string]$body, [string]$dark) {
    Save-Sprite $name @{ 'p'=$body; 'q'=$dark } @(
        '................',
        '..p.........p...',
        '..pp.......pp...',
        '...pp.....pp....',
        '....pppppppp....',
        '...pppppppppp...',
        '...pqpppppqpp...',
        '....pppppppp....',
        '.....pppppp.....',
        '......p..p......',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................')
}
New-LobsterIcon 'item_raw_lobster' '#4a6a8a' '#2a4a6a'
New-LobsterIcon 'item_lobster'     '#d83820' '#a02010'

# Swordfish: a long fish with a pointed bill
function New-SwordfishIcon([string]$name, [string]$body, [string]$dark) {
    Save-Sprite $name @{ 'p'=$body; 'q'=$dark } @(
        '................',
        '................',
        '................',
        '.....ppppppp....',
        '....ppppppppppq.',
        'ppppppppppppppqq',
        '....ppppppppppq.',
        '.....ppppppp....',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................')
}
New-SwordfishIcon 'item_raw_swordfish' '#aeb6be' '#6a727a'
New-SwordfishIcon 'item_swordfish'     '#d09838' '#9a6820'

# Fishing tackle
Save-Sprite 'item_rod' @{ 'w'='#c8a060'; 'h'='#6a4a28' } @(
    '.............ww.',
    '............ww..',
    '...........ww...',
    '..........ww....',
    '.........ww.....',
    '........ww......',
    '.......ww.......',
    '......ww........',
    '.....ww.........',
    '....ww..........',
    '...hh...........',
    '..hh............',
    '.h..............',
    '................',
    '................',
    '................')
Save-Sprite 'item_lobster_pot' @{ 'w'='#a07840' } @(
    '................',
    '................',
    '...wwwwwwwww....',
    '..w.w.w.w.w.w...',
    '..wwwwwwwwwww...',
    '..w.w.w.w.w.w...',
    '..wwwwwwwwwww...',
    '..w.w.w.w.w.w...',
    '..wwwwwwwwwww...',
    '...wwwwwwwww....',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'item_harpoon' @{ 'w'='#b8c0c8'; 'h'='#7a5a30' } @(
    '.......w........',
    '......www.......',
    '.....w.w.w......',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '......hhh.......',
    '................',
    '................',
    '................')

# Fletching + ranged gear
Save-Sprite 'item_knife' @{ 'b'='#c8ccd2'; 'h'='#7a5a30' } @(
    '................',
    '................',
    '.............b..',
    '............bb..',
    '...........bb...',
    '..........bb....',
    '.........bb.....',
    '........bb......',
    '.......bb.......',
    '......bh........',
    '.....hh.........',
    '....hh..........',
    '...h............',
    '................',
    '................',
    '................')
Save-Sprite 'item_arrow_shaft' @{ 'w'='#c8a060' } @(
    '................',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '.......w........',
    '................',
    '................')
function New-TipIcon([string]$name, [string]$tip) {
    Save-Sprite $name @{ 't'=$tip } @(
        '................',
        '................',
        '................',
        '.......t........',
        '......ttt.......',
        '.....ttttt......',
        '......ttt.......',
        '.......t........',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................')
}
New-TipIcon 'item_bronze_tips' '#c08040'
New-TipIcon 'item_iron_tips'   '#a8a8b2'
function New-ArrowIcon([string]$name, [string]$tip) {
    Save-Sprite $name @{ 't'=$tip; 'w'='#c8a060'; 'f'='#e8e8e8' } @(
        '................',
        '.......t........',
        '......ttt.......',
        '.......w........',
        '.......w........',
        '.......w........',
        '.......w........',
        '.......w........',
        '......fwf.......',
        '.....f.w.f......',
        '................',
        '................',
        '................',
        '................',
        '................',
        '................')
}
New-ArrowIcon 'item_bronze_arrow' '#c08040'
New-ArrowIcon 'item_iron_arrow'   '#a8a8b2'
function New-BowIcon([string]$name, [string]$wood) {
    Save-Sprite $name @{ 'w'=$wood; 's'='#d8d8d0' } @(
        '....ww..........',
        '...w..s.........',
        '..w...s.........',
        '..w...s.........',
        '.w....s.........',
        '.w....s.........',
        '.w....s.........',
        '.w....s.........',
        '.w....s.........',
        '..w...s.........',
        '..w...s.........',
        '...w..s.........',
        '....ww..........',
        '................',
        '................',
        '................')
}
New-BowIcon 'item_shortbow' '#a8743a'
New-BowIcon 'item_oak_bow'  '#6e4a24'

Save-Sprite 'item_axe' @{ 'h'='#6e5230'; 'b'='#b87f4e'; 'd'='#94633a' } @(
    '................',
    '....bbb.........',
    '...bbbbb........',
    '..bbbbdbb.......',
    '..bbbdhhdb......',
    '..bdbhhhhd......',
    '...d.hhhh.......',
    '......hhh.......',
    '.......hhh......',
    '........hhh.....',
    '.........hhh....',
    '..........hhh...',
    '...........hhh..',
    '............hh..',
    '................',
    '................')
Save-Sprite 'item_pickaxe' @{ 'h'='#6e5230'; 'b'='#b87f4e'; 'd'='#94633a' } @(
    '................',
    '..bb............',
    '..dbbb..........',
    '...bdbbb........',
    '....hhdbbb......',
    '....hhh.dbb.....',
    '.....hhh..db....',
    '......hhh...b...',
    '.......hhh......',
    '........hhh.....',
    '.........hhh....',
    '..........hhh...',
    '...........hhh..',
    '............hh..',
    '................',
    '................')
Save-Sprite 'item_net' @{ 'n'='#c9b673'; 'd'='#a89455'; 'h'='#6e5230' } @(
    '................',
    '....hh..........',
    '....hh..........',
    '....nnnnnnnnn...',
    '...nd.nd.nd.n...',
    '...n.nd.nd..n...',
    '...nd.nd.nd.n...',
    '...n.nd.nd..n...',
    '...nd.nd.nd.n...',
    '...n.nd.nd..n...',
    '...nd.nd.nd.n...',
    '....n.nd.nd.n...',
    '....nnnnnnnnn...',
    '................',
    '................',
    '................')
Save-Sprite 'item_tinderbox' @{ 'a'='#8a8a85'; 'b'='#5e5e58'; 'f'='#f8c83c' } @(
    '................',
    '................',
    '................',
    '................',
    '....bbbbbbbbb...',
    '...baaaaaaaaab..',
    '...baaaafaaaab..',
    '...baaaffaaaab..',
    '...baaaaffaaab..',
    '...baaaaaaaaab..',
    '...bbbbbbbbbbb..',
    '................',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'item_bones' @{ 'w'='#e8e4d8'; 'd'='#b8b4a4' } @(
    '................',
    '................',
    '..ww........ww..',
    '..www......www..',
    '...wwww..wwww...',
    '.....wwwwww.....',
    '......wwww......',
    '.......ww.......',
    '......wwww......',
    '.....wwwwww.....',
    '...wwww..wwww...',
    '..www......www..',
    '..ww........ww..',
    '................',
    '................',
    '................')

Write-Host "Generating runecrafting..."

# Rune essence rock: bluish crystal boulder (never depletes)
Save-Sprite 'obj_essence_rock' @{ 'a'='#7a8aa4'; 'b'='#5a6a84'; 'c'='#94a4be'; 'o'='#bef0ff' } @(
    '................',
    '................',
    '....bbbbbb......',
    '..bbacccabbb....',
    '.baccccccaabb...',
    '.bacccoccccab...',
    'bacccccccccaab..',
    'baccoccccoccab..',
    'baccccccccccab..',
    'bacccccoccccaab.',
    'baacccccccccaab.',
    'bbaaccocccaaabb.',
    'bbaaaaaaaaaaabb.',
    '.bbbaaaaaaabbb..',
    '..bbbbbbbbbbb...',
    '................')

# Altars: stone plinth with glowing sigil
function New-Altar([string]$name, [string]$glow, [string]$core) {
    $pal = @{ 's'='#9a9a94'; 'd'='#62625c'; 'g'=$glow; 'w'=$core }
    Save-Sprite $name $pal @(
        '................',
        '..dddddddddddd..',
        '.dssssssssssssd.',
        '.dssggssssggssd.',
        '.dsssggssggsssd.',
        '.dssssgwwgssssd.',
        '.dsssggssggsssd.',
        '.dssggssssggssd.',
        '.ddssssssssssdd.',
        '...dssssssssd...',
        '...dssssssssd...',
        '..ddssssssssdd..',
        '.dddddddddddddd.',
        '.dddddddddddddd.',
        '................',
        '................')
}
New-Altar 'obj_altar_air'   '#7ae0f0' '#e8ffff'
New-Altar 'obj_altar_fire'  '#f08030' '#ffe8a0'
New-Altar 'obj_altar_water' '#4a90e0' '#d0e8ff'
New-Altar 'obj_altar_earth' '#6aa83a' '#dceaa0'
New-Altar 'obj_altar_law'   '#b060e0' '#f0d8ff'
New-Altar 'obj_altar_chaos' '#c83838' '#ffd0d0'

# Item icons
Save-Sprite 'item_essence' @{ 'a'='#aebed8'; 'b'='#8a9ab4'; 'w'='#e8f8ff' } @(
    '................',
    '................',
    '................',
    '......ab........',
    '.....aawb.......',
    '....aawwab......',
    '...aawwaaab.....',
    '...awwaaaab.....',
    '..aawaaawaab....',
    '..awaaaawwab....',
    '..aaaawwwaab....',
    '...baaawaab.....',
    '....bbaabb......',
    '......bb........',
    '................',
    '................')
Save-Sprite 'item_air_rune' @{ 's'='#d8d8d0'; 'd'='#a8a8a0'; 'g'='#48b8d8' } @(
    '................',
    '................',
    '...ssssssssss...',
    '..sssssssssssd..',
    '..ssggggggssss..',
    '..sssssssggsss..',
    '..ssggggggsssd..',
    '..ssgsssssssss..',
    '..ssggggggsssd..',
    '..sssssssggsss..',
    '..ssggggggsssd..',
    '..sssssssssssd..',
    '...dddddddddd...',
    '................',
    '................',
    '................')
Save-Sprite 'item_fire_rune' @{ 's'='#d8d8d0'; 'd'='#a8a8a0'; 'g'='#e85820'; 'y'='#f8c83c'; 'w'='#fff0c0' } @(
    '................',
    '................',
    '...ssssssssss...',
    '..sssssssssssd..',
    '..ssssssgsssss..',
    '..sssssggssssd..',
    '..ssssggygssss..',
    '..sssggyyggssd..',
    '..ssggyyyyggss..',
    '..ssgyywwyygsd..',
    '..ssggyyyyggss..',
    '..ssssggggsssd..',
    '...dddddddddd...',
    '................',
    '................',
    '................')

# Spell projectiles (8x8 glowing bolts)
Save-Sprite 'obj_bolt_air' @{ 'c'='#4aa0d8'; 'w'='#bef0ff' } @(
    '........',
    '..cccc..',
    '.cwwwwc.',
    '.cwwwwc.',
    '.cwwwwc.',
    '..cccc..',
    '........',
    '........')
Save-Sprite 'obj_bolt_fire' @{ 'c'='#e85820'; 'w'='#ffe8a0' } @(
    '........',
    '..cccc..',
    '.cwwwwc.',
    '.cwwwwc.',
    '.cwwwwc.',
    '..cccc..',
    '........',
    '........')

Write-Host "Generating smithing..."

# Furnace: stone dome with glowing mouth
Save-Sprite 'obj_furnace' @{ 'a'='#8a8a84'; 'b'='#62625c'; 'c'='#9c9c96'; 'o'='#f07820'; 'y'='#f8c83c' } @(
    '................',
    '....bbbbbbbb....',
    '...bacccccab....',
    '..baccccccccb...',
    '..bacccccccab...',
    '.baccccccccccb..',
    '.bacccccccccab..',
    '.baccbbbbbccab..',
    '.bacbooooobcab..',
    '.bacbooyoobcab..',
    '.bacbooooobcab..',
    '.bacbbooobbcab..',
    '.baccbbbbbccab..',
    '.bbbbbbbbbbbbb..',
    '.bbbbbbbbbbbbb..',
    '................')

# Anvil on a wooden block
Save-Sprite 'obj_anvil' @{ 'a'='#54545e'; 'b'='#3a3a44'; 'c'='#6e6e78'; 'w'='#6e5230'; 'd'='#553e22' } @(
    '................',
    '................',
    '................',
    '..accccccccca...',
    '..aaaaaaaaaaa...',
    '....baaaaab.....',
    '.....baaab......',
    '.....baaab......',
    '....baaaaab.....',
    '...baaaaaaab....',
    '..wwwwwwwwwww...',
    '..wwwwwwwwwww...',
    '..dwwwwwwwwd....',
    '..dddddddddd....',
    '................',
    '................')

# Metal bars (ingots)
Save-Sprite 'item_bronze_bar' @{ 'b'='#b87f4e'; 'd'='#94633a'; 'h'='#d8a070' } @(
    '................',
    '................',
    '................',
    '................',
    '................',
    '....hhhhhhhh....',
    '...hbbbbbbbbd...',
    '..hbbbbbbbbbbd..',
    '..dbbbbbbbbbbd..',
    '..dddddddddddd..',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')

# Hammer
Save-Sprite 'item_hammer' @{ 'a'='#7a7a84'; 'b'='#54545e'; 'h'='#6e5230'; 'd'='#553e22' } @(
    '................',
    '................',
    '...aaaaaaa......',
    '...aaaaaaab.....',
    '...aaaaaaab.....',
    '...baaaaabb.....',
    '......hd........',
    '......hd........',
    '......hd........',
    '......hd........',
    '......hd........',
    '......hd........',
    '......hd........',
    '......dd........',
    '................',
    '................')

# Bronze sword (iron variant made by recolor below)
Save-Sprite 'item_bronze_sword' @{ 'b'='#b87f4e'; 'd'='#94633a'; 'h'='#553e22'; 'g'='#c8a23c' } @(
    '................',
    '............bb..',
    '...........bbb..',
    '..........bbbd..',
    '.........bbbd...',
    '........bbbd....',
    '.......bbbd.....',
    '......bbbd......',
    '..g..bbbd.......',
    '...gbbbd........',
    '....gbd.........',
    '...ggg..........',
    '..hh.gg.........',
    '.hh...g.........',
    '................',
    '................')

# Bronze armour (iron variants made by recolor below)
$armPal = @{ 'b'='#b87f4e'; 'd'='#94633a'; 'h'='#d8a070' }
Save-Sprite 'item_bronze_helm' $armPal @(
    '................',
    '................',
    '................',
    '.....bbbbbb.....',
    '...bbhhhhhhbb...',
    '..bhhbbbbbbhhb..',
    '..bhbbbbbbbbhb..',
    '..bbbbbbbbbbbb..',
    '..bbb.bbbb.bbb..',
    '..bbbbbbbbbbbb..',
    '..dbbbbbbbbbbd..',
    '...dddddddddd...',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'item_bronze_body' $armPal @(
    '................',
    '................',
    '...bbb..bbb.....',
    '..bhhb..bhhb....',
    '.bhbbbbbbbbhb...',
    '.bbbbbbbbbbbb...',
    '.bbbbbbbbbbbb...',
    '.bbbhbbbbhbbb...',
    '.bbbbbbbbbbbb...',
    '.bbbbbbbbbbbb...',
    '.bdbbbbbbbbdb...',
    '..dbbbbbbbbd....',
    '..dddddddddd....',
    '................',
    '................',
    '................')
Save-Sprite 'item_bronze_shield' $armPal @(
    '................',
    '...bbbbbbbb.....',
    '..bhhhhhhhhb....',
    '..bhbbbbbbhb....',
    '..bbbbbbbbbb....',
    '..bbbhhhhbbb....',
    '..bbbbhhbbbb....',
    '..bbbbbbbbbb....',
    '...bbbbbbbb.....',
    '...bbbbbbbb.....',
    '....bbbbbb......',
    '.....bbbb.......',
    '......bb........',
    '................',
    '................',
    '................')

# Magic gear icons (single tier, blue wizard kit)
$mgPal = @{ 'w'='#3a5a9a'; 'x'='#6a8ad0'; 't'='#6b4a2a'; 'o'='#bef0ff' }
Save-Sprite 'item_staff' $mgPal @(
    '................',
    '.......ww.......',
    '......wxxw......',
    '......wxxw......',
    '.......ww.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '.......tt.......',
    '......tttt......',
    '................',
    '................')
Save-Sprite 'item_wizard_hat' $mgPal @(
    '................',
    '.......ww.......',
    '......wxxw......',
    '......wxxw......',
    '.....wxxxxw.....',
    '.....wxxxxw.....',
    '....wxxxxxxw....',
    '...wxxxxxxxxw...',
    '..wwwwwwwwwwww..',
    '..wwwwwwwwwwww..',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'item_wizard_robe' $mgPal @(
    '................',
    '................',
    '.....wwww.......',
    '....wwwwww......',
    '....wxxxxw......',
    '...wwwwwwww.....',
    '...wxxxxxxw.....',
    '...wxxxxxxw.....',
    '..wwwwwwwwww....',
    '..wxxxxxxxxw....',
    '..wxxxxxxxxw....',
    '.wwwwwwwwwwww...',
    '.wwwwwwwwwwww...',
    '................',
    '................',
    '................')

# ---- worn-equipment overlays (16x24, drawn on top of the player base) ----
# M/D/L use the bronze palette so the iron recolor map applies cleanly.
$eqPal = @{ 'M'='#b87f4e'; 'D'='#94633a'; 'L'='#d8a070' }

Save-Sprite 'eq_bz_helm_d' $eqPal @(
    '................','.....MMMMMM.....','....MLLLLLLM....','....MLLLLLLM....',
    '....MMMMMMMM....','....M......M....','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_helm_u' $eqPal @(
    '................','.....MMMMMM.....','....MLLLLLLM....','....MLLLLLLM....',
    '....MLLLLLLM....','....MLLLLLLM....','....MMMMMMMM....','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_helm_s' $eqPal @(
    '................','.....MMMMMM.....','....MLLLLLLM....','....MLLLLLLM....',
    '....MMMMMM......','....M...........','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................')

Save-Sprite 'eq_bz_body_d' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','...MMMMMMMMMM...','...MLLLLLLLLM...','...MLLLLLLLLM...',
    '...MLLLLLLLLM...','...MLLLLLLLLM...','...MLLLLLLLLM...','....MMMMMMMM....',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_body_u' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','...MMMMMMMMMM...','...MLLLLLLLLM...','...MLLLLLLLLM...',
    '...MLLLLLLLLM...','...MLLLLLLLLM...','...MLLLLLLLLM...','....MMMMMMMM....',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_body_s' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','....MMMMMMMM....','...MLLLLLLLLM...','...MLLLLLLLLM...',
    '...MLLLLLLLLM...','...MLLLLLLLLM...','....MMMMMMMM....','................',
    '................','................','................','................',
    '................','................','................','................')

Save-Sprite 'eq_bz_wep_d' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','...........MM...','...........LL...',
    '...........LL...','...........LL...','...........LL...','...........LL...',
    '...........L....','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_wep_u' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','...........MM...','...........LL...',
    '...........LL...','...........LL...','...........LL...','...........LL...',
    '...........L....','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_wep_s' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','..........M.....',
    '.........MMLLLLL','..........M.....','................','................',
    '................','................','................','................',
    '................','................','................','................')

Save-Sprite 'eq_bz_shd_d' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','..MMMM..........','..MLLLM.........',
    '..MLLLM.........','..MLLLM.........','...MLM..........','....M...........',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_shd_u' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','..MMMM..........','..MLLLM.........',
    '..MLLLM.........','..MLLLM.........','...MLM..........','....M...........',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_bz_shd_s' $eqPal @(
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','...MMMM.........',
    '...MLLLM........','...MLLLM........','....MMM.........','................',
    '................','................','................','................',
    '................','................','................','................')

Write-Host "Generating shops..."

# Coin pile icon
Save-Sprite 'item_coins' @{ 'g'='#c8a23c'; 'y'='#f8d86c'; 'd'='#8a6a1c' } @(
    '................',
    '................',
    '................',
    '......gggg......',
    '.....gyyyyg.....',
    '.....gyyyyg.....',
    '.....dggggd.....',
    '...gggg..gggg...',
    '..gyyyyggyyyyg..',
    '..gyyyyggyyyyg..',
    '..dggggddggggd..',
    '................',
    '................',
    '................',
    '................',
    '................')

# Market stall (awning 'a' recolored per shop)
Save-Sprite 'obj_stall_general' @{ 'a'='#8a4a2a'; 'b'='#f0ece0'; 'w'='#6b4a2a'; 'c'='#a07840'; 'g'='#c8a23c' } @(
    '..aaaaaaaaaaaa..',
    '..abbaabbaabba..',
    '..aaaaaaaaaaaa..',
    '..cccccccccccc..',
    '..cggggggggggc..',
    '..cggggggggggc..',
    '..cccccccccccc..',
    '..w..........w..',
    '..w..........w..',
    '..w..........w..',
    '..w..........w..',
    '................',
    '................',
    '................',
    '................',
    '................')

Write-Host "Generating dungeon..."

# Cave floor: dark stone noise
New-NoiseTile 'tile_cave' @{ 'a'='#3a3a42'; 'b'='#2c2c34'; 'c'='#46464f' } 'a' @(,@('b',26),@('c',14)) 5005

# Stairs down: a framed pit with descending steps
Save-Sprite 'obj_stairs_down' @{ 's'='#7d7d78'; 'd'='#4a4a48'; 'k'='#15151a' } @(
    '................',
    '..ssssssssssss..',
    '..s.ssssssss.s..',
    '..s.dddddddd.s..',
    '..s.ssssssss.s..',
    '..s.dddddddd.s..',
    '..s.kkkkkkkk.s..',
    '..s.kkkkkkkk.s..',
    '..ssssssssssss..',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')
# Stairs up: steps rising toward light
Save-Sprite 'obj_stairs_up' @{ 's'='#7d7d78'; 'c'='#9a9a94'; 'w'='#d8d8d0' } @(
    '................',
    '..ssssssssssss..',
    '..s.wwwwwwww.s..',
    '..s.cccccccc.s..',
    '..s.ssssssss.s..',
    '..s.cccccccc.s..',
    '..s.ssssssss.s..',
    '..s.cccccccc.s..',
    '..ssssssssssss..',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')

# Skeleton: 2 frames
$skPal = @{ 'w'='#e8e4d8'; 'd'='#b8b4a4'; 'k'='#1a1a1a' }
Save-Sprite 'skeleton_a' $skPal @(
    '................',
    '.....wwww.......',
    '....wwwwww......',
    '....wkwwkw......',
    '....wwwwww......',
    '.....wwww.......',
    '......ww........',
    '...wwwwwwww.....',
    '...w.wwww.w.....',
    '.....wwww.......',
    '....wwwwww......',
    '.....wddw.......',
    '.....w..w.......',
    '.....w..w.......',
    '....ww..ww......',
    '................')
Save-Sprite 'skeleton_b' $skPal @(
    '................',
    '.....wwww.......',
    '....wwwwww......',
    '....wkwwkw......',
    '....wwwwww......',
    '.....wwww.......',
    '......ww........',
    '....wwwwww......',
    '...w.wwww.w.....',
    '...w.wwww.w.....',
    '....wwwwww......',
    '.....wddw.......',
    '....ww..ww......',
    '....w....w......',
    '...ww....ww.....',
    '................')

# Demon: floor-2 boss (24x24, single frame, red with horns and fangs)
$dmPal = @{ 'r'='#b03028'; 'd'='#7a1c18'; 'e'='#f8d030'; 'w'='#e8e4d8'; 'k'='#1a1a1a'; 'h'='#9a9088'; 'l'='#4a1a16' }
Save-Sprite 'demon' $dmPal @(
    '......h..........h......',
    '......h..........h......',
    '......hrrrrrrrrrrh......',
    '.......rrrrrrrrrr.......',
    '.......rddddddddr.......',
    '.......reeddddeer.......',
    '.......rrwwwwwwrr.......',
    '........rkkkkkkr........',
    '....dddrrrrrrrrrrddd....',
    '...drrrrrrrrrrrrrrrrd...',
    '...rrrrrrrrrrrrrrrrrr...',
    '...rrrrrddrrrrddrrrrr...',
    '...rrrrrrrrrrrrrrrrrr...',
    '...rrrrrrrrrrrrrrrrrr...',
    '....llllllllllllllll....',
    '....rrrrrr....rrrrrr....',
    '....rrrrrr....rrrrrr....',
    '....rrrrrr....rrrrrr....',
    '....rrrrr......rrrrr....',
    '....rrrr......rrrr......',
    '...kkkkk......kkkkk.....',
    '........................',
    '........................',
    '........................')

# Boss: Goblin Warlord (24x24, single frame, hulking and green)
$bsPal = @{ 'g'='#5a8a32'; 'd'='#46702a'; 'e'='#c83c1e'; 'w'='#e8e4d8'; 'k'='#1a1a1a'; 'l'='#6b4a2a' }
Save-Sprite 'boss' $bsPal @(
    '........................',
    '........gggggggg........',
    '.......gggggggggg.......',
    '.....gg.gggggggg.gg.....',
    '.....gg.gggggggg.gg.....',
    '.......gggggggggg.......',
    '.......gddddddddg.......',
    '.......geeggggeeg.......',
    '.......ggwwwwwwgg.......',
    '........gkkkkkkg........',
    '....dddggggggggggddd....',
    '...dgggggggggggggggd....',
    '...gggggggggggggggggg...',
    '...gggggddgggggddgggg...',
    '...gggggggggggggggggg...',
    '...gggggggggggggggggg...',
    '....llllllllllllllll....',
    '....gggggg....gggggg....',
    '....gggggg....gggggg....',
    '....gggggg....gggggg....',
    '....ggggg......ggggg....',
    '....gggg......gggg......',
    '...kkkkk......kkkkk.....',
    '........................')

Write-Host "Generating bot recolors..."

# Palette-swap the player sprites into bot variants (tunic + hair)
function New-Recolor([string]$srcName, [string]$dstName, [hashtable]$map) {
    $srcPath = Join-Path $outDir "$srcName.png"
    $src = New-Object System.Drawing.Bitmap($srcPath)
    $out = New-Object System.Drawing.Bitmap($src.Width, $src.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $src.Height; $y++) {
        for ($x = 0; $x -lt $src.Width; $x++) {
            $c = $src.GetPixel($x, $y)
            $key = ('#{0:x2}{1:x2}{2:x2}' -f $c.R, $c.G, $c.B)
            if ($c.A -gt 0 -and $map.ContainsKey($key)) {
                $hex = $map[$key]
                $c = [System.Drawing.Color]::FromArgb($c.A,
                    [Convert]::ToInt32($hex.Substring(1,2),16),
                    [Convert]::ToInt32($hex.Substring(3,2),16),
                    [Convert]::ToInt32($hex.Substring(5,2),16))
            }
            $out.SetPixel($x, $y, $c)
        }
    }
    $src.Dispose()
    $out.Save((Join-Path $outDir "$dstName.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $out.Dispose()
    Write-Host "  $dstName"
}

$botSets = @{
    'bota' = @{ '#3e5e7e' = '#8e3e3e'; '#4a3320' = '#2a2a2a' }   # red tunic, black hair
    'botb' = @{ '#3e5e7e' = '#6e468e'; '#4a3320' = '#c8a23c' }   # purple tunic, blond hair
    'botc' = @{ '#3e5e7e' = '#b8703e'; '#4a3320' = '#8e5e3e' }   # orange tunic, auburn hair
}
$parts = 'down_a','down_b','up_a','up_b','side_a','side_b'
foreach ($set in 'bota','botb','botc') {
    foreach ($p in $parts) {
        New-Recolor "pl_$p" "${set}_$p" $botSets[$set]
    }
}

# Chef NPC: white apron + chef hat (white "hair")
New-Recolor 'pl_down_a' 'chef_down_a' @{ '#3e5e7e' = '#f0eee8'; '#4a3320' = '#f8f8f4' }
# Knight NPC: steel plate body + helm (grey)
New-Recolor 'pl_down_a' 'knight_down_a' @{ '#3e5e7e' = '#8a92a0'; '#4a3320' = '#c0c8d4' }
# Wight: an undead-green recolour of the skeleton (floor-2 fodder)
New-Recolor 'skeleton_a' 'wight_a' @{ '#e8e4d8' = '#8aa89a'; '#b8b4a4' = '#5e7a6c' }
New-Recolor 'skeleton_b' 'wight_b' @{ '#e8e4d8' = '#8aa89a'; '#b8b4a4' = '#5e7a6c' }

# Water/earth/law runes: recolour the air rune's glyph to each element
New-Recolor 'item_air_rune' 'item_water_rune' @{ '#48b8d8' = '#3a78d0' }
New-Recolor 'item_air_rune' 'item_earth_rune' @{ '#48b8d8' = '#5a9a3a' }
New-Recolor 'item_air_rune' 'item_law_rune'   @{ '#48b8d8' = '#b050e0' }
New-Recolor 'item_air_rune' 'item_chaos_rune' @{ '#48b8d8' = '#9a3060' }

# Cow: placid training fodder. Side view, head left, 16x16, 2 frames (legs shuffle)
$cowPal = @{ 'w'='#f0ece0'; 'p'='#3a342e'; 'k'='#1c1813'; 'n'='#d98a8a'; 'h'='#d8c49a'; 'e'='#1c1813' }
Save-Sprite 'mob_cow_a' $cowPal @(
    '................',
    '................',
    '..hh........kk..',
    '.kwwwwwwwwwwwk..',
    'nwewwwwwwwwwwk..',
    'nwppwwwwwppwwk..',
    '.wwwwwwwwwwwwk..',
    '.wwppwwwwwppww..',
    '..wwwwwwwwwww...',
    '..wwwwwwwwwww...',
    '..k.kk...kk.k...',
    '..k.kk...kk.k...',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'mob_cow_b' $cowPal @(
    '................',
    '................',
    '..hh........kk..',
    '.kwwwwwwwwwwwk..',
    'nwewwwwwwwwwwk..',
    'nwppwwwwwppwwk..',
    '.wwwwwwwwwwwwk..',
    '.wwppwwwwwppww..',
    '..wwwwwwwwwww...',
    '..wwwwwwwwwww...',
    '..kk.k...k.kk...',
    '..kk.k...k.kk...',
    '................',
    '................',
    '................',
    '................')

# Post-and-rail fence: encloses the cow pasture (solid object)
$fencePal = @{ 'w'='#9a6a3a'; 'd'='#6e4a26' }
Save-Sprite 'obj_fence' $fencePal @(
    '................',
    '.d..d..d..d..d..',
    '.w..w..w..w..w..',
    'wwwwwwwwwwwwwwww',
    '.w..w..w..w..w..',
    '.w..w..w..w..w..',
    'wwwwwwwwwwwwwwww',
    '.w..w..w..w..w..',
    '.d..d..d..d..d..',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................')

# Combat Tutor (Sergeant Hardy): red tunic + bronze helm recolour of the player
New-Recolor 'pl_down_a' 'obj_tutor' @{ '#3e5e7e' = '#8a3a2a'; '#4a3320' = '#b8923a' }

# Whelp: small red dragon, floor-3 fodder + the Dragon's brood (16x16, 2 frames)
$whelpPal = @{ 'r'='#c0392b'; 'd'='#7a241b'; 'k'='#1a1410'; 'e'='#f5d020'; 'o'='#8a3a1a' }
Save-Sprite 'mob_whelp_a' $whelpPal @(
    '.k............k.',
    '.rr..........rr.',
    '.rrr.rrrrrr.rrr.',
    '.rrrrrrrrrrrrrr.',
    '..rrdrrrrrrdrr..',
    '...rekrrrrekr...',
    '...rrrrrrrrrr...',
    '....rrdddrrr....',
    '.....rrrrrr.....',
    '...rrrrrrrrrr...',
    '..rr.rrrrrr.rr..',
    '.....oooooo.....',
    '.....oooooo.....',
    '....rrr..rrr....',
    '....rr....rr....',
    '................')
Save-Sprite 'mob_whelp_b' $whelpPal @(
    '.k............k.',
    '.rr..........rr.',
    '.rrr.rrrrrr.rrr.',
    '.rrrrrrrrrrrrrr.',
    '..rrdrrrrrrdrr..',
    '...rekrrrrekr...',
    '...rrrrrrrrrr...',
    '....rrdddrrr....',
    '.....rrrrrr.....',
    '...rrrrrrrrrr...',
    '..rr.rrrrrr.rr..',
    '.....oooooo.....',
    '.....oooooo.....',
    '...rrr....rrr...',
    '...rr......rr...',
    '................')

# Ancient Dragon: the floor-3 boss (24x24). Winged, horned, fire-breathing
$dragonPal = @{ 'r'='#b8321f'; 'd'='#6e1f12'; 'w'='#e0742a'; 'k'='#1a1410'; 'e'='#f5d020'; 'o'='#3a1208' }
Save-Sprite 'mob_dragon' $dragonPal @(
    '........k......k........',
    '........kk....kk........',
    '.......krrrrrrrrk.......',
    '......rrrrrrrrrrrr......',
    '......rrerrrrrrerr......',
    '......rrrrrrrrrrrr......',
    '.......rroooorr.........',
    '........rrrrrr..........',
    '.........rrrr...........',
    '...ww....rrrrrr....ww...',
    '..wwww..rrrrrrrr..wwww..',
    '.wwwwww.rrrrrrrr.wwwwww.',
    'wwwwwww.rrrrrrrr.wwwwwww',
    '.wwwww..rrrrrrrr..wwwww.',
    '..www...rrrrrrrr...www..',
    '........rrrrrrrr........',
    '........rrddddrr........',
    '.........rrrrrr.........',
    '........rr....rr........',
    '........rr....rr........',
    '.......kkk..kkk.........',
    '........................',
    '........................',
    '........................')

# Dragonstone: the Dragon's lucrative trophy gem (16x16)
$gemPal = @{ 'r'='#d83a2a'; 'd'='#8a1e10'; 'k'='#1a1a1a'; 'w'='#ffd0a0' }
Save-Sprite 'item_dragonstone' $gemPal @(
    '................',
    '......kkkk......',
    '.....kwwrrk.....',
    '....kwrrrrrk....',
    '...krrrrrrrrk...',
    '...krrrrrrrrk...',
    '...kdrrrrrrdk...',
    '....kdrrrrdk....',
    '.....kdrrdk.....',
    '......kddk......',
    '.......kk.......',
    '................',
    '................',
    '................',
    '................',
    '................')

# Left-facing variants: copy-mode blits cannot mirror (reversed S range is
# undefined on real RDP hardware), so bake flipped sprites instead
foreach ($set in 'pl','bota','botb','botc') {
    foreach ($p in @{ side_a = 'side_la'; side_b = 'side_lb' }.GetEnumerator()) {
        $src = New-Object System.Drawing.Bitmap((Join-Path $outDir "${set}_$($p.Key).png"))
        $flipped = New-Object System.Drawing.Bitmap($src)
        $src.Dispose()
        $flipped.RotateFlip([System.Drawing.RotateFlipType]::RotateNoneFlipX)
        $flipped.Save((Join-Path $outDir "${set}_$($p.Value).png"), [System.Drawing.Imaging.ImageFormat]::Png)
        $flipped.Dispose()
        Write-Host "  ${set}_$($p.Value)"
    }
}

# Iron-tier recolors of bronze gear
$ironMap = @{ '#b87f4e' = '#a8a8b2'; '#94633a' = '#83838d'; '#d8a070' = '#c8c8d2' }
New-Recolor 'item_axe'          'item_iron_axe'   $ironMap
New-Recolor 'item_pickaxe'      'item_iron_pick'  $ironMap
New-Recolor 'item_bronze_bar'   'item_iron_bar'   $ironMap
New-Recolor 'item_bronze_sword' 'item_iron_sword' $ironMap
New-Recolor 'item_bronze_helm'  'item_iron_helm'  $ironMap
New-Recolor 'item_bronze_body'  'item_iron_body'  $ironMap
New-Recolor 'item_bronze_shield' 'item_iron_shield' $ironMap

# Equipment overlays: iron recolors + baked left-facing (side) flips
function New-FlipX([string]$srcName, [string]$dstName) {
    $src = New-Object System.Drawing.Bitmap((Join-Path $outDir "$srcName.png"))
    $f = New-Object System.Drawing.Bitmap($src)
    $src.Dispose()
    $f.RotateFlip([System.Drawing.RotateFlipType]::RotateNoneFlipX)
    $f.Save((Join-Path $outDir "$dstName.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $f.Dispose()
    Write-Host "  $dstName"
}
foreach ($slot in 'helm','body','wep','shd') {
    foreach ($dir in 'd','u','s') {
        New-Recolor "eq_bz_${slot}_$dir" "eq_ir_${slot}_$dir" $ironMap
    }
    New-FlipX "eq_bz_${slot}_s" "eq_bz_${slot}_sl"
    New-FlipX "eq_ir_${slot}_s" "eq_ir_${slot}_sl"
}

# Magic gear overlays (single tier). Hat and robe are symmetric so all four
# facings share one design; the staff differs per facing.
$ovPal = @{ 'w'='#3a5a9a'; 'x'='#6a8ad0'; 't'='#6b4a2a'; 'o'='#bef0ff' }
$hatGrid = @(
    '.......ww.......','......wxxw......','.....wxxxxw.....','....wxxxxxxw....',
    '...wwwwwwwwww...','................','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................',
    '................','................','................','................')
$robeGrid = @(
    '................','................','................','................',
    '................','................','................','................',
    '................','...wwwwwwwwww...','..wxxxxxxxxxxw..','..wxxxxxxxxxxw..',
    '..wxxxxxxxxxxw..','..wxxxxxxxxxxw..','..wwxxxxxxxxww..','...wxxxxxxxxw...',
    '...wxxxxxxxxw...','...wwwwwwwwww...','................','................',
    '................','................','................','................')
foreach ($d in 'd','u','s','sl') {
    Save-Sprite "eq_hat_$d"  $ovPal $hatGrid
    Save-Sprite "eq_robe_$d" $ovPal $robeGrid
}
$staffD = @(
    '................','................','................','................',
    '................','............o...','...........oxo..','............o...',
    '............t...','............t...','............t...','............t...',
    '............t...','............t...','............t...','............t...',
    '............t...','................','................','................',
    '................','................','................','................')
Save-Sprite 'eq_staff_d' $ovPal $staffD
Save-Sprite 'eq_staff_u' $ovPal $staffD
Save-Sprite 'eq_staff_s' $ovPal @(
    '................','................','................','................',
    '................','..........o.....','.........oxo....','..........o.....',
    '..........t.....','..........t.....','..........t.....','..........t.....',
    '..........t.....','..........t.....','..........t.....','..........t.....',
    '..........t.....','................','................','................',
    '................','................','................','................')
New-FlipX 'eq_staff_s' 'eq_staff_sl'

# Shop stalls: recolor the awning per shop type
New-Recolor 'obj_stall_general' 'obj_stall_weapon' @{ '#8a4a2a' = '#b03030' }
New-Recolor 'obj_stall_general' 'obj_stall_armor'  @{ '#8a4a2a' = '#3a6a9a' }
New-Recolor 'obj_stall_general' 'obj_stall_magic'  @{ '#8a4a2a' = '#7a3a9a' }

# Higher gear tiers: recolor the bronze icons and overlays (b/d/h -> tier hues)
$gearTiers = @{
    'st' = @{ '#b87f4e'='#b8c0cc'; '#94633a'='#8a929e'; '#d8a070'='#dce4ec' }  # steel
    'mi' = @{ '#b87f4e'='#4a5ac0'; '#94633a'='#36429a'; '#d8a070'='#7a8ae0' }  # mithril
    'ru' = @{ '#b87f4e'='#2aa89a'; '#94633a'='#1c8076'; '#d8a070'='#6ad8c8' }  # rune
}
foreach ($t in 'st','mi','ru') {
    $map = $gearTiers[$t]
    New-Recolor 'item_bronze_sword'  "item_${t}_sword"  $map
    New-Recolor 'item_bronze_helm'   "item_${t}_helm"   $map
    New-Recolor 'item_bronze_shield' "item_${t}_shield" $map
    New-Recolor 'item_bronze_body'   "item_${t}_body"   $map
    foreach ($slot in 'helm','body','wep','shd') {
        foreach ($dir in 'd','u','s') {
            New-Recolor "eq_bz_${slot}_$dir" "eq_${t}_${slot}_$dir" $map
        }
        New-FlipX "eq_${t}_${slot}_s" "eq_${t}_${slot}_sl"
    }
}

# Warlord's Bane: a legendary gold sword (quest reward), icon + worn overlay
$baneMap = @{ '#b87f4e'='#f0d050'; '#94633a'='#c0a030'; '#d8a070'='#fff0a0' }
New-Recolor 'item_bronze_sword' 'item_bane' $baneMap
foreach ($dir in 'd','u','s') { New-Recolor "eq_bz_wep_$dir" "eq_bane_wep_$dir" $baneMap }
New-FlipX 'eq_bane_wep_s' 'eq_bane_wep_sl'

# Dragonfire blade: the Dragon's rare unique, a molten-orange sword (icon + worn)
$dfMap = @{ '#b87f4e'='#e0641e'; '#94633a'='#a83410'; '#d8a070'='#ffc050' }
New-Recolor 'item_bronze_sword' 'item_dragonfire' $dfMap
foreach ($dir in 'd','u','s') { New-Recolor "eq_bz_wep_$dir" "eq_df_wep_$dir" $dfMap }
New-FlipX 'eq_df_wep_s' 'eq_df_wep_sl'

# ---- Coal: a dark ore that fuels steel and mithril smelting ----
New-Rock    'obj_rock_coal' '#161616'
New-OreIcon 'item_coal'     '#101010'
New-Recolor 'item_bronze_bar' 'item_steel_bar' $gearTiers['st']

# ---- Gold: a yellow ore smelted into bars that mount dragonstone jewelry ----
New-Rock    'obj_rock_gold' '#e8c84a'
New-OreIcon 'item_gold_ore' '#e8c84a'
New-Recolor 'item_bronze_bar' 'item_gold_bar' @{ '#b87f4e'='#e8c84a'; '#94633a'='#b89818'; '#d8a070'='#f8e87a' }

# ---- Dragonstone jewelry: gold settings (g) + a violet gem (p/q) ----
$jewPal = @{ 'g'='#e8c84a'; 'p'='#b04ad0'; 'q'='#7a2a9a' }
Save-Sprite 'item_dstone_ring' $jewPal @(
    '................','................','.......p........','......qpq.......',
    '.......p........','.....gg.gg......','....g.....g.....','....g.....g.....',
    '....g.....g.....','.....g...g......','......ggg.......','................',
    '................','................','................','................')
Save-Sprite 'item_dstone_bracelet' $jewPal @(
    '................','................','................','....gggggg......',
    '...g......g.....','..g..qpq...g....','...g......g.....','....gggggg......',
    '................','................','................','................',
    '................','................','................','................')
Save-Sprite 'item_dstone_amulet' $jewPal @(
    '................','....g.....g.....','.....g...g......','......g.g.......',
    '.......g........','.......p........','......pqp.......','.....pqpqp......',
    '......pqp.......','.......p........','................','................',
    '................','................','................','................')
Save-Sprite 'item_dstone_necklace' $jewPal @(
    '................','...g.......g....','....g.....g.....','.....g...g......',
    '......ggg.......','......qpq.......','.....qpppq......','......qpq.......',
    '................','................','................','................',
    '................','................','................','................')
# enchanted: the gem flares brighter (and the gold warms)
$enchMap = @{ '#b04ad0'='#f08aff'; '#7a2a9a'='#c45ae0'; '#e8c84a'='#fff070' }
New-Recolor 'item_dstone_ring'     'item_fury_ring'      $enchMap
New-Recolor 'item_dstone_bracelet' 'item_guard_bracelet' $enchMap
New-Recolor 'item_dstone_amulet'   'item_glory_amulet'   $enchMap
New-Recolor 'item_dstone_necklace' 'item_power_necklace' $enchMap

# ---- Mithril: a deep-blue ore deeper in the mine, smithed into bars/gear ----
$mithFleck = '#4a5ac0'
New-Rock    'obj_rock_mithril' $mithFleck
New-OreIcon 'item_mith_ore'    $mithFleck
New-Recolor 'item_bronze_bar'  'item_mith_bar' $gearTiers['mi']
New-TipIcon   'item_mith_tips'  '#7a8ae0'
New-ArrowIcon 'item_mith_arrow' '#7a8ae0'

# ---- Crafting: hides + leather, needle + thread ----
function New-HideIcon([string]$name, [string]$body, [string]$dark) {
    Save-Sprite $name @{ 'p'=$body; 'q'=$dark } @(
        '................',
        '................',
        '....pppppp......',
        '...pqppppqp.....',
        '..ppppppppp.....',
        '..pppppppppp....',
        '..pppppppppp....',
        '..qppppppppp....',
        '...pppppppp.....',
        '...pppppppq.....',
        '....pppppp......',
        '.....qppq.......',
        '................',
        '................',
        '................',
        '................')
}
New-HideIcon 'item_cowhide'        '#b8946a' '#8a6a44'
New-HideIcon 'item_leather'        '#c89c64' '#9a7038'
New-HideIcon 'item_dragon_hide'    '#5a8a46' '#3a6030'
New-HideIcon 'item_dragon_leather' '#4a7038' '#2e5024'

Save-Sprite 'item_needle' @{ 'n'='#d0d0d8'; 'e'='#9098a0'; 'v'='#f0f0f8' } @(
    '.......e........',
    '......e.e.......',
    '......e.e.......',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......n........',
    '.......v........',
    '................',
    '................',
    '................',
    '................')
Save-Sprite 'item_thread' @{ 's'='#b07840'; 't'='#e0d8a0' } @(
    '................',
    '................',
    '....ssssssss....',
    '....s......s....',
    '....tttttttt....',
    '....tttttttt....',
    '....tttttttt....',
    '....tttttttt....',
    '....tttttttt....',
    '....s......s....',
    '....ssssssss....',
    '................',
    '................',
    '................',
    '................',
    '................')

# ---- Ranged armour: brown leather and green dragonhide (icons + worn) ----
$leatherMap = @{ '#b87f4e'='#9c6b3e'; '#94633a'='#6b4526'; '#d8a070'='#c49464' }
$dhideMap   = @{ '#b87f4e'='#4a7a3a'; '#94633a'='#356029'; '#d8a070'='#6ea84e' }
New-Recolor 'item_bronze_helm' 'item_leather_coif' $leatherMap
New-Recolor 'item_bronze_body' 'item_leather_body' $leatherMap
New-Recolor 'item_bronze_helm' 'item_dhide_coif'   $dhideMap
New-Recolor 'item_bronze_body' 'item_dhide_body'   $dhideMap
foreach ($pair in @(@('le',$leatherMap), @('dh',$dhideMap))) {
    $tag = $pair[0]; $map = $pair[1]
    foreach ($slot in 'helm','body') {
        foreach ($dir in 'd','u','s') {
            New-Recolor "eq_bz_${slot}_$dir" "eq_${tag}_${slot}_$dir" $map
        }
        New-FlipX "eq_${tag}_${slot}_s" "eq_${tag}_${slot}_sl"
    }
}

# ---- Tanner NPC: a leather-aproned craftsman by the pasture ----
New-Recolor 'pl_down_a' 'tanner_down_a' @{ '#3e5e7e' = '#8a5a30'; '#4a3320' = '#5a3a1e' }

# ======== Frostmere: the frozen second area ========
Write-Host "Generating Frostmere..."
# snow ground + frozen-lake ice tiles
$snowPal = @{ 'a'='#dfe7f0'; 'b'='#ccd8e8'; 'c'='#eef4fb'; 'd'='#b8c8dc' }
New-NoiseTile 'tile_snow' $snowPal 'a' @(,@('b',20),@('c',14),@('d',6)) 5101
$icePal = @{ 'a'='#a9c8e0'; 'b'='#bcd8ec'; 'c'='#90b4d4' }
New-NoiseTile 'tile_ice' $icePal 'a' @(,@('b',16),@('c',10)) 5102
# snow-dusted pine (recolor the tree's foliage to dark evergreen + snow tips)
New-Recolor 'obj_tree' 'obj_pine' @{ '#2e5c20'='#1e4a30'; '#3f7a2c'='#2e6040'; '#4f9438'='#cfe0ee' }
# ice warrior: a frost-blue skeleton
New-Recolor 'skeleton_a' 'mob_icew_a' @{ '#e8e4d8'='#bcdcf4'; '#b8b4a4'='#5e88b8' }
New-Recolor 'skeleton_b' 'mob_icew_b' @{ '#e8e4d8'='#bcdcf4'; '#b8b4a4'='#5e88b8' }
# Yeti: a white-and-ice recolor of the demon (24x24 mini-boss)
New-Recolor 'demon' 'mob_yeti' @{ '#b03028'='#e0eaf4'; '#7a1c18'='#a8bcd0'; '#f8d030'='#5ac8e8'; '#9a9088'='#c8d4e0'; '#4a1a16'='#7088a0' }
# frost wolf: a small grey quadruped, two walk frames
$wolfPal = @{ 'w'='#9aa0a8'; 'd'='#6a7078'; 'l'='#33373c' }
Save-Sprite 'mob_wolf_a' $wolfPal @(
    '................','................','................','............dd..',
    '..ddd......dwwd.','.dwwwwwwwwwwwwl.','.dwwwwwwwwwwwwd.','.dwwwwwwwwwwwwd.',
    '..d.d....d.d....','..d.d....d.d....','................','................',
    '................','................','................','................')
Save-Sprite 'mob_wolf_b' $wolfPal @(
    '................','................','................','............dd..',
    '..ddd......dwwd.','.dwwwwwwwwwwwwl.','.dwwwwwwwwwwwwd.','.dwwwwwwwwwwwwd.',
    '...d.d..d.d.....','...d.d..d.d.....','................','................',
    '................','................','................','................')
# the boat (dock travel)
Save-Sprite 'obj_boat' @{ 'h'='#9a6a3a'; 's'='#eef0ea'; 'm'='#5a3c20' } @(
    '................','................','.......m........','.......m........',
    '......sss.......','.....ssss.......','......sss.......','......mmm.......',
    '..hhhhhhhhhhh...','..hhhhhhhhhhh...','...hhhhhhhhh....','....hhhhhhh.....',
    '................','................','................','................')
# pine logs (recolour ordinary logs evergreen)
New-Recolor 'item_logs' 'item_pine_logs' @{ '#8a6a40'='#5a7048'; '#6e5230'='#42583a'; '#b89468'='#86a06e'; '#5a4226'='#33402a' }
# Frostmaul: an icy maul icon + worn overlay (recolour the bronze weapon)
Save-Sprite 'item_frostmaul' @{ 'I'='#a9d8f0'; 'j'='#6ab0d8'; 'w'='#6b4a2a' } @(
    '................','................','....IIIIII......','...IjIIIIjI.....',
    '...IIIIIIII.....','....IIIIII......','......ww........','......ww........',
    '......ww........','......ww........','......ww........','.....wwww.......',
    '................','................','................','................')
$fmMap = @{ '#b87f4e'='#a9d8f0'; '#94633a'='#6ab0d8'; '#d8a070'='#d8f0ff' }
foreach ($dir in 'd','u','s') { New-Recolor "eq_bz_wep_$dir" "eq_fm_wep_$dir" $fmMap }
New-FlipX 'eq_fm_wep_s' 'eq_fm_wep_sl'

# bigger bones: warm ivory (big) and cold blue-grey (dragon), recolours of bones
New-Recolor 'item_bones' 'item_big_bones'    @{ '#e8e4d8'='#f2ecd6'; '#b8b4a4'='#cabf94' }
New-Recolor 'item_bones' 'item_dragon_bones' @{ '#e8e4d8'='#bcd0dc'; '#b8b4a4'='#5e7e92' }

# combat potions: one corked flask, five liquids (L = recoloured per potion)
Write-Host "Generating potions..."
$potRows = @(
    '................',
    '......cccc......',
    '......cccc......',
    '......oooo......',
    '......oggo......',
    '.....oggggo.....',
    '....ogLLLLgo....',
    '...ogLLLLLLgo...',
    '..ogLLLLLLLLgo..',
    '..oLLLLLLLLLLo..',
    '..oLLLLLLLLLLo..',
    '..oLLLLLLLLLLo..',
    '..ogLLLLLLLLgo..',
    '...oLLLLLLLLo...',
    '....oooooooo....',
    '................')
$potBase = @{ 'c'='#8a5a2b'; 'o'='#33323f'; 'g'='#cfe6ef' }
Save-Sprite 'item_pot_attack'   ($potBase + @{ 'L'='#d23a3a' }) $potRows
Save-Sprite 'item_pot_strength' ($potBase + @{ 'L'='#3ab24a' }) $potRows
Save-Sprite 'item_pot_defence'  ($potBase + @{ 'L'='#3a6ad2' }) $potRows
Save-Sprite 'item_pot_combat'   ($potBase + @{ 'L'='#e08a1e' }) $potRows
Save-Sprite 'item_pot_prayer'   ($potBase + @{ 'L'='#e6d24a' }) $potRows

Write-Host "Generating title logo..."

# 256 wide: CI4 TMEM pitch must stay 8-byte aligned (280 wide = 140B = broken)
$lw = 256; $lh = 56
$bmp = New-Object System.Drawing.Bitmap($lw, $lh, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
$font = New-Object System.Drawing.Font('Arial Black', 22, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
$fmt = New-Object System.Drawing.StringFormat
$fmt.Alignment = [System.Drawing.StringAlignment]::Center
$fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
$shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 40, 28, 12))
$gold   = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 248, 200, 60))
$g.DrawString('RUNE VALLEY 64', $font, $shadow, (New-Object System.Drawing.RectangleF(3, 3, $lw, $lh)), $fmt)
$g.DrawString('RUNE VALLEY 64', $font, $gold,   (New-Object System.Drawing.RectangleF(0, 0, $lw, $lh)), $fmt)
$g.Dispose()
# clean the GDI output to strict binary pixels (transparent / gold / shadow):
# stray partial-alpha pixels from text rendering produce corrupt sprites
$clean = New-Object System.Drawing.Bitmap($lw, $lh, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
for ($y = 0; $y -lt $lh; $y++) {
    for ($x = 0; $x -lt $lw; $x++) {
        $p = $bmp.GetPixel($x, $y)
        if ($p.A -lt 128) {
            $clean.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0))
        } elseif ($p.R -gt 120) {
            $clean.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 248, 200, 60))
        } else {
            $clean.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 40, 28, 12))
        }
    }
}
$bmp.Dispose()
$bmp = $clean

# dice into 16 small tiles (4x4 grid of 64x14) so each sprite is a tiny,
# comfortably TMEM-sized texture like every other sprite in the game
for ($i = 0; $i -lt 16; $i++) {
    $r = [int][math]::Floor($i / 4); $c = $i % 4
    $tile = New-Object System.Drawing.Bitmap(64, 14, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $sg = [System.Drawing.Graphics]::FromImage($tile)
    $sg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $sg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $dst = New-Object System.Drawing.Rectangle(0, 0, 64, 14)
    $src = New-Object System.Drawing.Rectangle(($c * 64), ($r * 14), 64, 14)
    $sg.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
    $sg.Dispose()
    $tile.Save((Join-Path $outDir ("ui_logo_{0:d2}.png" -f $i)), [System.Drawing.Imaging.ImageFormat]::Png)
    $tile.Dispose()
}
Write-Host "  ui_logo_00..15 (16 tiles of 64x14)"
$bmp.Dispose()

Write-Host "Done. PNGs in $outDir"
