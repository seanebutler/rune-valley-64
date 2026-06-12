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
New-Altar 'obj_altar_air'  '#7ae0f0' '#e8ffff'
New-Altar 'obj_altar_fire' '#f08030' '#ffe8a0'

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

Write-Host "Done. PNGs in $outDir"
