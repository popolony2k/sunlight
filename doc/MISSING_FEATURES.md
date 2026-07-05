# Missing features / project gaps

This tracks project-level gaps found during a review of the build system, release
scaffolding, CI, and test/sample coverage — as opposed to `TODO.txt`/`FIXME.txt`,
which track in-engine feature/bug work.

## Real risks

- **Broken `install()` rule.** `src/CMakeLists.txt` installs `sunlightConfig.cmake`
  and `sunlightConfigVersion.cmake`, but neither file is ever generated — there is
  no `configure_package_config_file()`/`write_basic_package_version_file()` call
  and no `.cmake.in` template anywhere in the repo. `cmake --install` fails today.
- **Unpinned dependencies.** `libxml2` and `tmx` are both fetched with
  `GIT_TAG master` (unlike `raylib`, which is pinned to `5.5`). Two builds done on
  different days can silently pull different upstream commits, so builds aren't
  reproducible, and an upstream breaking change on either library can break
  `sunlight` with no warning.
- **No CI.** There is no `.github/workflows` (or any other CI) at all. A real test
  suite exists (`tests/`, 17 cases / 2058 assertions) but nothing runs it
  automatically on push or PR — regressions are only caught if someone remembers
  to build and test locally before merging.

## Missing scaffolding

- No version anywhere: `project(sunlight)` has no `VERSION`, and there are no git
  tags/releases. There's no way to tell a consumer "you're on v0.x".
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
