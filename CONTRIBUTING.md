# Contributing to sunlight

Thanks for your interest in contributing! This document covers the practical
steps for building, testing, and submitting changes.

## Building

Requires CMake 3.24+ and system zlib. Unless `-DUSE_LOCAL_LIBXML2` is passed,
libxml2/raylib/tmx are fetched automatically via `FetchContent`. See the
[README](README.md#requirements-memo) for platform-specific dependency setup
(vcpkg on Windows, raylib's own Linux dependencies, etc.).

```shell
cmake -B build -S .
cmake --build build --target sunlight -j 4
```

To build the samples and/or run the test suite locally:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON -DBUILD_LIBRARY_TESTS=ON
cmake --build build -j 4
ctest --test-dir build
```

`src/CMakeLists.txt` globs all `.cpp`/`.h` files under `src/` recursively — after
adding a new source file, re-run `cmake -B build -S .` so the glob picks it up.

## Before submitting a PR

- **Run the test suite** (`ctest --test-dir build` or
  `./build/tests/sunlight_tests`) and confirm it's still green. Add test
  coverage for new logic where practical — see `tests/mock_engine.h`,
  `tests/mock_sound.h`, and `tests/mock_tilemap.h` for the mocking seams
  (`EngineFactory::SetEngine()`, `SoundFactory::SetCreator()`) already
  available for code that touches `IEngine`/`ISound`.
- **Reset `BUILD_LIBRARY_SAMPLES`/`BUILD_LIBRARY_TESTS` to `OFF`** before
  committing if you turned them on locally — they default to `OFF` and PRs
  shouldn't change that.
- Keep commits focused; a bug fix doesn't need an unrelated refactor riding
  along with it.

## Code conventions

These are the patterns the existing code already follows — matching them keeps
the codebase consistent:

- **One namespace per module**, mirroring the directory layout:
  `SunLight::<Module>` (e.g. `SunLight::Renderer`, `SunLight::Collision`).
- **Backend abstraction pattern**: any subsystem that touches raylib directly
  follows the shape `IThing` (interface) + `ThingFactory` (owns the one
  `#if DEFAULT_ENGINE == 1 ... #else #error` switch) + `thing/raylib/RaylibThing`
  (concrete implementation). See `IEngine`/`EngineFactory` as the reference
  instance of this pattern. Call sites elsewhere should never need raylib types
  directly.
- **Pointer ownership**: containers/members that exclusively own heap-allocated
  data use `std::unique_ptr`, not raw `new`/`delete`. Raw pointers are reserved
  for genuinely non-owning references (back-pointers, externally-owned objects
  registered into a manager, handles crossing a C API boundary like tmx
  callbacks). When adding a new owning member, prefer `unique_ptr`.
- **Native geometry/texture types**: prefer `base/primitives.h`'s
  `stRectangle`/`stVector2D`/`TextureHandle` over raylib's own `Rectangle`/
  `Vector2`/`Texture2D` in new backend-agnostic code.
- **`SunLight::Input::KeyboardKey` gotcha**: it's a distinct enum from raylib's
  own global `KeyboardKey`, with matching numeric values but no
  `using namespace SunLight::Input`. An unqualified `KeyboardKey` inside
  `namespace SunLight::Renderer` silently resolves to whichever one happens to
  be visible via prior `#include`s. Always fully qualify as
  `SunLight::Input::KeyboardKey` in new code.

See `CLAUDE.md` for the full architecture writeup (module map, known issues,
git workflow) if you're making a larger change.

## Submitting changes

1. Fork the repository (or push a branch directly if you have write access).
2. Open a pull request against `main` describing what changed and why.
3. Make sure the CI checks (`build (ubuntu-latest)`, `build (macos-latest)`,
   `build (windows-latest)`) pass — they build the library, both samples, and
   the test suite on all three platforms.
4. `main` requires at least one approving review before merging.

## Reporting bugs / requesting features

Open a GitHub issue. For security vulnerabilities, see [SECURITY.md](SECURITY.md)
instead — please don't report those in a public issue.
