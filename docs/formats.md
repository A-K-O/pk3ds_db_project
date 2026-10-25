# Save and record formats

Layouts for each generation. Every number below was read from source code, and each section names
the file it came from so you can check it again. Anything not checked is marked **UNVERIFIED**.

Primary source: [PKHeX](https://github.com/kwsch/PKHeX), commit `9c3391d` (2026-10-25).
Paths below are relative to `PKHeX.Core/`.

---

## Generation 6 and 7 (3DS): XY, ORAS, SM, USUM

All four games share one container design. PKHeX calls it "BEEF" after the footer magic (`Saves/SAV_BEEF.cs`).
The save is one flat file split into fixed **blocks**. A table at the end of the file holds each block's length and a 16-bit checksum.

All multi-byte integers are **little-endian**.

### Telling the games apart

You can identify the game from the file size plus the footer magic (`Saves/Util/SaveUtil.cs`).

| Game | File size | Footer table offset (size − 0x200) | Blocks | Box count |
|------|-----------|------------------------------------|--------|-----------|
| X / Y           | `0x65600` | `0x65400` | 55 (0–54) | 31 |
| Omega Ruby / Alpha Sapphire | `0x76000` | `0x75E00` | 58 (0–57) | 31 |
| ORAS Special Demo | `0x5A00` | `0x5800` | — | — (out of scope) |
| Sun / Moon      | `0x6BE00` | `0x6BC00` | 37 (0–36) | 32 |
| Ultra Sun / Ultra Moon | `0x6CC00` | `0x6CA00` | 39 (0–38) | 32 |

Magic check: the `u32` at `file_size - 0x1F0` must equal `0x42454546`. On disk those bytes are `46 45 45 42`, which spells "FEEB" because the value is stored little-endian.

Which game within a pair (X vs Y, and so on) comes from the `Game` byte in MyStatus (see below), not from the file size.

### Footer (block table)

Taken from `Saves/Blocks/BlockInfo3DS.cs`. Let `bo = file_size - 0x200`:

| Offset | Type | Meaning |
|--------|------|---------|
| `bo + 0x00` | u64 | Timestamp of the current save ("secure value") |
| `bo + 0x08` | u64 | Timestamp of the previous save |
| `bo + 0x10` | u32 | Magic `0x42454546` ("BEEF") |
| `bo + 0x14 + id*8` | u32 | Block length |
| `bo + 0x18 + id*8` | u16 | Block id |
| `bo + 0x1A + id*8` | u16 | Block checksum |

Each block starts on a `0x200` boundary. Its stored length is the exact length (no padding), and the next block starts at the next multiple of `0x200`.

**One copy only.** Gen 4 and 5 saves keep two copies inside the file. These games keep a single copy, so "update every redundant copy" means updating each block's checksum (and the Gen 7 signature, below). The 3DS save filesystem may do its own journaling underneath, but that is **UNVERIFIED** here. See 3dbrew's DISA/DIFF pages before relying on it.

### Block checksums

| Gen | Algorithm in PKHeX | Standard name | Check value of `"123456789"` |
|-----|--------------------|---------------|------------------------------|
| 6 (XY, ORAS) | `Checksums.CRC16_CCITT` | CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no final XOR) | `0x29B1` |
| 7 (SM, USUM) | `Checksums.CRC16Invert` | CRC-16/USB (poly 0x8005 reflected = 0xA001 table, init 0xFFFF, final XOR 0xFFFF) | `0xB4C8` |

The check values were computed by reimplementing PKHeX's code in Python (2026-10-25). Use them as the first tests in `tests/`.
Each checksum covers `data[offset .. offset+length)` for its block.

### Gen 7 extra: the MemeCrypto signature

Taken from `Saves/SAV7.cs` and `Saves/Encryption/MemeCrypto/MemeCrypto.cs`. SM and USUM saves carry an RSA signature over a SHA-256 of the block table:

- Signature region: `0x80` bytes at `(SM: 0x6BA00 | USUM: 0x6C000) + 0x100`. This sits inside the last block (id 36, "TurtleSalmonSave").
- Signed data: the first `0x140` (SM) or `0x150` (USUM) bytes of the footer at `bo`.
- PKHeX's process: compute every block checksum, then write SHA-256(footer slice) into the first 0x20 bytes of the region, then RSA-sign the 0x80-byte region in place.
- Because the signature lives inside block 36, writing it invalidates block 36's checksum. PKHeX deliberately leaves block 36's checksum stale. When loading, PKHeX zeroes the region ("always zero").

**Open questions before write support:** Does the game reject a save with a bad signature? Where does the signing key come from (PKHeX `MemeKey.cs`)? Can we use it in a GPL-free way? Read-only parsing does not need any of this.

### Where the Pokémon are

PKHeX hard-codes these offsets in `Saves/SAV6XY.cs` and `Saves/SAV6AO.cs`, or takes them from the block table in `Saves/SAV7SM.cs` and `Saves/SAV7USUM.cs`:

| Data | XY | ORAS | SM | USUM |
|------|----|------|----|------|
| MyStatus (trainer)   | `0x14000` (blk 17) | `0x14000` (blk 17) | `0x01200` (blk 3) | `0x01400` (blk 3) |
| Party                | `0x14200` (blk 18) | `0x14200` (blk 18) | `0x01400` (blk 4) | `0x01600` (blk 4) |
| Box layout (names, etc.) | `0x04400` (blk 12) | `0x04400` (blk 12) | `0x04800` (blk 13) | `0x04C00` (blk 13) |
| Box Pokémon          | `0x22600` (blk 53) | `0x33000` (blk 56) | `0x04E00` (blk 14) | `0x05200` (blk 14) |
| Battle Box           | `0x04A00` (blk 13) | `0x04A00` (blk 13) | — (Gen 7 uses battle teams in box layout) | — |
| Daycare              | `0x1B200` (blk 38) | `0x1BC00` (blk 38) | `0x6B400` (blk 33) | `0x6BA00` (blk 33) |
| Hall of Fame         | `0x19400` (blk 36) | `0x19E00` (blk 36) | — | — |
| Poké Pelago (Resort) | — | — | `0x3B400` (blk 15) | `0x3B800` (blk 15) |

- **Party block** (`0x61C` bytes): six party records of `0x104` bytes each, then a `u8` party count at `+0x618`.
- **Box block**: `boxes × 30 slots × 0xE8` bytes, packed with no gaps. Slot `(box, i)` is at `Box + (box*30 + i) * 0xE8`.
  Check: Gen 6 is 31·30·0xE8 = `0x34AD0` and Gen 7 is 32·30·0xE8 = `0x36600`. Both match the block lengths.
- An empty slot reads as all zeroes. PKHeX treats a slot as empty when its decrypted species is 0 (**UNVERIFIED** detail; check `SaveFile.cs`).

Full block lists live in `Saves/Access/SaveBlockAccessor{6XY,6AO,7SM,7USUM}.cs`. Copy the tables from there when they're needed. They are long and not needed for box reading.

### MyStatus (trainer info)

Offsets are relative to the start of the block (`Saves/Substructures/Gen6/MyStatus6.cs` and `Gen7/MyStatus7.cs`):

| Field | Gen 6 | Gen 7 |
|-------|-------|-------|
| TID16 (u16) | `0x00` | `0x00` |
| SID16 (u16) | `0x02` | `0x02` |
| Game version (u8) | `0x04` | `0x04` |
| Gender (u8) | `0x05` | `0x05` |
| Language (u8) | `0x2D` | `0x35` |
| OT name (0x1A bytes, UTF-16LE, 12 chars + terminator) | `0x48` | `0x38` |

### Box layout block

- **Gen 6** (`BoxLayout6.cs`): 31 box names of `0x22` bytes each start at `0x000`. Wallpapers start at `0x41E` (1 byte per box). Flags are at `0x43D`, unlocked count at `0x43E`, and last viewed box at `0x43F`.
- **Gen 7** (`BoxLayout7.cs`): 32 box names of `0x22` bytes each start at `0x000`. Battle-team slot list is at `0x4C4`, wallpapers at `0x5C0`, flags at `0x5E0`, unlocked count at `0x5E1`, and last viewed box at `0x5E3`.

---

## Pokémon records: PK6 and PK7

Both generations use the same 232-byte (`0xE8`) stored format. Party records add `0x1C` bytes of battle stats, for `0x104` (260) bytes in total (`PKM/Util/PokeCrypto.cs`).
A PK7 has the same byte layout as a PK6. A few bytes were repurposed (listed below).

### Encryption

Records are encrypted inside the save. The encryption constant (EC) is the `u32` at `0x00`, and it is also the seed.

To decrypt (`PokeCrypto.Decrypt67`):

1. `seed = EC`. Bytes `0x00–0x07` (EC, sanity, checksum) are never encrypted.
2. **XOR**: process bytes `0x08..0xE8` one u16 at a time. Before each word, do `seed = seed * 0x41C64E6D + 0x6073` (32-bit wraparound), then `word ^= seed >> 16`.
3. Party stats `0xE8..0x104`: XOR them the same way with a **fresh** `seed = EC`.
4. **Unshuffle**: `sv = (EC >> 13) & 31`. Split `0x08..0xE8` into four 56-byte (`0x38`) blocks A, B, C, D. After unshuffling, decrypted block `i` holds the block that was at encrypted position `BlockPosition[sv*4 + i]`.
   Rows 24–31 repeat rows 0–7, which saves a `% 24`.

`BlockPosition` has 24 rows of 4 entries:

```
 0: 0 1 2 3    1: 0 1 3 2    2: 0 2 1 3    3: 0 3 1 2    4: 0 2 3 1    5: 0 3 2 1
 6: 1 0 2 3    7: 1 0 3 2    8: 2 0 1 3    9: 3 0 1 2   10: 2 0 3 1   11: 3 0 2 1
12: 1 2 0 3   13: 1 3 0 2   14: 2 1 0 3   15: 3 1 0 2   16: 2 3 0 1   17: 3 2 0 1
18: 1 2 3 0   19: 1 3 2 0   20: 2 1 3 0   21: 3 1 2 0   22: 2 3 1 0   23: 3 2 1 0
```

To encrypt, run the same steps in reverse: shuffle with `sv' = BlockPositionInvert[sv]`, then XOR.
Inverse table: `0,1,2,4, 3,5,6,7, 12,18,13,19, 8,10,14,20, 16,22,9,11, 15,21,17,23`.

**Checksum** (`PKM/Shared/G6PKM.cs`): the `u16` at `0x06` equals the 16-bit wrapping sum of the decrypted words in `0x08..0xE8`.

**Round-trip test:** decrypting and then re-encrypting must give back the original bytes exactly. This is our "store original bytes" rule: the bank should keep the **decrypted** 232 bytes plus a gen tag, and re-encrypt only when writing back.

### Key fields (decrypted)

Taken from `PKM/PK6.cs` and `PKM/PK7.cs`:

| Off | Size | Field | Notes |
|-----|------|-------|-------|
| 0x00 | u32 | Encryption constant | |
| 0x04 | u16 | Sanity | 0 when valid |
| 0x06 | u16 | Checksum | |
| 0x08 | u16 | Species (National Dex) | 0 = empty |
| 0x0A | u16 | Held item | |
| 0x0C | u32 | Trainer ID32 (TID16 at 0x0C, SID16 at 0x0E) | |
| 0x10 | u32 | EXP | |
| 0x14 | u8  | Ability | |
| 0x15 | u8  | Ability number | |
| 0x18 | u32 | PID | |
| 0x1C | u8  | Nature | |
| 0x1D | u8  | bit0 fateful encounter, bits 1–2 gender, bits 3–7 form | |
| 0x1E–0x23 | 6×u8 | EVs: HP, Atk, Def, **Spe, SpA, SpD** | Note the order |
| 0x24–0x29 | 6×u8 | Contest stats | |
| 0x2A | u8  | Markings | |
| 0x2B | u8  | Pokérus | |
| 0x30–0x36 | | Ribbon bits | 0x36 is Gen 7 only |
| 0x40 | 0x1A | Nickname (UTF-16LE) | |
| 0x5A–0x60 | 4×u16 | Moves 1–4 | |
| 0x62–0x65 | 4×u8 | PP | |
| 0x66–0x69 | 4×u8 | PP Ups | |
| 0x6A–0x70 | 4×u16 | Relearn moves | |
| 0x74 | u32 | IVs: 5 bits each HP/Atk/Def/Spe/SpA/SpD, bit 30 = egg, bit 31 = nicknamed | |
| 0x78 | 0x1A | Handling trainer name | |
| 0x93 | u8  | Current handler (0 = OT) | |
| 0xA2 | u8  | HT friendship | |
| 0xB0 | 0x1A | OT name | |
| 0xCA | u8  | OT friendship | |
| 0xD1–0xD3 | 3×u8 | Egg date (Y, M, D) | |
| 0xD4–0xD6 | 3×u8 | Met date (Y, M, D) | |
| 0xD8 | u16 | Egg location | |
| 0xDA | u16 | Met location | |
| 0xDC | u8  | Ball | |
| 0xDD | u8  | bits 0–6 met level, bit 7 OT gender | |
| 0xDE | u8  | Gen 6: ground tile. **Gen 7: Hyper Training flags** | Repurposed |
| 0xDF | u8  | Origin game version | |
| 0xE0–0xE2 | 3×u8 | Country, region, console region | |
| 0xE3 | u8  | Language | |
| *Party only* | | | |
| 0xE8 | u32 | Status condition | |
| 0xEC | u8  | Level | |
| 0xED/0xEE | u8 | Gen 6: Furfrou/Hoopa form timers. Gen 7: Poké Pelago dirt type/location | Repurposed |
| 0xEF | u8 | Gen 6 only: Super Training bag effect | Cleared on 6→7 |
| 0xF0–0xFC | 7×u16 | Current HP, max HP, Atk, Def, Spe, SpA, SpD | |

Gen 6 bytes `0x16`/`0x17` hold the Super Training bag. When PKHeX converts PK6 to PK7, it clears or changes `0x72`, `0xAA–0xAF`, `0xE4–0xE7` and `0xEF` (`PK6.cs` `ConvertToPK7`). This conversion is the "report changed fields" list for a 6→7 transfer.

Strings use UTF-16LE with a `0x0000` terminator. Gen 6/7 use some private-use code points for special glyphs (gender symbols and others). See `PKM/Strings/StringConverter6.cs` / `StringConverter7.cs`. **UNVERIFIED** detail, look before implementing.

---

## Getting the file on a 3DS

See [save-access.md](save-access.md) for file locations, title IDs, libctru calls and PKHeX setup.
The USUM layouts in this doc were checked against a real Ultra Moon save from Azahar on 2026-10-25 (size, magic, block CRCs, MyStatus, and a decrypted party PK7).

---

## Suggested first tests (`tests/`)

1. CRC-16/CCITT-FALSE(`"123456789"`) == `0x29B1`; CRC-16/USB(`"123456789"`) == `0xB4C8`.
2. Game detection by size + magic. Also check that truncated, empty, wrong-size and bad-magic input is rejected.
3. PK6 crypt round-trip on a synthetic record: build a decrypted record, fix up its checksum, encrypt, decrypt, and compare byte-for-byte. Test several EC values that hit different `sv` rows, including `sv` ≥ 24.
4. Block checksum validation on a synthetic save: zero-fill the file at the right size, write the footer, compute the CRCs, then flip one byte and expect a failure.
