# Handoff to Scarab

This is the brief the Scarab session starts from: what changed in sunlight, why, and how Scarab
works through it. It was written in the sunlight session with the owner, and the owner approved it
before it was sent (item H3 to H5 of sunlight's [`doc/MASTER_PLAN.md`](MASTER_PLAN.md)).

It lives in sunlight's `feat/master_plan` branch, next to the files it points to. A local checkout of
sunlight sits beside Scarab's (`../sunlight` from Scarab's root); Scarab's own build also fetches it
under `build/_deps/sunlight-src`. Like [`doc/BREAKAGES.md`](BREAKAGES.md), this file is integration
material only: it is removed before `feat/master_plan` goes to `main`.

## 1. The integration

Three projects are being brought up to date together, one after the other:

1. **sunlight** ran its own master plan on the branch `feat/master_plan`. It is finished, and that
   branch is now the **integration base**.
2. **Scarab** runs its own master plan next, on its own `master_plan` branch, built against sunlight's
   `feat/master_plan` - not against a sunlight release.
3. **Caravellius** does the same after Scarab, against both.

Nothing goes to any project's `main`, and no release is cut, until the whole integration is done.
Changes Scarab needs in sunlight while it works land on sunlight's `feat/master_plan` too (see 8).

## 2. What changed in sunlight, and why

Scarab pins sunlight `0.38.0` today (`SCARAB_SUNLIGHT_VERSION`, root `CMakeLists.txt`). sunlight's master
plan started right after `v0.38.0`, so everything below is new to Scarab. In short, by phase (each one
is described in full, with its reasons and tests, in `doc/MASTER_PLAN.md`):

| Phase | What | Why |
|---|---|---|
| A | An AddressSanitizer CI job; non-orthogonal maps refused clearly; key codes matched to raylib by name; `ScriptProcessor` deleting commands safely | Correctness first: ASan found real memory bugs as soon as it ran |
| B | Tiled text and tile objects drawn; unknown object/layer types warned about | Maps drew only part of what Tiled saves |
| C, D | Thick lines, polylines, polygons, rectangles, ellipses and points; every object can be rotated | Tiled's object layers as authored |
| E | Isometric, staggered and hexagonal maps, behind a projection seam, with visible-tile culling | Maps other than orthogonal |
| F | Input moved to a `ControllerManager`; default camera keys made opt-in (`DefaultControllerBindings`); pad list and hot-plug; player slots; an action map with rebinding and save/load; dead zones; gamepad rumble through SDL2 | A game owns its input; pads beyond "pad 0" |
| L | Per-layer camera and parallax; per-object opacity; sprite draw order within a layer | Tiled's parallax and opacity; predictable sprite order |
| M | The renderer as a drawing surface with no map: shapes and sprites without `LoadMap` | Menus, title screens, tools |
| S | `TileMapRenderer` split into independent classes (`MapData`, `LayerRenderer`, `SpriteRegistry`, `Camera`, ...) | A renderer of 3,650 lines; parts reusable on their own |
| T | Doxygen clean, map-loading and frame-loop tests, CHANGELOG, README and samples, accented fonts | The library documented and tested as it is |
| H | This handoff: a code style guide, sunlight aligned to it, this brief | Scarab next |

**[`doc/BREAKAGES.md`](BREAKAGES.md) is the authoritative list** of what affects Scarab and Caravellius:
49 entries, each with what changed, what to check and what to do. The ones that hit code Scarab uses
today, found by reading Scarab's `src/` (2026-10-14, read only):

| Change | Where Scarab is hit |
|---|---|
| The default camera keys are opt-in (F0b); `MoveCamera*` now scroll all the way to the map's edge, the last step a partial one (entry 46) | 32 `MoveCamera*` calls |
| `stDimension2D`, `stSize2D`, `stCoordinate2D` moved from `SunLight::TileMap` to `SunLight::Base` | 11 places |
| A layer's `nOpacity` is 0..255, 255 opaque (it read 256 for an opaque layer before) | 8 places |
| `RendererConfig::bUseDefaultKeyHandler` removed; the classic constructor lost its fifth argument | 4 places |
| `Sprite::Unload()` leaves no sequence active (entry 47); the renderer's `Stop()` unloads every registered sprite | 3 `Unload` calls |
| `AddSprite` | 2 calls |

Scarab does not implement `IEngine`, does not call `SetPixel` and does not use the haptics or parallax, so those
entries need nothing from it. Go through every entry anyway: this table is a starting point, not a
substitute.

## 3. How Scarab works through it

Scarab's own master plan, planned with the owner before anything is written (see 6). Its first items
are expected to be:

1. **Build against `feat/master_plan`, pinned to one commit.** Scarab's `FetchContent` pins
   `GIT_TAG v${SCARAB_SUNLIGHT_VERSION}` today. `GIT_TAG` also takes a commit hash, and that is the
   way agreed with the owner:
   - **Pin the exact commit given in the handoff message** - the full 40-character hash of
     `feat/master_plan` when this brief was sent (it cannot be written here: a commit cannot hold its
     own hash). Confirm it in the local checkout first (`git -C ../sunlight log -1 <hash>`).
     Reproducible: every build and CI run gets the same sunlight. When sunlight's branch moves, the
     new hash comes through the owner, and Scarab bumps its pin on purpose, in its own reviewed
     commit - as it bumps the version today.
   - Not the branch name (`GIT_TAG feat/master_plan`): every configure would fetch whatever the
     branch holds that day, so the same Scarab commit could build different sunlight code.
   - For local work against sunlight changes not yet pushed:
     `cmake -B build -S . -DFETCHCONTENT_SOURCE_DIR_SUNLIGHT=../sunlight` builds against the local
     checkout, with no fetch (a standard `FetchContent` option).
   - `scarab --version` prints `sunlight v<version>` from the same variable. With a commit pinned
     there is no release to name, so it should print the branch and the short hash
     (`sunlight feat/master_plan@<short hash>`) - how, is part of planning this item.
2. **Work through `BREAKAGES.md`**, entry by entry, each one fixed or marked as not applying.
3. **The requisites in 7**: an AddressSanitizer CI job and the first unit tests.
4. **The code style phase (4).**

## 4. Code style

sunlight's code is the reference for Scarab's: [`doc/CODE_STYLE.md`](CODE_STYLE.md), taken from the
owner's own code, with the owner's choices where it varied. sunlight's code was aligned to it (H2).

Scarab already shares most of it (spaced `::`, `m_` and the Hungarian prefixes, `st` structs, upper-case
enumerators, Doxygen comments, no magic numbers, the zlib header). Its `CLAUDE.md` says to match the
existing style and not "fix" it; **that rule changes: the guide is the reference now**, and Scarab's
`CLAUDE.md` should say so.

Measured against the guide (read only, 48 files, 11,525 lines): about 457 pointers to `Type* p`, 70
references to `Type& r`, 23 `#endif /*`, 12 braces, and by hand 28 braces on their own line, 9 `->`
without spaces and 5 `using namespace`. Scarab's plan gets a style phase: an audit against the guide,
then the alignment, one item and one PR at a time. The way sunlight did it (H2 in its plan): a script
that touches code only and stays within a line, and the proof that every object file is identical
byte for byte before and after.

## 5. Branches

The same strategy as sunlight's:

- One feature branch and one PR per item, into the integration branch.
- A large phase gets a staging branch, `feat/complete_<phase>_plan`, created from `master_plan`: its
  items' PRs go into it, and one final PR closes the phase into `master_plan`.
- Squash merges.
- A staging branch is added to CI's branch list while its phase runs, and dropped right after the
  phase merges.

## 6. Working rules

- **Planning is the strongest phase of the whole plan.** Every phase and every item is designed and
  discussed with the owner before any code is written - even inside a phase already approved. The
  design is shown first; the work starts on the owner's go-ahead.
- **Nothing is committed, pushed or opened as a PR without the owner's review** - integration
  branches and small config commits included. Show the change and its checks, then wait. Once the
  owner approved a PR, merging it on green CI needs no new ask.
- Tests first: a bug fix comes with a test shown to fail on the old code, then pass on the fix.
- The full suite, the sanitizer and the leak check before calling anything done.
- Every claim in a comment is checked (read the library's source, measure it) - a wrong "why" is
  worse than none.
- Real-window checks are done by the owner.

## 7. Three requisites from sunlight's experience

- **An AddressSanitizer job in Scarab's CI, at least on Linux** - like sunlight's
  "AddressSanitizer (Linux)" job in its `.github/workflows/ci.yml`. Linux only for CI time alone: a
  sanitized build on every runner would slow every PR. Windows and macOS can be added later if
  needed. On Linux it also runs LeakSanitizer, which macOS's AddressSanitizer does not.
- **Unit tests for every module that can have them**, as sunlight has (doctest, with mocks and a
  null backend where a window or a device is involved). Scarab has none today. Many of sunlight's
  memory bugs were found only by writing such tests and running them under the sanitizer:
  a heap-buffer-overflow in the tile lookup (A6); `LoadMap` over a loaded map leaking the old one
  (T2); listener lists walked while a callback appended to them, undefined behaviour (T3);
  `Sprite::Unload` leaving a dangling sequence iterator, a use-after-free (S1d); a mapless
  `AddSprite` leak caught by CI's LeakSanitizer (M2).
- **A sample for every new visual feature**, as Scarab already has samples: a visual feature cannot
  be fully checked by unit tests, so the owner checks it in a real window through its sample before
  the item is closed.

## 8. When Scarab finds a sunlight problem

Report it to the owner, who decides where and when it is fixed. The sunlight session holds after
sending this brief and does nothing on its own; the owner brings work to it when needed.

## 9. sunlight commits to pin

Every `feat/master_plan` commit handed to Scarab to pin, newest first. Each later one only adds to
the one before it; `doc/BREAKAGES.md` says what each change affects.

| Commit | What it adds | BREAKAGES |
|---|---|---|
| `4274ecbf6cdd71c3625254f68ae14e0ed6631acb` | Layer tint at runtime (`stLayer::tint`); group layers' tint and opacity reach their content; image layers apply their opacity (I8, asked for by Scarab in its V12) | 57 |
| `4189199f7b3af75e590792f85d51051916f4a206` | Tile collisions and tile picking in map space; `GetCollidingTiles`; one callback per tile touched (I7, found by Scarab in its V11) | 56 |
| `a6ac4f6bea9973795c1668dd8eabfc67defc03c1` | `UnloadMap()`/`Stop()` take the unloaded sprites' colliders out (I6, found by Scarab in its V9) | 55 |
| `fa953f38cb22508247b528c8234ad2bb0f4293e8` | `CollisionManager` checks every layer id; `Clear()` and `RemoveAll()` work (I5, found by Scarab) | 54 |
| `f651accc2291a2ea9020dad3bfc2f1245e76858f` | A map's embedded tileset image path with `..` or `.` loads (I4, found by Scarab in its V6) | 53 |
| `9c8c6bf421a1147d1e8c099e698dfbe7c1885af2` | Fonts by handle: `LoadFont` once, then `SetFont` / `RegisterFont` / `UnregisterFont` with the handle (I3). Breaks `set_font`/`register_font` on purpose; sent with a guide to the new model | 52 |
| `bfce5983370fa0661c18701164a10f7e18f1e7a1` | Blend modes: normal, add, multiply, subtract, for a game's own drawing and per sprite (I2). Also a new item for Scarab's plan, sent with it | 51 |
| `a63fec6811ba28fb46a94015d05b852393016692` | Semi-transparent drawing no longer darker than authored (I1, found by Scarab) | 50 |
| `772851ce7120c0d4f25dbf72c8d15f7e54bcd1d5` | The handoff (H5): sunlight's master plan, finished | 1 to 49 |

## 10. Open points in sunlight, for context

- Objects ignore Tiled's `draworder` and are drawn in reverse file order.
- The views still live inside `TileMapRenderer` (the largest piece left after S).
