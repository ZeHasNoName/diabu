# Diabu Mod Development Guide

## Project intent

This is a DevilutionX fork for source-level gameplay modding. Initial goals are
developer tooling, new spells, and later character classes. This snapshot has
no stable runtime plugin API; gameplay extensions are compiled into the engine.

## Repository map

- `Source/spelldat.h`: `SpellID`, `MissileID`, `MAX_SPELLS`, metadata types.
- `Source/spelldat.cpp`: spell costs, availability, sounds, and missile mapping.
- `Source/spells.cpp`: generic casting and mana calculations.
- `Source/misdat.*`: missile metadata and sprite mapping.
- `Source/missiles.cpp`: missile creation, processing, collision, and damage.
- `Source/panels/spell_book.cpp`: fixed spellbook layout.
- `Source/panels/spell_list.cpp`: quick-spell list.
- `Source/player.h`: `HeroClass` and player state.
- `Source/playerdat.*`: class stats, caps, skills, animation sizes, and sounds.
- `Source/DiabloUI/hero/selhero.cpp`: class-selection UI.
- `Source/pack.*`, `Source/loadsave.cpp`, `Source/msg.cpp`: save/network formats.
- `Source/debug.cpp`: debug-build commands and testing helpers.
- `Source/gamemenu.cpp`: pause, options, and debug menus.
- `docs/debug.md`: debug controls and command documentation.

## Spell rules

- Append new `SpellID` values immediately before `LAST`. Never insert or reorder
  existing IDs: they are persisted and sent over the network.
- Update `MAX_SPELLS`, `SpellsData`, spellbook placement, acquisition,
  validation, and tests together.
- Spells use 64-bit masks. The current 52 entries leave only 12 positions; IDs
  must remain below 64 unless save/network representations are redesigned.
- The legacy Diablo packed-player format intentionally stores only 37 spell
  levels. Preserve that compatibility behavior.
- Prefer composing existing missiles and graphics first. Add a `MissileID` and
  new missile processing only when the design requires it.

## Class rules

- Append `HeroClass` values and update every enum-sized table.
- Search globally for `case HeroClass::`; behavior is spread across combat,
  inventory, effects, UI, sounds, saves, and validation.
- Prototype by reusing an existing `classPath`, as Bard reuses Rogue and
  Barbarian reuses Warrior. Original class art is a separate content project.
- Modded saves and multiplayer peers should use the same mod version.

## Debug workflow

- Debug-only code is guarded by `_DEBUG`.
- Debug builds expose a `Debug` submenu from the in-game pause menu.
- Advanced operations remain chat commands in `Source/debug.cpp`, including
  `goto`, `drop`, `spawn`, `god`, and `setspells`.
- Menu actions should be thin frontends over reusable debug helpers/commands.

## Build and test

- The configured Windows tree is `build/x64-Debug` (MSVC + Ninja), with a built
  executable and local test MPQs already present.
- Visual Studio can build the `x64-Debug` CMake configuration. For shell builds,
  make the Visual Studio-bundled Ninja and MSVC environment available, then run
  `cmake --build build/x64-Debug --parallel`.
- `BUILD_TESTING` is enabled. Run relevant tests after gameplay-state or
  serialization changes.
- Original Diablo/Hellfire MPQs are local test data. Do not modify, commit, or
  redistribute them.

## Working conventions

- Inspect `git status` and preserve unrelated user changes.
- Keep mod work on the `mod` branch unless directed otherwise.
- Prefer vertical slices: data, behavior, UI/acquisition, persistence, tests.
- Avoid unrelated upstream refactors. This fork is older than current upstream,
  so newer upstream code may require adaptation.
