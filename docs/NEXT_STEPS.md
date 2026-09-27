# Další krok: presety podle skutečných drumkitů

Stav po verzi 1.0 (M1–M6 hotové, Windows build z GitHub Actions funguje):

## Zpětná vazba z testu ve FL Studiu

- Většina presetů **nezní dost jako trap**. Tlačítko KILL je „zachrání“, ale základ musí být lepší.
- Hajtky jsou zatím syntetizované (Killa_Hat_01–14.wav ještě nejsou v `Resources/Samples/`).
- V rollech **nejde nic odebrat**. Klik na notu ji jen ztlumí, u rollu po změně rychlosti to nefunguje
  a celý roll smazat nejde.

## Úkoly

1. **Nastudovat hi-hat MIDI v `Documents\drumkits`** (22 kitů, ~545 hi-hat MIDI).
   - Spustit `python tools/midi_analyzer/analyze.py "C:\Users\<user>\Documents\drumkits" --per-kit`.
   - Hlavně se ale podívat do konkrétních souborů, jak jsou rolly postavené: pozice v taktu,
     opakování mezi takty, jak se mění 1.–4. takt, kde je fill, jak pracují s velocity a pitchem,
     jak vypadá základní grid pod rolly.
   - **Nic nekopírovat 1:1** (licence kitů nepovoluje redistribuci), jen se učit a presety napsat autorsky.
2. **Přepsat tovární presety** v `tools/preset_authoring/factory_presets.py`
   (pak `python tools/preset_authoring/build_presets.py`, validace a statistiky proběhnou automaticky).
   - Uspořádat je **od nejjednodušších hajtek po nejsložitější** (i napříč kategoriemi dává smysl
     mít jasnou progresi).
   - Pořád 12 kategorií × 8 a názvy z `CLAUDE.md`, pokud se nedomluvíme jinak.
3. **Odebírání v rollech** (UI, `Source/ui/RollVisualizer.cpp` + `Source/engine/RollModel.*`):
   smazat jednotlivou notu (i v přepočítaném rollu) a smazat celý roll (např. pravý klik → menu).
   Úpravy musí jít přes undo a ukládat se do projektu.
4. Přidat skutečné hajtky z Killa Drum Kit Vol.1 do `Resources/Samples/`
   (`Killa_Hat_01.wav` … `Killa_Hat_14.wav`).

## Kontrola po změnách

```
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Pak commit a push. GitHub Actions udělá Windows VST3 (artefakt `RollsKilla-Windows-VST3`).
