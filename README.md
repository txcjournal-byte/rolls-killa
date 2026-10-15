# HAT GOBLIN by TrapWin

Hi-hat and snare roll engine. Hold a key, it rolls in sync with your DAW. Drag the roll out as MIDI.

## Download (latest test build)

- Windows installer (VST3 + Standalone): https://github.com/txcjournal-byte/rolls-killa/releases/download/test/HAT-GOBLIN-Setup-Windows.exe
- Windows VST3 only (zip): https://github.com/txcjournal-byte/rolls-killa/releases/download/test/HAT-GOBLIN-VST3-Windows.zip
- macOS installer (VST3 + AU + Standalone, Apple Silicon + Intel): https://github.com/txcjournal-byte/rolls-killa/releases/download/test/HAT-GOBLIN-Setup-Mac.pkg
- macOS plugins only (zip): https://github.com/txcjournal-byte/rolls-killa/releases/download/test/HAT-GOBLIN-Plugins-Mac.zip

## Key map (C3 = MIDI 60)

| Keys | What they do |
| --- | --- |
| C1 / D1 | pitch -1 / +1 semitone |
| E1 | latch on / off |
| C2 .. B2 | 12 roll patterns (FLICK 3, RAMP UP, RAMP DN, GALLOP, STUTTER, SKIP, FLAM 32, OFF 16, RATCHET, GLITCH, BOUNCE, ROLL 5) |
| C3 .. C4 | rates 1/2, 1/4T, 1/4, 1/8T, 1/8, 1/16, 1/16T, 1/32, 1/32T, 1/64, 1/64T, 1/128, 1/256 |

## Features

- 120 factory hats in 6 banks (BASIC, CRISP, DIRTY, LOFI, EXTREME, WEIRD), or drop your own WAV / AIFF / FLAC / MP3
- Locks to the host tempo and grid while the DAW plays; free-running at the host BPM when stopped
- Swing, humanize, velocity shape (flat / ramp up / ramp down / accent), velocity random, probability
- Pitch ramp across the roll, pan ping-pong / sweep, choke (tight fast rolls), speed 1/2x / 1x / 2x, mono / poly, latch
- FX: saturate, crush, high pass, low pass, room, output with soft limiter
- DRAG MIDI: drag one bar of the last roll into the DAW as editable notes (GM closed hat, note 42)

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target HatGoblin_VST3 HatGoblin_Standalone HatGoblinTests
./build/HatGoblinTests_artefacts/Release/HatGoblinTests
```

GitHub Actions builds Windows and macOS on every push and publishes the installers to the `test` release.
macOS builds are signed and notarized when the repository has the `MAC_CERT_*` / `APPLE_*` secrets (same names as in the other TrapWin plugin repos); without them they are ad-hoc signed.
