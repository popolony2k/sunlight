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

- Tests only cover pure-logic code (`Viewport`, `Collider`, `Helper`,
  `base/primitives.h`). `TileMapRenderer`, `Sprite`, `CollisionManager`,
  `SoundManager`, and `ScriptProcessor` have zero test coverage.
  Now that `IEngine` is a real interface, a mock `IEngine` implementation could
  unlock testing `TileMapRenderer`'s input/collision/sprite-dispatch logic
  without a real window — that wasn't possible before the engine abstraction.
- No sample demonstrates `SoundManager` or `ScriptProcessor` (only tilemap
  rendering and sprites are shown, via `samples/tilemaprenderer` and
  `samples/sprite`).

## Already-tracked backlog (see `TODO.txt` / `FIXME.txt`)

- Scroll-boundary access violation, isometric/hexagonal map support, per-layer
  parallax, per-object property management are still open.
- Only one `IEngine` backend exists (raylib) — the README's "aims to extend to
  SDL" goal is still aspirational, though architecturally cheaper now than
  before the engine abstraction.
