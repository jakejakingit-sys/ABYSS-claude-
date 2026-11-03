# ABYSS (work in progress) — TI-84 CE side-scrolling dungeon game

**Status: early vertical slice. NOT a finished game. No `ABYSS.8xp` has been built.**

## What is verified (real gcc runs, in this repo)
`make -f Makefile.host` regenerates level/asset C files and runs **89 host checks, 0 failures** covering:
physics (accel/decel, variable jump, coyote time, jump buffer, wall, no tunnelling, one-way platforms, drop-through, ladders),
spike damage/cooldown/armour/regen interruption, treasure persistence, coins surviving death, level completion + unlock,
upgrade purchases (cost, max level, no overspend), ability unlock table, save round-trip with CRC-16,
**every single-bit corruption of the save detected**, version/range/length rejection, recovery to defaults,
level validation (dimensions, spawn, one exit, spike placement, heuristic reachability, treasure bits).

These are **host tests only**. They say nothing about calculator hardware behaviour or speed.

## What is written but UNVERIFIED (never compiled against real CEdev, never run)
`src/main.c`, `src/platform_ce.c`, `Makefile`, `.github/workflows/build.yml`.
Syntax-checked only against my own stub headers. Graphx/keypadc/fileioc calls, key-group constants, the `cedev-config`
Makefile include, and the CI download URL are from memory and **must be checked against your installed CEdev docs**.
Expect compile errors on first real build.

## What exists in the game
- Portable engine core (player physics, tile collision, treasure, damage, save, upgrades, ability unlock rules)
- 2 hand-designed levels (World 1: *The Entrance*, *Forgotten Halls*), ASCII sources in `levels/world1/`
- 5-frame player sprite (ASCII art -> `tools/mkassets.py`), procedurally drawn tiles (placeholder art)
- Waystation lobby (PLAY / UPGRADES / COLLECTION / STATS), level select, working upgrade screen, stats

## NOT implemented (honest list)
18 of 20 levels; all 4 guardian fights; ability *gameplay* (only the unlock table exists); crumbling floors, moving platforms,
creatures, mechanisms/runes/puzzles, gravity/teleport/phase mechanics; crouch/dash/land/death/victory animations; creature and
guardian sprites; 3 of 4 tilesets; sound (none); ending; relics/secrets; CI never run.

## Build
Host tests: `make -f Makefile.host`
Calculator (needs CEdev installed, `cedev-config` on PATH): `make` -> `bin/ABYSS.8xp`. Send with TI Connect CE / CEmu, run with `prgm`/arTIfiCE.

## Controls (as coded)
Arrows move/climb, 2ND jump, DOWN drops through one-way platforms, ENTER confirm, CLEAR back/quit.

## Memory (see MEMORY_REPORT.md)
