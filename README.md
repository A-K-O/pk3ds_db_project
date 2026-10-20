# Pokemon DB Project

A Nintendo 3DS homebrew project for the Pokémon ecosystem, aiming to bring
together the roles of Poké Transporter, Pokémon Bank, PKSM and similar tools
in a single native-feeling app.

Built in C with devkitARM, libctru and citro2d/citro3d.

## Layout

| Path | What it is |
|------|------------|
| [`demos/01-moving-square`](demos/01-moving-square) | First learning demo: move a square on the top screen with the D-Pad/Circle Pad |

## Toolchain quick start

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started).
2. `sudo dkp-pacman -S 3ds-dev` (no `sudo` on Windows).
3. `make` inside a demo folder → `.3dsx` file for the Homebrew Launcher.
4. Test in the Azahar emulator, or on a modded console with `3dslink`.
