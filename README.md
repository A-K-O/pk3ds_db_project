# 3DS Pokémon Database

A Nintendo 3DS homebrew app for viewing and storing Pokémon and moving them
between saves and personal banks, in the spirit of PKSM but with a
native-feeling, first-party-style UI. Covers Generations I–VII.

Built in C++ with devkitARM, libctru and citro2d/citro3d.

## Layout

| Path | What it is |
|------|------------|
| `core/` | Portable C++: save readers, record views, checksums, bank format. No libctru. |
| `platform/3ds/` | 3DS-specific code: entry point, file access, input, drawing |
| `ui/` | Screens |
| `gfx/` | Source PNGs and `.t3s` lists for tex3ds |
| `romfs/` | Files bundled into the app (`romfs/gfx/` is built from `gfx/`) |
| `tests/` | PC unit tests for `core/` |
| `scripts/` | Developer setup scripts |

## Building

1. Install devkitPro and the `3ds-dev` group.
   - Fedora: `scripts/setup-devkitpro-fedora.sh`
   - macOS / Windows / Debian: follow [devkitPro's Getting Started](https://devkitpro.org/wiki/Getting_Started), then `dkp-pacman -S 3ds-dev`
2. Run `make` in the repo root. This produces `pk3ds-db.3dsx`.
3. Test in the Azahar emulator, or on a modded console: open the Homebrew
   Launcher, press Y, then `3dslink pk3ds-db.3dsx`.

Every push is also built by GitHub Actions; the `.3dsx` can be downloaded
from the workflow run.
