# Getting at 3DS save files

How to find, copy and inspect Gen 6/7 saves (XY, ORAS, SM, USUM) on Azahar, on a modded 3DS, and in PKHeX. Format details are in [formats.md](formats.md).

Ground rule (from CLAUDE.md): **only ever open a copy.** Copy `main` into a working folder outside this repo first, then point PKHeX or our code at the copy. `.gitignore` blocks files named `main`, but don't rely on that.

---

## The file itself

In all four games, the whole save is one file called **`main`**, with no extension. It sits in the game's save-data archive.
The 3DS's own filesystem decrypts it before any app sees it, so Checkpoint, PKSM, Azahar and our app all see the same plain bytes that PKHeX opens.

Title IDs (from PKSM `3ds/source/titles/loader.cpp`; Ultra Moon is also confirmed by the Azahar folder below):

| Game | Title ID | High / low words |
|------|----------|------------------|
| X | `0004000000055D00` | `00040000` / `00055D00` |
| Y | `0004000000055E00` | `00040000` / `00055E00` |
| Omega Ruby | `000400000011C400` | `00040000` / `0011C400` |
| Alpha Sapphire | `000400000011C500` | `00040000` / `0011C500` |
| Sun | `0004000000164800` | `00040000` / `00164800` |
| Moon | `0004000000175E00` | `00040000` / `00175E00` |
| Ultra Sun | `00040000001B5000` | `00040000` / `001B5000` |
| Ultra Moon | `00040000001B5100` | `00040000` / `001B5100` |

The same ID covers every region; per 3dbrew, title IDs don't encode region. Bytes 2–4 of the low word (`001B51`) are the "unique ID".

---

## Azahar (emulator)

Azahar was forked from Citra, and it lays the emulated SD card out the same way a real 3DS does:

```
<sdmc>/Nintendo 3DS/<ID0>/<ID1>/title/<high>/<low>/data/00000001/main
```

On this machine (the Flatpak `org.azahar_emu.Azahar`), `<sdmc>` is:

```
~/.var/app/org.azahar_emu.Azahar/data/azahar-emu/sdmc/
```

The `qt-config.ini` paths are in `~/.var/app/org.azahar_emu.Azahar/config/azahar-emu/`. A native (non-Flatpak) install would use `~/.local/share/azahar-emu/` instead (**UNVERIFIED**).
The emulator uses all-zero `ID0`/`ID1` folders (`000…0`), so the Ultra Moon save here is:

```
…/sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000/001b5100/data/00000001/main
```

- The hex folder names are lowercase on disk.
- `00000001.metadata` next to the `00000001` folder is Azahar's own bookkeeping (16 bytes). Leave it alone.
- Quickest way in: **File → Open Azahar Folder** in Azahar. Citra also had **right-click a game → Open Save Data Location**, and Azahar likely kept it (**UNVERIFIED**).
- Stop emulation before copying the file out or putting a file back, so you never catch a half-written save.

**Checked against the real file (2026-10-25, read-only):** the Ultra Moon `main` is `0x6CC00` bytes and has the `BEEF` magic. 38 of its 39 block CRCs match CRC-16/USB. The one that doesn't is block 36, which is expected because the signature lives inside it (see formats.md).
The trainer block gives game = 33 (UM in PKHeX's `GameVersion`). Party slot 1 decrypts to species 725 (Litten), level 5, with a valid checksum. The layouts in formats.md are confirmed for USUM.

### Putting an edited save back into Azahar

Replace `main` with the edited copy while emulation is stopped. Keep the original somewhere safe first.
Whether Azahar enforces the 3DS "secure value" (see below) is **UNVERIFIED**. Test with a throwaway save.

---

## Modded 3DS

### Checkpoint (backup/restore, recommended for getting files)

Checkpoint is the usual save manager. Backups land on the SD card under:

```
sdmc:/3ds/Checkpoint/saves/<unique id> <game title>/<backup name>/main
```

Back up in Checkpoint, then copy the folder to the PC (pull the SD card, or use FTPD). Restoring works the other way round.
Checkpoint handles both installed (SD) titles and physical cartridges.

### PKSM

PKSM can open a save directly and keeps its own backups in `/3ds/PKSM/backup/`. It's a useful reference for how the finished app should behave. Its code is GPL-3: read it, don't copy it.

### Secure value (matters for write support)

The 3DS keeps a per-title **secure value** outside the save. It exists to stop people restoring old saves.
The Gen 6/7 footer timestamps (`TimeStampCurrent`/`Previous` in PKHeX `SAV_BEEF.cs`) are tied to it.
Before PKSM writes a save back, it deletes the stored secure value with `FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, …)`, and it shows an error if that fails. Restoring an old backup without this step may make the game refuse the save.
Read-only work doesn't need any of this.

---

## Reading the save from our own homebrew (libctru)

These signatures were checked in `/opt/devkitpro/libctru/include/3ds/services/fs.h`. The path layout comes from 3dbrew's *Filesystem services* page and agrees with PKSM's `Archive::save`.

```cpp
// User save archive: binary path of three u32 words {mediatype, title ID low, title ID high}
const u32 path[3] = { MEDIATYPE_SD, 0x001B5100, 0x00040000 };   // Ultra Moon, installed on SD
FS_Archive arch;
Result rc = FSUSER_OpenArchive(&arch, ARCHIVE_USER_SAVEDATA, { PATH_BINARY, sizeof(path), path });

Handle file;
rc = FSUSER_OpenFile(&file, arch, fsMakePath(PATH_ASCII, "/main"), FS_OPEN_READ, 0);
u64 size; FSFILE_GetSize(file, &size);
// FSFILE_Read(file, &bytesRead, offset, buffer, chunk) — read in a loop; PKSM notes large
// saves can return short reads.
FSFILE_Close(file);
FSUSER_CloseArchive(arch);
```

- `ARCHIVE_USER_SAVEDATA` = `0x567890B2`, `MEDIATYPE_SD` = 1, `MEDIATYPE_GAME_CARD` = 2.
- For a cartridge, use `MEDIATYPE_GAME_CARD`. (Whether the title ID words still matter for carts is **UNVERIFIED**; see how Checkpoint and PKSM handle it.)
- To find which games are installed, use `AM_GetTitleCount` / `AM_GetTitleList(…, MEDIATYPE_SD, …)` and match against the table above. The AM service has to be initialized with `amInit()` first.
- Writing (later): write the file, then call `FSUSER_ControlArchive(arch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, …)`. Without the commit the change is thrown away. Plus the secure value step above.
- **Open question:** whether a plain `.3dsx` (run from the Homebrew Launcher) can open other titles' save archives depends on the access rights it runs with. Checkpoint and PKSM manage it. Try it in a sandbox spike before designing around it.

Azahar emulates these FS calls too, so the same code can be tried in the emulator first.

---

## PKHeX on Fedora

PKHeX is a Windows Forms app. Two ways to use it on Linux:

### 1. The GUI under Wine

Source: Project Pokémon's Linux guide, revision 10/2026.

```sh
WINEPREFIX=$HOME/.local/share/wineprefixes/pkhex wineboot                     # cancel the Mono prompt
WINEPREFIX=$HOME/.local/share/wineprefixes/pkhex wine windowsdesktop-runtime-10.0.10-win-x64.exe
WINEPREFIX=$HOME/.local/share/wineprefixes/pkhex wine /path/to/PKHeX.exe
```

- It needs the **64-bit .NET Desktop Runtime 10**. Plain Wine is fine; Wine Staging is only needed for plugins.
- On Wine ≥ 11.5, some menus (Trainer Info, Box Search) don't open. Prefix the launch command with `DOTNET_SYSTEM_GLOBALIZATION_USENLS=1`, or stay on Wine 11.4.
- If Linux .NET is also installed and PKHeX won't start, prefix the launch with `env -u DOTNET_ROOT -u DOTNET_BUNDLE_EXTRACT_BASE_DIR`.
- **Azahar saves live in a hidden folder (`~/.var/…`).** Either turn on *winecfg → Drives → Show dot files*, or (better) copy `main` somewhere visible first.
- After a Wine update, reinstall the .NET runtime if PKHeX breaks.

Handy for checking our parser: the box viewer and trainer info, and PKHeX's block/checksum info view (exact menu name **UNVERIFIED**; look under *Tools*). *File → Export SAV* writes a re-signed copy. Also useful: compare a box slot against a `.pk7` exported from PKHeX, which is the decrypted 232 bytes. Don't commit those either; make synthetic fixtures for tests.

### 2. PKHeX.Core as a library, natively

`PKHeX.Core` is on NuGet (v26.10.18, `net10.0`, GPL-3.0). A throwaway C# console project using it runs natively on Fedora (`dotnet` SDK 10, no Wine) and can print whatever our parser prints. That makes it an oracle for cross-checking.
Keep that project outside this repo, like the API spikes, so GPL code never mixes with ours.

---

## Workflow for building the parser

1. Play to a save in Azahar, stop emulation, and copy `main` to e.g. `~/pokemon-saves/usum/2026-10-25/main`.
2. Open the copy in PKHeX and write down the expected values (trainer, box 1 contents).
3. Run our PC-side parser on the same copy and compare. Real saves are for manual checks only; unit tests use synthetic fixtures (CLAUDE.md).
4. Later, test on hardware: back up with Checkpoint, then run our read-only homebrew.
