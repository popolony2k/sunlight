# Missing features / project gaps

This tracks project-level gaps found during a review of the build system, release
scaffolding, CI, and test/sample coverage — as opposed to `TODO.txt`/`FIXME.txt`,
which track in-engine feature/bug work.

## Resolved

- ~~No CI~~ — `.github/workflows/ci.yml` builds the library, both samples, and
  the test suite, then runs `sunlight_tests` via `ctest`, on Linux/macOS/Windows,
  on every push to `main` and every PR. The three `build (<os>)` jobs are also
  required status checks on `main`'s branch protection. (#40)
- ~~Unpinned dependencies~~ — `libxml2` (`v2.15.3`) and `tmx` (`tmx_1.10.1`) are
  now pinned to stable release tags, matching `raylib` (`5.5`). (#41)
- ~~Broken `install()` rule~~ — `cmake --install` now actually generates
  `sunlightConfig.cmake`/`sunlightConfigVersion.cmake`, installs the `sunlight`
  target, its headers, and the vendored `raylib`/`tmx` runtime libs, and a
  `sunlight::sunlight` imported target is hand-declared in
  `cmake/sunlightConfig.cmake.in` (avoiding `install(EXPORT)`'s transitive
  export-set requirement, which vendored FetchContent targets aren't built for).
  `project(sunlight)` also gained a `VERSION 0.1.0`, needed for the generated
  ConfigVersion file. Verified end-to-end against a standalone external consumer
  project. (#42)
- ~~No `SoundManager` test coverage~~ — `SoundFactory::CreateSound()` now isolates
  `SoundManager` from the concrete `RayLibSound` backend (#52), and `SoundFactory::SetCreator()`
  gives tests a seam to substitute a mock `ISound`. `tests/test_soundmanager.cpp` covers
  `Load`/`Unload`/`Play`/`Stop`/`Pause`/`Resume`/`IsPlaying`'s id-keyed dispatch and
  bookkeeping against a `MockSound` test double, with no real audio device involved.
- ~~No `IEngine`-mocking seam~~ — `EngineFactory::SetEngine()` overrides what
  `GetEngine()` returns (tests only), letting a mock `IEngine` stand in without a
  real window/render context. `tests/test_texturecanvas.cpp` is the first consumer:
  it covers `TextureCanvas::Load`/`Unload`/`Update` against a `MockEngine` test
  double (texture handle plumbing, size adoption on load, and draw dispatch gated
  on visibility/viewport clipping).
- ~~No `CollisionManager` test coverage~~ — `tests/mock_tilemap.h` adds `MockTileMap`,
  a stub `ITileMap` implementing only what `CollisionManager` actually calls on its
  parent (`GetLayer`, `TileMapToTileMatrix`, `GetTile`). `tests/test_collisionmanager.cpp`
  covers layer-id validation on `AddCollider`/`AddColliderToColliderRule`/
  `AddColliderToTileRule`, `Update()`'s collider-to-collider dispatch (fires only for
  overlapping colliders on a paired layer, respects `RemoveCollider`), and the
  collider-to-tile path's failure/no-op branches, including a real (but
  collision-shape-less) `tmx_tile` to exercise `Collider::Hit(stTile&)` safely.
- ~~No `Sprite` test coverage~~ — `tests/test_sprite.cpp` covers `AddTextureSequence`'s
  dimension-adoption/aliasing (a sprite's dimension is shared with each texture added
  to it via `SetDimension2DPtr`, not copied), `SetActiveTextureSequence`/
  `GetActiveTextureSequence`/`GetActiveTexture`'s valid/unknown-id handling,
  `SetVisible`/`Move`, and `Update`/`Unload`'s dispatch to the active/every texture
  via a `MockEngine`. Caught along the way: `BaseCanvas::GetViewport()` delegates to
  a parent's viewport once one is set (`AddTextureSequence` parents each texture
  under its `Sprite`), so the viewport must be configured through the sprite (or on
  the texture after parenting) — configuring it on a freshly constructed,
  not-yet-added `TextureCanvas` silently targets the wrong object once it's added.
- ~~No `ScriptProcessor` test coverage~~ — `tests/test_scriptprocessor.cpp` covers
  queue draining, `WAIT_CMD` blocking until its delay elapses, listener dispatch
  (and the no-listener-attached no-op case), `WAIT_SPRITES_QUEUE_EMPTY`/
  `ResetWaitSpritesQueueEmptyCmd`, `GOTO_LABEL_CMD` backward jumps, and
  `LOOP_CMD`/`END_LOOP_CMD` repetition. Writing these tests surfaced two real bugs,
  both fixed:
  - `LOOP_CMD` unconditionally reset its repeat counter to `0` every time it ran,
    and never read `TwoParmsCommand::nParm2` (the requested repeat count) at all -
    a loop body always ran exactly once regardless of the count passed in. Fixed
    to seed the counter from `nParm2`.
  - `GOTO_LABEL_CMD`/`END_LOOP_CMD` dereferenced their label lookup without
    checking for a miss, crashing on a forward reference or a typo'd/unmatched
    label id. Fixed to guard the lookup and report the failure through
    `IScriptListener::OnError()` instead - previously dead code, since nothing in
    `ScriptProcessor` ever called it.
- ~~No `TileMapRenderer` test coverage~~ (partial) — `tests/test_tilemaprenderer.cpp`
  locks in the documented pre-`Start()` contract: `LoadMap`/`AddSprite`/`RemoveSprite`
  (which all gate on `m_bIsStarted`) and `UnloadMap`/`GetMapInfo`/`GetLayer`/`SetLayer`/
  `TileMapToTileMatrix` (which all need a loaded `m_pTmxMap`) fail gracefully rather
  than crash when called too early, plus `GetInputHandler`/`GetCollisionManager`
  return stable, usable references standalone. Investigated and ruled out testing
  the map-loading pipeline itself (parsing a real `.tmx` through a `MockEngine`,
  no rendering needed): `LoadMap()` returns `false` immediately unless `m_bIsStarted`
  is already `true`, which only happens after `Start()`'s real `InitWindow()` call
  succeeds - unreachable on a headless CI runner, so that slice was scoped out
  rather than built on a foundation that can't run in CI.
- ~~No `CONTRIBUTING.md`, `SECURITY.md`, or `CHANGELOG.md`~~ — `CONTRIBUTING.md`
  covers building/testing, the code conventions from `CLAUDE.md`, and the PR
  workflow. `SECURITY.md` points reporters at GitHub's private vulnerability
  reporting (enabled on the repo as part of this change) instead of a public
  issue. `CHANGELOG.md` follows Keep a Changelog, starting from the point the
  project adopted these practices (`main` is unreleased, so everything so far
  lives under `[Unreleased]`).

## Missing scaffolding

- No git tags/releases yet — `project(sunlight VERSION 0.1.0)` exists now (added
  while fixing the `install()` rule), but there's still no tagged release, so
  there's no way to tell a consumer "you're on v0.x" from git history alone.
- No Doxygen config. Every header already uses consistent `@brief`/`@param`
  doxygen-style comments, but none of it is ever rendered into browsable docs.
- No `.gitattributes`, despite the project explicitly supporting
  Windows/Mac/Linux — no line-ending normalization across platforms.

## Test / sample coverage gaps

- Tests cover pure-logic code (`Viewport`, `Collider`, `Helper`, `base/primitives.h`,
  `ScriptProcessor`), `SoundManager` (via a mock `ISound`), `TextureCanvas`, `Sprite`
  (both via a mock `IEngine`), and `CollisionManager` (via a mock `ITileMap`, see
  above). `TileMapRenderer` has only thin coverage (its pre-`Start()` guard
  contract, see above) - the map-loading pipeline and everything past it needs a
  real window (`Start()`'s `InitWindow()` call), so it's unreachable without a
  display, and `Run()`'s input/update/collision dispatch loop is `private`/`inline`,
  only reachable through that same real window loop.
- No sample demonstrates `SoundManager` or `ScriptProcessor` (`samples/tilemaprenderer`,
  `samples/sprite`, and `samples/collision` cover map rendering, sprites/animation,
  and `CollisionManager` respectively).

## Already-tracked backlog (see `TODO.txt` / `FIXME.txt`)

- Scroll-boundary access violation, isometric/hexagonal map support, per-layer
  parallax, per-object property management are still open.
- Only one `IEngine` backend exists (raylib) — the README's "aims to extend to
  SDL" goal is still aspirational, though architecturally cheaper now than
  before the engine abstraction.
