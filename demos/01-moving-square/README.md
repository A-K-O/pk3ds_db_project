# 01 – Moving Square

The smallest useful 3DS program: a square on the top screen that you move
in any direction, plus a text console on the bottom screen.

It covers the basics the rest of the project builds on:

- **Initialization and shutdown** of `gfx`, citro3d and citro2d
- **The main loop**: `aptMainLoop()` → read input → update → draw, once per frame (~60 fps)
- **Input**: digital buttons (`hidKeysDown` / `hidKeysHeld`) and the analog
  Circle Pad (`hidCircleRead`)
- **GPU 2D drawing** on the top screen (400×240) with `C2D_DrawRectSolid`
- **A debug console** on the bottom screen (320×240) with `consoleInit` + `printf`

## Controls

| Input      | Action                                   |
|------------|------------------------------------------|
| D-Pad      | Move at full speed (8 directions)        |
| Circle Pad | Move with analog speed (any direction)   |
| A          | Cycle the square's color                 |
| B          | Re-center the square                     |
| START      | Exit to the Homebrew Launcher            |

## Building

Install devkitPro, then the 3DS packages:

```sh
sudo dkp-pacman -S 3ds-dev   # omit sudo on Windows (MSYS2)
```

Then from this folder:

```sh
make
```

This produces `moving-square.3dsx`. `make clean` removes build outputs.

You can also download a prebuilt `.3dsx` from the **Actions** tab on GitHub:
every push builds the demo in the official `devkitpro/devkitarm` Docker image.

## Running

- **Emulator (Azahar):** File → Load File → `moving-square.3dsx`.
- **Real hardware over Wi-Fi:** open the Homebrew Launcher, press **Y** to
  start the network loader, then run `3dslink moving-square.3dsx`
  (add `-a <console IP>` if it isn't found automatically).
- **Real hardware via SD card:** copy the `.3dsx` to `sdmc:/3ds/` and launch it
  from the Homebrew Launcher.

## Things to try next

- Make the movement frame-rate independent (multiply speed by frame time).
- Move the square with the stylus: read `hidTouchRead()` on the bottom screen.
- Draw text on the top screen with `C2D_Text` and the system font
  (`C2D_FontLoadSystem`).
- Add easing so the square accelerates and decelerates smoothly.
