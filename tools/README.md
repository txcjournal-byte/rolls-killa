# Rolls Killa tools

Python 3.9+, no extra packages. Not part of the plugin.

## midi_analyzer

Statistics of hi-hat MIDI files (spacing, roll rates, roll length, start position, velocity shape, pitch).
Used to check that the factory presets sound like real trap programming. Nothing is copied from the kits.

```
python tools/midi_analyzer/analyze.py "C:\Users\<you>\Documents\drumkits" --per-kit
python tools/midi_analyzer/analyze.py --presets Resources/Presets/Factory
```

## preset_authoring

Factory presets are written by hand in `preset_authoring/factory_presets.py` (a small pattern language,
documented at the top of `build_presets.py`) and compiled to `Resources/Presets/Factory/*.json`:

```
python tools/preset_authoring/build_presets.py          # validate, write JSON, print stats
python tools/preset_authoring/build_presets.py --check  # validate only
```

The same validation rules run again in the C++ unit tests (`Tests/LibraryTests.cpp`).
