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
- `Start()` now applies the renderer's configured exit key (`RendererConfig::exitKey`
  / the last `SetExitKey`) instead of always resetting it to ESC, so a key chosen
  before `Start()` - or before a restart - sticks. Behaviour change only for code
  that called `SetExitKey` before `Start()` and relied on it being overwritten.
- `TileMapRenderer`'s `Start()`/`Run()`/`Stop()` no longer call raylib directly
  (window create/close, exit key, should-close, begin/end frame all go through
  `IWindow`), and `tilemaprenderer.cpp` no longer includes `<raylib.h>`.

### Added

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
