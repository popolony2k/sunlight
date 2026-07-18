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

## Missing scaffolding

- No git tags/releases yet — `project(sunlight VERSION 0.1.0)` exists now (added
  while fixing the `install()` rule), but there's still no tagged release, so
  there's no way to tell a consumer "you're on v0.x" from git history alone.
- No `CONTRIBUTING.md`, `SECURITY.md`, or `CHANGELOG.md`.
- No Doxygen config. Every header already uses consistent `@brief`/`@param`
  doxygen-style comments, but none of it is ever rendered into browsable docs.
- No `.gitattributes`, despite the project explicitly supporting
  Windows/Mac/Linux — no line-ending normalization across platforms.

## Test / sample coverage gaps

- Tests cover pure-logic code (`Viewport`, `Collider`, `Helper`, `base/primitives.h`),
  `SoundManager` (via a mock `ISound`), `TextureCanvas` (via a mock `IEngine`), and
  `CollisionManager` (via a mock `ITileMap`, see above). `TileMapRenderer`, `Sprite`,
  and `ScriptProcessor` still have zero test coverage - the `MockEngine`/
  `EngineFactory::SetEngine()` seam exists for `Sprite` too (it just forwards to its
  active `TextureCanvas`), but `TileMapRenderer` also owns its window lifecycle
  (`InitWindow`, `BeginDrawing`/`EndDrawing`, ...) directly via raylib, so only the
  parts of it that route through `IEngine` are unlockable this way, not the whole class.
- No sample demonstrates `SoundManager` or `ScriptProcessor` (`samples/tilemaprenderer`,
  `samples/sprite`, and `samples/collision` cover map rendering, sprites/animation,
  and `CollisionManager` respectively).

## Already-tracked backlog (see `TODO.txt` / `FIXME.txt`)

- Scroll-boundary access violation, isometric/hexagonal map support, per-layer
  parallax, per-object property management are still open.
- Only one `IEngine` backend exists (raylib) — the README's "aims to extend to
  SDL" goal is still aspirational, though architecturally cheaper now than
  before the engine abstraction.
