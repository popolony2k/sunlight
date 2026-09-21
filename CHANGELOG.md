# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
This project has not yet made a tagged release; version numbers will start
once `0.1.0` ships.

This changelog starts from the point the project adopted the practices
described in `CLAUDE.md` (backend abstraction, CI, unit tests). Earlier
history (2021–2023, the project's early prototyping phase) isn't itemized
here — see the git log for that period.

## [Unreleased]

### Fixed

- **`MoveCameraUp()` / `MoveCameraLeft()` crashed (segfault) when no map was loaded** - on the renderer
  and, through it, on any view (`IView::MoveCameraUp/Left`). Their scroll limit is worked out from the
  map's tile size, and they dereferenced the (null) map without checking. They now do nothing while no
  map is loaded. (`MoveCameraDown/Right` never touched the map and are unchanged.) Found by the Scarab
  view API work, where a script may call `camera_move_up()` before loading a map.

- **A map's external tilesets (`.tsx`) and object templates (`.tx`) are now read through
  `SunLight::FileSystem`.** libtmx opened them itself, straight from the OS relative to the working
  directory, so they bypassed the filesystem layer: a map with an external tileset failed to load
  (`cannot open extern tileset ...`) from a mounted archive with no loose copy (a `.zip` pack), and
  could never pass through the read filter (an encrypted pack - a raw OS open cannot decode it). Only
  the `.tmx` itself, and textures/sound/fonts, honoured mounts and the filter. `LoadMap` now reads
  every external reference up front through the FileSystem - resolved against the directory of the file
  that names it, `..` and backslashes normalised, a template's own tileset loaded first, the images they
  name resolved against THEIR directory - and hands them to libtmx as buffers through its Resource
  Manager, which lives as long as the map (freed by `UnloadMap`). A reference the FileSystem cannot
  provide is left to libtmx's own lookup, so a project keeping them loose beside the working directory
  works exactly as before; a map with only embedded tilesets takes the unchanged, manager-less path.
  Found and fixed on the way: libtmx's allocator hooks are only initialised by its `tmx_load*` entry
  points, not by `tmx_make_resource_manager()`, so creating the manager first (the first map in a
  process) would have crashed.

- The five samples (`sprite`, `collision`, `gamepad`, `scriptprocessor`, `tilemaprenderer`) failed to
  load their map ("Cannot load map: ... could not be read") when given an absolute base path - which
  is what the IDE launch configuration passes - ever since the resource filesystem (v0.16.0) started
  reading only inside mounted locations. They now make the given directory their working directory
  and load everything by relative name, so an absolute and a relative base path both work. (When this
  was fixed, a map's external `.tsx` tilesets were still opened by libtmx itself, straight from the OS
  relative to the working directory, which is why the working directory - not just a mount - was needed;
  that limitation is gone, see the external tilesets fix above.) Samples only: not part of any release
  archive.

### Added

- `samples/multiview` sample (`multiview_test`): the same map in three places at once - a main view, a
  minimap (`FitToMap`, sprite layer masked out, translucent background) and a close-up with its own
  camera and zoom - with keys to show/hide views, mask a layer and change draw order. Samples only: not
  part of any release archive.

- **Extra views are now DRAWN: the multi-view frame.** Each visible view gets its own pass over the
  same map into the one render target, in draw order, painter's style (a later view paints over an
  earlier one where they overlap): background cleared once, whole, as always -> for each view in
  order: make it the active one, fill its rectangle with its background (extra views only), draw the
  layers its mask lets through -> FPS counter once. A renderer that never calls `CreateView` runs the
  exact same single call as before (animation trace identical to the previous release; camera trace
  byte-identical). New on `IView`:
  - `SetVisible`/`GetVisible` (a hidden view is skipped and keeps its state; the default view can be
    hidden too), `SetDrawOrder`/`GetDrawOrder` (ascending; ties by id; the default view starts at 0, a
    new view at its own id, so extras start on top);
  - `SetClearBackground`/`GetClearBackground`, `SetBackgroundColor`, `UseMapBackgroundColor`: whether
    the view's rectangle is filled before drawing, and with what (the map's own color by default; an
    alpha < 255 blends over what is underneath). For the default view this is the frame's own
    background - the same flag as `TileMapRenderer::SetClearBackground`, which also gained the missing
    `GetClearBackground()`;
  - the **layer mask**: `ShowLayer(id | name, bool)`, `ShowOnlyLayers(ids)`, `ShowAllLayers()`,
    `IsLayerShown(id)`. A masked layer is skipped whole in that view - and so are the sprites
    registered against it (they belong to their layer), exactly like `visible="0"` but for one view.
    A masked group hides all of its children; a shown group applies the mask to each child - so
    `ShowOnlyLayers` must list a child's group as well as the child. The mask is kept by id, so it
    can be set before a map is loaded (by name needs the map);
  - `FitToMap()`: largest zoom at which the whole map fits the view + camera at the map's top-left
    (the minimap setup), computed in whole zoom steps (exact for a view that is exactly a fraction of
    the map), clamped to the zoom range; `false` with no map.
  Also `Viewport::GetZoomLimits(min&, max&)` (both INCLUSIVE, like `SetMinZoom`/`SetMaxZoom`).
  Sprites advance **once per frame** however many views draw them: the first pass that reaches a sprite
  runs `Update()`, later passes only `Draw()` (the `Advance()`/`Draw()` split of v0.30.0).
  **Limits, by design of this first version:**
  - every visible extra view costs another full pass over the map's tiles per frame (cost is N x);
  - a sprite's position is relative to the viewport that draws it (no world coordinates yet), so the
    same sprite appears at the same offset inside each view showing its layer - masks are how a view
    is given the sprites it should show;
  - a sprite that is outside the first pass's viewport does not advance that frame even if a later
    view has it on screen;
  - image layers (drawn at the screen origin, blind to camera and viewport) are drawn by the default
    view only;
  - collisions and input stay in default-view space.

- `IView::GetScrollStepSize(int&, int&)` and `TileMapRenderer::GetScrollStepSize(int&, int&)`: read a
  view's scroll step back (until now only the setter existed, so the value - including the map tile
  size a `-1` resolves to at `LoadMap` - was readable nowhere). Reads exactly what the next
  `MoveCamera*` would move by, or `-1` while it is still "the map's tile size, not known yet". Asked
  for by the Scarab view API. New pure virtual on `IView` (only `View` implements it).

- **Views (state half): every renderer now has a default view, and more can be created.** A view
  is the part of the scene's state that differs between two windows onto the same world: a
  viewport (rectangle + zoom) and a camera position + scroll step. The map, its layers and the
  sprites stay shared. New `SunLight::TileMap::IView` (`GetId`, `GetViewport`, `ZoomIn/Out`,
  `ResetZoom`, `MoveCamera*`, `SetCameraPosition`/`GetCameraPosition`, `SetScrollStepSize`,
  `ResetCamera`, `TileMapToTileMatrix`) and, on `ITileMap`/`TileMapRenderer`, `GetDefaultView()`
  (view 0: wraps the renderer's own viewport and camera, so `renderer.MoveCameraLeft()` and
  `renderer.GetDefaultView().MoveCameraLeft()` are the same operation), `CreateView(rect)` (returns
  a new id; ids are never reused), `GetView(id)`, `RemoveView(id)` (the default view cannot be
  removed) and `GetViewCount()`. A view does not reimplement camera/zoom logic: operating on one
  briefly makes its state the renderer's working state, runs the renderer's own code, and puts
  everything back - so an additional view scrolls, clamps and zooms exactly as the default one
  (tested step by step against it) and cannot drift from it. A view created before a map is loaded
  gets the map's tile size as its scroll step when `LoadMap` runs, like the default view.
  **Additional views are not drawn yet**: this release adds only their state and control; drawing
  them (with per-view layer masks, visibility, order) is the next release. Zero behaviour change
  for existing code, proven against the previous release: the recorded animation trace is
  identical and the camera/alignment trace differs only in the 42 lines already changed by the
  previous release's `toTile` fix. New pure virtuals on `ITileMap` (`GetDefaultView`, `CreateView`,
  `GetView`, `RemoveView`, `GetViewCount`) - a class implementing `ITileMap` itself must add them.

- `TextureCanvas`/`Sprite` frame step split into its two halves: `Advance()` (the STATE half -
  the animation mode's frame index, and for a `Sprite` the texture map's frame choice; once per
  frame) and `Draw()` (puts the current state on screen and changes none, so it may run several
  times for one frame - the groundwork for drawing the same scene into more than one view).
  `Canvas` gained the virtual `Advance()`/`Draw()` (no-op defaults). `Update()` is unchanged in
  behaviour: it is exactly `Advance()` then `Draw()`, and the renderer still calls it. Verified
  byte-for-byte against the previous release: 219,133 recorded lines / 41,400 draw calls over all
  six animation modes, several tile sizes, on-screen / partly off-screen / fully off-screen
  positions, clock steps, frame delays, `Reset()`, sequence switches and visibility toggles.

### Changed

- **BREAKING (semantics): a viewport's `size` is now a width/height, everywhere.** A
  viewport is the rectangle `[pos, pos + size)`: `pos` is its top-left corner and `size` its
  width and height, so `(10, 10, 1240, 900)` shows x in `[10, 1250)` and y in `[10, 910)`.
  Before, the clipping (`Viewport::GetClippedRect`, `SetPixel`) treated `size` as the
  coordinate of the FAR edge whenever `pos` was not `(0, 0)` - a legacy viewport of
  `(10, 10, 1250, 910)` is the same visible area as `(10, 10, 1240, 900)` now - while other
  code read it as a width. **Migrate a viewport with `pos != (0, 0)` by subtracting `pos` from
  `size`** (`(px, py, ex, ey)` -> `(px, py, ex - px, ey - py)`); viewports anchored at `(0, 0)`
  mean the same in both. The five samples were converted (`900 x 800` -> `890 x 790`).
  For the same visible area **everything is pixel-for-pixel identical**: clipping, `SetPixel`,
  tile/object/sprite drawing, every `LoadMap` alignment (including the "snap to a whole
  viewport height" of `BOTTOM_RIGHT`/`CENTER_WIDTH_BOTTOM`), and the `MoveCameraLeft`/`Up`
  scroll limits - verified by recording ~1.75 million draw/alignment/scroll results on real
  Caravellius maps with the legacy numbers on the previous release and the equivalent
  width/height numbers on this one, and comparing them byte for byte (unit tests keep the
  legacy code as an oracle). Only `TileMapToTileMatrix` changes, and only for viewports whose
  `pos.x != pos.y`: see the fix below.

### Fixed

- `TileMapToTileMatrix` computed the tile ROW with the viewport's `pos.x` instead of `pos.y`.
  Invisible when `pos.x == pos.y` (e.g. `(10, 10)`), wrong otherwise.

### Added

- `Viewport::GetEnableUserZoom()`, the missing getter for `SetEnableUserZoom` (the
  flag was already readable through `GetZoomProperties().bEnabledUserZoom`).
- Public zoom-scale constants in `base/viewport.h` (`SunLight::Base`): `ZOOM_STEP`
  (0.0625), `ZOOM_POS_MIN` (0), `ZOOM_POS_COUNT` (256 - the number of positions and the
  EXCLUSIVE upper bound), `ZOOM_POS_MAX` (255 - the last valid position),
  `ZOOM_POS_DEFAULT` (15, whose factor is 1.0), `ZOOM_FACTOR_MIN` (0.0625) and
  `ZOOM_FACTOR_MAX` (16.0). A position `p` has the factor `(p + 1) x ZOOM_STEP`; the
  step is a power of two, so every factor is exact in a float. `Viewport` is now
  built from these (the private `#define` copies are gone), so a consumer validating
  a zoom - e.g. a factor-based renderer creation - reads the numbers here instead of
  hardcoding a copy that can drift.

### Fixed

- `TileMapRenderer::MoveCameraUp` compared the viewport WIDTH against the vertical
  map boundary (built from map height and scroll-step height) - a copy-paste of
  `MoveCameraLeft`'s horizontal check. It now uses the viewport HEIGHT. Invisible
  for a square viewport; with a non-square one, vertical scrolling was bounded by the
  wrong dimension (a wide-but-short viewport couldn't scroll up, a tall-but-narrow one
  scrolled past the map).
- `Viewport::SetMinZoom`/`SetMaxZoom` were a one-way ratchet (each validated against the
  CURRENT limits, so limits could only ever narrow), and `SetMaxZoom( p )` stored `p` as an
  EXCLUSIVE bound, so `SetMaxZoom( ZOOM_POS_MAX )` left 254 as the last usable position -
  contradicting the position semantics of `SetZoom` and the published `ZOOM_POS_MAX`
  constant. Both now validate against the ABSOLUTE scale `[ZOOM_POS_MIN, ZOOM_POS_MAX]` (so
  limits can be widened again), both bounds are INCLUSIVE positions, a request that is off
  the scale or would cross the other limit is rejected (limits unchanged), and narrowing the
  limits clamps the current and preferred zoom (and the current factor) into the new range
  instead of leaving them outside it. **Behaviour change** (no caller anywhere used the old
  behaviour).
- `Viewport::GetZoomFactor( nZoomPos )` ignored its argument: for an in-range
  position it returned the factor of the CURRENT zoom position instead of the one
  asked for (e.g. asking for position 60 while at the default 15 returned 1.0, not
  3.8125). It now returns the factor of the requested position; an out-of-range
  position still falls back to the preferred position's factor. No caller in
  sunlight, its tests, samples or Scarab used it, so nothing changed in practice
  (the renderer and `TextureCanvas` scale by `GetZoomProperties().fZoomFactor`).

- The null backend's virtual time drifted: `VirtualClock` summed `1/fps` per frame
  (300 frames read 4.999999999999988, 5040 read 83.99999999999652 - all 300
  whole-second boundaries up to 18000 frames were inexact), so a script waiting
  for `elapsed >= N` seconds released one frame late whenever the sum undershot
  (Caravellius' 84.0 s intro sync measured 84.133 s). Frames are now COUNTED and
  time is `base + frames / fps` - one division, exact at every whole second
  (`300 / 60` is exactly `5.0`, `5040 / 60` exactly `84.0`). `NullWindow` keeps
  its own counted timeline for `GetElapsedTime`, so a restart's elapsed time is
  exact too. `VirtualClock` gained `SetFrameRate`/`AdvanceFrame`; `Advance(double)`
  remains for arbitrary steps. The tests now assert EXACT equality (they used a
  tolerance, which is why they missed it).

### Changed

- **Breaking (backend implementers only):** the window operations moved off
  `IEngine` onto a new `IWindow` (`src/window/`, with `WindowFactory` and
  `backends/raylib/RaylibWindow`): `SetFullscreen`/`GetFullscreen`,
  `SetWindowResizeable`, `SetTargetFPS`, `SetWindowTitle`,
  `GetScreenWidth`/`GetScreenHeight` and `GetElapsedTime`. `IEngine` gained
  `ClearBackground` and `DrawFPS`.
- **Breaking (backend implementers only):** `IEngine::OnWindowClosing` is gone.
  "The window is about to close" is now a window event:
  `IWindow::AddCloseHandler`/`RemoveCloseHandler`, fired by `IWindow::Close`
  right before the render context is destroyed (shared list semantics in
  `CloseHandlerList`). `RaylibEngine` subscribes to release its custom font;
  `TileMapRenderer::Stop` just closes the window. `WindowFactory::GetDefaultWindow`
  (ignores the test override) is what an engine subscribes through.
- `FullscreenStrategy` and its `FULLSCREEN_STRATEGY_*` values moved to
  `SunLight::Window`. The `IEngine::FullscreenStrategy`/`IEngine::FULLSCREEN_STRATEGY_*`
  aliases kept for one release (v0.24.0) are now removed - the only known
  consumer (Scarab) has migrated. **Breaking** for code still spelling the
  old `IEngine::` names.
- A renderer created without a viewport now gets one covering the whole render
  area, instead of a zero-sized one. `LoadMap` divides by the viewport height, so
  a renderer that never had a viewport configured crashed there with a
  divide-by-zero (SIGFPE) instead of loading the map.
- `Start()` now applies the renderer's configured exit key (`RendererConfig::exitKey`
  / the last `SetExitKey`) instead of always resetting it to ESC, so a key chosen
  before `Start()` - or before a restart - sticks. Behaviour change only for code
  that called `SetExitKey` before `Start()` and relied on it being overwritten.
- `TileMapRenderer`'s `Start()`/`Run()`/`Stop()` no longer call raylib directly
  (window create/close, exit key, should-close, begin/end frame all go through
  `IWindow`), and `tilemaprenderer.cpp` no longer includes `<raylib.h>`.

### Added

- **Null (headless) backend** - `RENDERER_BACKEND_NULL` is now real. A renderer
  created with it needs no display, GPU or windowing library, and time is virtual:
  `NullEngine` (draws nothing; `LoadTexture` returns the true PNG/JPEG size read
  from the header through `IFileSystem` - recognised by magic bytes, so a JPEG
  named `.png` works; `MeasureText` is a deterministic fixed metric, characters x
  size / 2; `SetFont` succeeds iff the file exists), `NullWindow` (never closes by
  itself; each `EndFrame` advances a `VirtualClock` by 1/target-FPS, so script
  waits and sprite/tile animation run on virtual time; `GetElapsedTime` is 0 before
  `Create`, restarts at 0 on each `Create`, 0 after `Close`), `NullInputHandler`
  (no key/button ever pressed, every axis exactly 0.0, never nullptr) and
  `NullBackend`, which installs them as one set through the engine/window/input/clock
  factories for as long as a null renderer lives (shared process-wide; the backend
  is global, so `TileMapRenderer::Create` refuses a null renderer while a default-backend
  one is live and vice versa). Audio is unaffected.
  `RendererConfig` gained `framePacing` (`FRAME_PACING_REAL_TIME` default /
  `FRAME_PACING_UNLIMITED`, null backend only) and `nMaxFrames` (any backend:
  `Run()` returns cleanly after that many frames since `Start()`; 0 = unlimited).
  `SunLight::General::VirtualClock` is the reusable frame-driven clock.
- `SunLight::General::Clock` (`src/general/clock.h`): a process-global, injectable
  source of "now" in milliseconds (`Clock::NowMilliseconds()`, `Clock::SetClock(IClock*)`;
  real monotonic clock by default). `ScriptProcessor`'s `WAIT_CMD`, `TextureMap`'s
  sprite frame timing and `TileMapRenderer`'s tile animation now read it instead of
  `steady_clock` directly, so a virtual clock (headless/test runs) can drive all of
  them at once and long waits/animations need no real time. Behaviour is unchanged
  unless a clock is installed. The real-thread `Timer` deliberately still uses real
  time. Install a clock before creating anything that timestamps against it.
- `RendererConfig` (`src/renderer/rendererconfig.h`): one value struct for
  everything a renderer needs at creation - backend enum
  (`RENDERER_BACKEND_DEFAULT`/`RAYLIB`/`NULL`/`LAST`), size, title, target FPS,
  resizeable, draw-FPS, stretch-to-fill, default key handler, exit key, view
  control mode, scroll steps, optional viewport and zoom - with `Validate()`,
  `IsBackendAvailable()` and `BackendName()`. `TileMapRenderer(const RendererConfig&)`
  builds from it; the checked `TileMapRenderer::Create(config, &error)` returns
  `nullptr` plus a message for an invalid config (e.g. a backend not compiled
  in - `RENDERER_BACKEND_NULL` is part of the contract but not implemented yet).
  The classic `TileMapRenderer(width, height, title, fps, useDefaultKeyHandler)`
  constructor is kept and delegates to it. `ViewControlMode` now lives in that header.
- `tests/mock_window.h` (`MockWindow`/`MockWindowFixture`) and
  `tests/test_tilemaprenderer_lifecycle.cpp`: `Start()`/`Run()`/`Stop()` are now
  unit-tested end to end - window creation arguments, exit key/target FPS
  defaults, frame bracketing, letterbox/stretch blit math, deferred
  `RequestExit`, teardown ordering, restart.

- `IEngine` backend abstraction (`src/engines/`) — all raylib draw calls now
  go through a swappable interface + `EngineFactory`, instead of raylib types
  leaking into `TextureCanvas`/`TileMapRenderer` headers.
- `IEngine::GetApplicationDirectory()` for resolving resource paths relative
  to the running executable instead of a hardcoded, machine-specific path.
- `IEngine::GetElapsedTime()` (raylib `GetTime()`) and its
  `IDrawSurface`/`TileMapRenderer` pass-through: high-resolution, monotonic
  wall-clock seconds, independent of the fixed per-tick `dt` — for timing
  logic that must not stretch when the game loop can't sustain its target FPS.
- `TileMapRenderer::SetWindowTitle()` for changing the window title after
  construction.
- Right-stick analog handling for `GAMEPAD_BUTTON_RIGHT_FACE_*` events,
  mirroring the existing left-stick → `LEFT_FACE_*` virtual DPad dispatch.
- `samples/collision` — demonstrates `CollisionManager`, `AddColliderToColliderRule`,
  and the `ICollisionListener` callback with a player-controlled sprite.
- `samples/gamepad` — demonstrates independent left/right analog stick control
  of two separate sprites on one connected gamepad.
- GitHub Actions CI (`.github/workflows/ci.yml`) building the library, both
  samples, and the test suite on Linux, macOS, and Windows on every push and
  PR against `main`; the three `build (<os>)` jobs are required status checks.
- Unit test suite (`tests/`, via [doctest](https://github.com/doctest/doctest)),
  covering:
  - Pure logic: `Viewport`, `Collider`, `Helper`, `base/primitives.h`.
  - `SoundManager`, via `SoundFactory::SetCreator()` and a `MockSound` test
    double — no real audio device involved.
  - `TextureCanvas` and `Sprite`, via `EngineFactory::SetEngine()` and a
    `MockEngine` test double — no real window/render context involved.
  - `CollisionManager`, via a `MockTileMap` test double.
  - `ScriptProcessor`, covering command queue draining, listener dispatch,
    `WAIT_CMD`/`WAIT_SPRITES_QUEUE_EMPTY` blocking, and `LOOP_CMD`/
    `GOTO_LABEL_CMD` control flow.
  - `TileMapRenderer`'s pre-`Start()` contract (the parts reachable without a
    real window).
- `SoundFactory`/`EngineFactory::SetEngine()` test-only override hooks, giving
  `SoundManager`/anything routing through `IEngine` a seam to substitute a
  mock backend.
- `cmake --install` now generates a real `sunlightConfig.cmake`/
  `sunlightConfigVersion.cmake` and installs the `sunlight` target, its
  headers, and the vendored `raylib`/`tmx` runtime libs.
- `doc/MISSING_FEATURES.md` tracking project-level gaps (scaffolding, CI,
  test/sample coverage) as they're found and closed.
- Real usage docs for the `sprite` and `tilemaprenderer` samples (previously
  stub placeholders).

### Changed

- `libxml2` and `tmx` dependencies pinned to stable release tags
  (`v2.15.3`/`tmx_1.10.1`), matching `raylib` (`5.5`), instead of tracking
  `master`.
- Internal containers/members that exclusively own heap-allocated data
  converted from raw `new`/`delete` to `std::unique_ptr` throughout the
  library (`Sprite::m_Sequences`, `CollisionManager`'s rule lists,
  `SoundManager::m_SoundMap`, `TileMapRenderer`'s input handler and event
  handler lists, and others).
- VSCode sample settings use the `VCPKG_ROOT` environment variable instead of
  a hardcoded vcpkg path.

### Fixed

- Broken `install()` rule — `cmake --install` previously produced a package
  config that couldn't actually be consumed.
- `sunlightConfig.cmake.in` path resolution when `sunlight` is consumed via
  `FetchContent` from another project (was using `CMAKE_SOURCE_DIR` instead of
  `CMAKE_CURRENT_SOURCE_DIR`).
- `TextureCanvas::Load()`'s auto-size-on-load guard checked
  `dimension.size.nHeight == 0` twice instead of checking `nWidth` and
  `nHeight` — harmless when both start at zero (the common case), but it
  could silently clobber a caller-set width if only height was left at zero.
- `ScriptProcessor`'s `LOOP_CMD` unconditionally reset its repeat counter to
  `0` on every run and never read the requested repeat count at all — a loop
  body always ran exactly once regardless of the count passed in.
- `ScriptProcessor`'s `GOTO_LABEL_CMD`/`END_LOOP_CMD` dereferenced their label
  lookup without checking for a miss, crashing on a forward reference or a
  typo'd/unmatched label id. Now reports the failure through
  `IScriptListener::OnError()` instead.
- A dead `HandleGamePadEvent` declaration with no implementation, left over
  from an earlier refactor.
- `SunLight::Input::KeyboardKey` vs. raylib's own global `KeyboardKey` name
  collision in `TileMapRenderer::SetExitKey`.

### Removed

- Nothing yet.
