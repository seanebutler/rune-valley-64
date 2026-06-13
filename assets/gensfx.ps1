# Generates all audio (SFX + music loop) for Rune Valley 64 as 16-bit mono WAVs.
# Uses an embedded C# synth for speed; original compositions, OSRS-inspired mood.
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @"
using System;
using System.IO;

public class Synth {
    public int Rate;
    public float[] Buf;
    Random rng = new Random(1234);

    public Synth(int rate, double seconds) {
        Rate = rate;
        // length must stay a multiple of 16 samples: VADPCM loop requirement
        Buf = new float[((int)(rate * seconds)) & ~15];
    }

    static double NoteFreq(int midi) { return 440.0 * Math.Pow(2.0, (midi - 69) / 12.0); }

    // voice 0: pluck (harp-ish)  1: bass pluck  2: fanfare (bright, sustained)
    public void Note(double start, int midi, double dur, double amp, int voice) {
        double f = NoteFreq(midi);
        int s0 = (int)(start * Rate);
        int n = (int)(dur * Rate);
        for (int i = 0; i < n; i++) {
            int idx = s0 + i;
            if (idx < 0 || idx >= Buf.Length) continue;
            double t = (double)i / Rate;
            double v = 0;
            if (voice == 0) {
                double env = Math.Exp(-t * 5.0) * Math.Min(1.0, t * 200);
                v = (Math.Sin(2*Math.PI*f*t) + 0.45*Math.Sin(4*Math.PI*f*t)
                   + 0.18*Math.Sin(6*Math.PI*f*t)) * env;
            } else if (voice == 1) {
                double env = Math.Exp(-t * 3.0) * Math.Min(1.0, t * 150);
                v = (Math.Sin(2*Math.PI*f*t) + 0.3*Math.Sin(4*Math.PI*f*t)) * env;
            } else {
                double atk = Math.Min(1.0, t * 60);
                double rel = Math.Min(1.0, (dur - t) * 12);
                double vib = 1.0 + 0.004 * Math.Sin(2*Math.PI*5.5*t);
                v = (Math.Sin(2*Math.PI*f*vib*t) + 0.5*Math.Sin(4*Math.PI*f*t)
                   + 0.25*Math.Sin(6*Math.PI*f*t) + 0.12*Math.Sin(8*Math.PI*f*t))
                   * atk * Math.Max(0, rel);
            }
            Buf[idx] += (float)(v * amp);
        }
    }

    public void Noise(double start, double dur, double amp, double decay, double lowpass) {
        int s0 = (int)(start * Rate);
        int n = (int)(dur * Rate);
        double prev = 0;
        for (int i = 0; i < n; i++) {
            int idx = s0 + i;
            if (idx < 0 || idx >= Buf.Length) continue;
            double t = (double)i / Rate;
            double w = rng.NextDouble() * 2 - 1;
            prev = prev + lowpass * (w - prev);   // one-pole lowpass, lowpass in (0,1]
            Buf[idx] += (float)(prev * amp * Math.Exp(-t * decay));
        }
    }

    public void Sweep(double start, double f0, double f1, double dur, double amp, double decay) {
        int s0 = (int)(start * Rate);
        int n = (int)(dur * Rate);
        double phase = 0;
        for (int i = 0; i < n; i++) {
            int idx = s0 + i;
            if (idx < 0 || idx >= Buf.Length) continue;
            double t = (double)i / Rate;
            double f = f0 + (f1 - f0) * (t / dur);
            phase += 2 * Math.PI * f / Rate;
            Buf[idx] += (float)(Math.Sin(phase) * amp * Math.Exp(-t * decay));
        }
    }

    public void Save(string path) {
        // normalize softly to avoid clipping
        float max = 0.0001f;
        foreach (float v in Buf) { float a = Math.Abs(v); if (a > max) max = a; }
        float gain = max > 0.95f ? 0.95f / max : 1.0f;
        using (var bw = new BinaryWriter(File.Create(path))) {
            int n = Buf.Length;
            bw.Write(new char[]{'R','I','F','F'}); bw.Write(36 + n*2);
            bw.Write(new char[]{'W','A','V','E'});
            bw.Write(new char[]{'f','m','t',' '}); bw.Write(16);
            bw.Write((short)1); bw.Write((short)1); bw.Write(Rate);
            bw.Write(Rate*2); bw.Write((short)2); bw.Write((short)16);
            bw.Write(new char[]{'d','a','t','a'}); bw.Write(n*2);
            for (int i = 0; i < n; i++) {
                double v = Buf[i] * gain;
                if (v > 1) v = 1; if (v < -1) v = -1;
                bw.Write((short)(v * 32767));
            }
        }
    }
}
"@

$outDir = Join-Path $PSScriptRoot 'wav'
New-Item -ItemType Directory -Force $outDir | Out-Null
$R = 22050

function P([string]$name) { Join-Path $outDir "$name.wav" }

# ---- SFX ----

# chop: axe thud into wood
$s = New-Object Synth($R, 0.25)
$s.Noise(0.0, 0.18, 0.8, 28, 0.25)
$s.Sweep(0.0, 150, 70, 0.15, 0.7, 22)
$s.Save((P 'chop')); Write-Host '  chop'

# mine: pickaxe tink on rock
$s = New-Object Synth($R, 0.22)
$s.Sweep(0.0, 1900, 1850, 0.16, 0.5, 30)
$s.Sweep(0.0, 2850, 2800, 0.10, 0.3, 40)
$s.Noise(0.0, 0.04, 0.5, 90, 0.9)
$s.Save((P 'mine')); Write-Host '  mine'

# splash: net hitting water
$s = New-Object Synth($R, 0.5)
$s.Noise(0.0, 0.45, 0.55, 9, 0.5)
$s.Sweep(0.02, 500, 150, 0.2, 0.25, 14)
$s.Save((P 'splash')); Write-Host '  splash'

# hit: combat thump
$s = New-Object Synth($R, 0.22)
$s.Sweep(0.0, 120, 55, 0.18, 0.9, 16)
$s.Noise(0.0, 0.06, 0.5, 60, 0.6)
$s.Save((P 'hit')); Write-Host '  hit'

# eat: chomp
$s = New-Object Synth($R, 0.25)
$s.Noise(0.0, 0.07, 0.6, 50, 0.55)
$s.Noise(0.12, 0.07, 0.5, 50, 0.55)
$s.Save((P 'eat')); Write-Host '  eat'

# cook: sizzle
$s = New-Object Synth($R, 0.5)
$s.Noise(0.0, 0.45, 0.4, 7, 0.85)
$s.Save((P 'cook')); Write-Host '  cook'

# firemaking: whoosh
$s = New-Object Synth($R, 0.55)
$s.Noise(0.0, 0.5, 0.55, 6, 0.3)
$s.Sweep(0.05, 200, 600, 0.35, 0.2, 7)
$s.Save((P 'fire')); Write-Host '  fire'

# levelup: original fanfare jingle
$s = New-Object Synth($R, 1.6)
$s.Note(0.00, 72, 0.16, 0.5, 2)   # C5
$s.Note(0.14, 76, 0.16, 0.5, 2)   # E5
$s.Note(0.28, 79, 0.16, 0.5, 2)   # G5
$s.Note(0.42, 84, 0.5,  0.6, 2)   # C6
$s.Note(0.42, 67, 0.5,  0.3, 1)   # G3 under
$s.Note(0.95, 79, 0.14, 0.45, 2)  # G5
$s.Note(1.08, 84, 0.45, 0.6, 2)   # C6
$s.Note(1.08, 72, 0.45, 0.3, 1)   # C4 under
$s.Save((P 'levelup')); Write-Host '  levelup'

# smith: hammer clang on anvil
$s = New-Object Synth($R, 0.32)
$s.Sweep(0.0, 950, 890, 0.24, 0.5, 16)
$s.Sweep(0.0, 1430, 1380, 0.12, 0.3, 26)
$s.Noise(0.0, 0.04, 0.5, 80, 0.8)
$s.Save((P 'smith')); Write-Host '  smith'

# craft: runecrafting sparkle
$s = New-Object Synth($R, 0.7)
$s.Note(0.00, 88, 0.18, 0.35, 0)
$s.Note(0.07, 92, 0.18, 0.35, 0)
$s.Note(0.14, 95, 0.18, 0.35, 0)
$s.Note(0.21, 100, 0.35, 0.4, 0)
$s.Note(0.30, 104, 0.3, 0.3, 0)
$s.Save((P 'craft')); Write-Host '  craft'

# death: sad descending line
$s = New-Object Synth($R, 1.4)
$s.Note(0.0, 69, 0.3, 0.5, 2)
$s.Note(0.3, 65, 0.3, 0.5, 2)
$s.Note(0.6, 62, 0.3, 0.5, 2)
$s.Note(0.9, 57, 0.5, 0.55, 2)
$s.Save((P 'death')); Write-Host '  death'

# ---- Music: original 16-bar medieval loop, A minor, 132 BPM ----
$bpm = 132.0
$beat = 60.0 / $bpm
$bars = 16
$len = $bars * 4 * $beat
$s = New-Object Synth($R, $len)

# helper: add note at (bar, beatInBar) -> uses closure over $s/$beat
function N([int]$bar, [double]$b, [int]$midi, [double]$durBeats, [double]$amp, [int]$voice) {
    $t = (($bar * 4) + $b) * $beat
    $s.Note($t, $midi, $durBeats * $beat * 1.1, $amp, $voice)
}

# Bass: roots/fifths, two per bar  (A2=45 E3=52 G2=43 D3=50 F2=41 C3=48 E2=40 D2=38 C2=36 G3=55)
$bassBars = @(
    @(45,52), @(45,52), @(43,50), @(43,50),
    @(41,48), @(41,48), @(40,52), @(40,47),
    @(45,52), @(45,52), @(43,50), @(43,50),
    @(41,48), @(48,55), @(38,45), @(45,45)
)
for ($i = 0; $i -lt 16; $i++) {
    N $i 0.0 $bassBars[$i][0] 1.6 0.50 1
    N $i 2.0 $bassBars[$i][1] 1.6 0.42 1
}

# Harp arpeggio pattern on off-beats (chord tones per bar)
$arpBars = @(
    @(57,60,64), @(57,60,64), @(55,59,62), @(55,59,62),
    @(53,57,60), @(53,57,60), @(52,55,59), @(52,55,59),
    @(57,60,64), @(57,60,64), @(55,59,62), @(55,59,62),
    @(53,57,60), @(60,64,67), @(50,53,57), @(57,60,64)
)
for ($i = 0; $i -lt 16; $i++) {
    $c = $arpBars[$i]
    N $i 0.5 $c[0] 0.5 0.22 0
    N $i 1.5 $c[1] 0.5 0.22 0
    N $i 2.5 $c[2] 0.5 0.22 0
    N $i 3.5 $c[1] 0.5 0.22 0
}

# Melody (harp lead), A minor — original tune
# bar: list of (beat, midi, dur)
$mel = @{
    0  = @(@(0,69,1),@(1,71,1),@(2,72,1.5),@(3.5,74,0.5))
    1  = @(@(0,76,1),@(1,74,1),@(2,72,2))
    2  = @(@(0,71,1),@(1,67,1),@(2,74,1.5),@(3.5,72,0.5))
    3  = @(@(0,71,1),@(1,69,1),@(2,67,2))
    4  = @(@(0,65,1),@(1,69,1),@(2,72,1),@(3,76,1))
    5  = @(@(0,77,1.5),@(1.5,76,0.5),@(2,74,2))
    6  = @(@(0,71,1),@(1,76,1),@(2,79,1.5),@(3.5,76,0.5))
    7  = @(@(0,74,1),@(1,71,1),@(2,69,2))
    8  = @(@(0,81,1),@(1,79,1),@(2,77,1.5),@(3.5,76,0.5))
    9  = @(@(0,77,1),@(1,76,1),@(2,74,2))
    10 = @(@(0,79,1),@(1,77,1),@(2,76,1.5),@(3.5,74,0.5))
    11 = @(@(0,76,1),@(1,74,1),@(2,71,2))
    12 = @(@(0,69,1),@(1,72,1),@(2,76,1),@(3,77,1))
    13 = @(@(0,79,1.5),@(1.5,77,0.5),@(2,76,2))
    14 = @(@(0,74,1),@(1,76,1),@(2,71,1),@(3,67,1))
    15 = @(@(0,69,3))
}
foreach ($bar in $mel.Keys) {
    foreach ($n in $mel[$bar]) {
        N $bar $n[0] $n[1] $n[2] 0.42 0
    }
}

$s.Save((P 'music')); Write-Host '  music (16-bar loop)'
Write-Host "Done. WAVs in $outDir"
