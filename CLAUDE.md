# 3DS Pokémon Database

<!-- Maintainer notes (stripped from context): keep this file under ~200 lines. Move detail into docs/ and point to it. Add a line only when Claude gets something wrong twice. -->

Nintendo 3DS homebrew app (C++) for viewing and storing Pokémon and moving them between saves and my own banks, like PKSM but with a native-feeling, first-party-style UI. I'm learning as I build, so explain the reasoning behind code and suggestions.

Scope: Generations I–VII (Game Boy, Game Boy Color, GBA, DS, and 3DS games, including Virtual Console and ROM/emulator saves). Generation 8 and later are out of scope. A DS downport may come later, so keep the core portable.

## Stack

- C++20 (`-std=gnu++20`) with devkitPro: devkitARM, libctru, citro2d/citro3d. tex3ds builds spritesheets.
- Environment: `DEVKITPRO=/opt/devkitpro`, `DEVKITARM=$DEVKITPRO/devkitARM`, set by `/etc/profile.d/devkit-env.sh`.
- Package manager: `dkp-pacman` everywhere. On Fedora it is a wrapper installed by `scripts/setup-devkitpro-fedora.sh`; never use the system `pacman` for devkitPro packages.
- Test in the Azahar emulator first, then on my modded 3DS.

## Commands

- Build the 3DS app (repo root): `make` → `pk3ds-db.3dsx`. Clean: `make clean`.
- Run on the console: `3dslink pk3ds-db.3dsx` (Homebrew Launcher, press Y; add `-a <console IP>` if it isn't found).
- CI: `.github/workflows/build.yml` builds every push in the `devkitpro/devkitarm` image and uploads the `.3dsx`.
- Core library tests on PC: TODO

## Architecture

- `core/`: portable C++ with save readers, record views, checksums, and the bank format. Never include libctru or libnds here.
- `platform/3ds/`: entry point (`main.cpp`), file access, input, and drawing.
- `ui/`: screens. Keep save-format knowledge out of this layer.
- `gfx/`: source PNGs and `.t3s` lists. `romfs/`: files bundled into the app; `romfs/gfx/` is build output (gitignored).
- `tests/`: PC unit tests using small synthetic fixtures.
- Build: `SOURCES` in the `Makefile` lists the compiled directories and does not recurse, so add new subdirectories there. Objects go in one flat `build/` folder, so source file names must be unique across directories. Include headers by path from the repo root (`#include "core/..."`).

Rules:

- Store each Pokémon as its original bytes plus a generation tag. Convert between generations only on explicit request, and report changed or dropped fields.
- Start every file format I define with a magic string and a version number. Write migrations.
- Texture width and height must be powers of two from 8 to 1024, and render targets must fit in 6 MB of VRAM. Pack icons into atlases.

## Save data safety

- Back up before any write. Never write to my only copy of a real save.
- Write path: validate checksums, update every redundant copy the game could load, write a temp file, verify it, then replace the original.
- Default to read-only or dry-run while developing.
- Test truncated, empty, bad-checksum, and corrupted input. Require byte-identical output from a parse-then-serialize round trip.

## Boundaries

- Never commit save files, ROMs, or extracted Nintendo or Game Freak assets (sprites, audio, UI art). `.gitignore` blocks common save/ROM extensions; don't override it.
- Never copy PKSM code (GPL v3). Read it for ideas only.
- Never invent save offsets, struct layouts, checksum rules, or API signatures. If unsure, say so and point me to Project Pokémon, PKHeX source, Bulbapedia, or the libctru and citro2d docs.

## Workflow

Before changing code, answer four questions: Does this need investigation first? Does it need a plan? What information is missing? How will I test it?

- Write the test first for the data layer (parsers, checksums, bank). Use a sandbox screen and a manual checklist for UI.
- Spike unfamiliar APIs outside this repo, in a separate sandbox repo laid out like devkitPro's 3ds-examples (one folder per experiment), then rewrite properly here. Never let spike code become the architecture. The first learning demo (moving square) is in this repo's history at commit `4c512ec`.
- Build read-only features before write support.
- Compile with `-Wall -Wextra` (set in the `Makefile`). Build the core on PC with AddressSanitizer and UBSan.
- Work in small commits, one branch per task, merged into `main` by pull request. Tasks live on my Kanban board as GitHub issues labelled `type: feature`, `type: bug`, `type: tech debt`, or `type: spike`.

## Working with me

- Prefer small, runnable steps over large rewrites.
- Explain unfamiliar C++ and 3DS concepts as they come up.

## More detail

Create these as needed and keep long reference material there, not here:

- `docs/formats.md`: save and record layouts per generation (to be created)
- `docs/references.md`: links to libctru, citro2d, PKSM, PKHeX, Project Pokémon, 3dbrew (to be created)
