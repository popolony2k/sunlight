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

### Added

- `IEngine` backend abstraction (`src/engines/`) — all raylib draw calls now
  go through a swappable interface + `EngineFactory`, instead of raylib types
  leaking into `TextureCanvas`/`TileMapRenderer` headers.
- `IEngine::GetApplicationDirectory()` for resolving resource paths relative
  to the running executable instead of a hardcoded, machine-specific path.
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
