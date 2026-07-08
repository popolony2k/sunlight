# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`sunlight` is a C++17 2D game engine library (zlib license) for building tile-map based games. It wraps [raylib](https://www.raylib.com/) for rendering/input/audio and [libtmx](https://github.com/baylej/tmx.git) for loading Tiled `.tmx`/`.tsx` maps, exposing a manager-style API for maps, sprites, collisions, and graphics primitives.

## Build

Requires CMake 3.24+, system zlib, and (unless `-DUSE_LOCAL_LIBXML2` is passed) libxml2/raylib/tmx are fetched automatically via `FetchContent` (raylib pinned to `5.5`; libxml2/tmx track `master`). On Linux, raylib's own system dependencies must be installed first (see raylib's wiki).

```shell
cmake -B build -S .
cmake --build build --target sunlight -j 4
```

Samples are opt-in (`OFF` by default):

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This builds `sprite_test` and `tilemaprenderer_test` and copies the raylib/tmx/LibXml2/sunlight shared libs next to each executable (required at runtime, via the `*_copy_binaries` custom targets). Each sample takes its own directory as `argv[1]` and resolves resource paths relative to it, e.g.:

```shell
./build/samples/tilemaprenderer/tilemaprenderer_test samples/tilemaprenderer/
```

Unit tests are opt-in too (`OFF` by default), using [doctest](https://github.com/doctest/doctest) fetched via `FetchContent`:

```shell
cmake -B build -S . -DBUILD_LIBRARY_TESTS=ON
cmake --build build --target sunlight_tests -j 4
./build/tests/sunlight_tests        # or: ctest --test-dir build
```

Tests only cover logic that doesn't need a window/display (`Viewport` zoom/clipping, `Collider` AABB overlap, `Helper::Random`, `base/primitives.h`) — anything touching `IEngine`/raylib directly isn't unit-testable this way. There is no linter/formatter config in the repo.

For a debug build: `cmake -B build -S . -DCMAKE_C_FLAGS="-g2" -DCMAKE_CXX_FLAGS="-g2"` (or the `SUNLIGHT_DEBUG_SYMBOLS_UNIX` option, on by default on Mac/Linux).

## Architecture

The core library lives in `src/`, one namespace per module under `SunLight::<Module>`. `src/CMakeLists.txt` globs all `.cpp`/`.h` recursively into a single `sunlight` target — re-run `cmake -B build -S .` after adding new source files so the glob picks them up.

### Module map
- `base` — `Object` (opaque `void*`-holding base class), `GraphicObject`, `Viewport`, `stColor`, and `primitives.h` (`stRectangle`, `stVector2D`, `TextureHandle` — backend-agnostic geometry/handle types).
- `tilemap` — interfaces/POD structs describing TMX map data (`ITileMap`, `stLayer`, `stTile`, `stDimension2D`, ...); pure shared types, no implementation.
- `renderer` — `TileMapRenderer`, the engine's central class: owns the window/game loop, camera/viewport, input dispatch, sprite map, tile animation, and low-level drawing primitives (Bresenham line, midpoint ellipse, polygon).
- `collision` — `CollisionManager`: layer-based collider rules (`ColliderToColliderRule`, `ColliderToTileLayerRule`) with an observer-style `ICollisionListener`.
- `canvas` / `sprite` — `Canvas`/`TextureCanvas` (single animated texture with tiling/zoom-aware blit) and `Sprite` (named sequences of `TextureCanvas`s, e.g. walk-cycle frames).
- `input` — `IInputHandler` + `InputHandlerFactory`, backed by `input/raylib/RaylibInputHandler`.
- `sound` — `SoundManager` behind `ISound`, backed by `sound/raylib/RayLibSound`.
- `engines` — `IEngine` (`SetPixel`, `DrawTexture`, `DrawTextureTiled`, `LoadTexture`/`UnloadTexture`, `GetApplicationDirectory`) + `EngineFactory::GetEngine()`, backed by `engines/raylib/RaylibEngine`. The seam through which the actual raylib draw calls happen.
- `scripting`, `concurrent`, `general` — script listener hooks, a `Timer` utility, `Helper::Random()`.

### Backend abstraction pattern

Every raylib-touching subsystem follows the same shape: an interface at the module root (`IInputHandler`, `ISound`, `IEngine`) + a factory owning a single `#if DEFAULT_ENGINE == 1 ... #else #error` switch, + a concrete `*/raylib/Raylib*` implementation. `DEFAULT_ENGINE` is injected by `src/CMakeLists.txt` via `-DDEFAULT_ENGINE=<n>` (only `1` = raylib exists today). Adding a second backend means: implement the interface, extend the one `#if` block in that module's factory `.cpp` — call sites elsewhere should never need raylib types directly.

`engines/EngineFactory`/`IEngine` is the newest and most complete instance of this pattern — `TextureCanvas` and `TileMapRenderer` route all texture load/unload/draw calls through it and no longer need `<raylib.h>` in their own headers. Two things are deliberately **not yet** routed through it (each would be its own larger task):
- `TileMapRenderer`'s window lifecycle (`InitWindow`, `WindowShouldClose`, `BeginDrawing`/`EndDrawing`, `SetTargetFPS`, `ClearBackground`) still calls raylib directly.
- `sound/soundmanager.cpp` has its own, older copy of the `#if DEFAULT_ENGINE` switch (predates `EngineFactory`, not yet consolidated into it).

**Gotcha:** `SunLight::Input::KeyboardKey` is a distinct enum from raylib's own global `KeyboardKey`, even though the numeric values match. Because both are named `KeyboardKey` and there is no `using namespace SunLight::Input`, an unqualified `KeyboardKey` inside `namespace SunLight::Renderer` silently resolves to whichever one happens to be visible via prior `#include`s — not necessarily the intended one. Always fully qualify as `SunLight::Input::KeyboardKey` in new code (see `TileMapRenderer::SetExitKey`, which was fixed for exactly this bug).

### Native geometry/texture types

`base/primitives.h` defines `stRectangle`/`stVector2D` (float x/y/width/height — sub-pixel precision for zoom/scale math) and `TextureHandle` (`typedef void*`: an opaque handle owned by whichever `IEngine` loaded it, cast back to the concrete texture type only inside that engine's own `.cpp`). Prefer these over raylib's `Rectangle`/`Vector2`/`Texture2D` in any new backend-agnostic code.

### Pointer ownership convention

Internal containers/members that exclusively own heap-allocated data (e.g. `Sprite::m_Sequences`, `CollisionManager`'s rule lists, `SoundManager::m_SoundMap`, `TileMapRenderer::m_pInputHandler`/`m_AnimInfoList`/event handler lists) use `std::unique_ptr`, not raw `new`/`delete` pairs. Raw pointers remain only for genuinely non-owning references — back-pointers (`BaseCanvas::m_pParent`), externally-owned objects registered into a container (`Collider*` inside `CollisionManager`, `Sprite*`/`TextureCanvas*` handed to a manager by its caller), self-referential struct members (`GraphicObject::m_pDimension` can point to its own `m_Dimension`), and handles crossing a C API boundary (tmx callbacks, `TextureHandle`). When adding a new owning member, prefer `unique_ptr` over raw `new`/`delete` to match this convention — several real bugs (leaks, dangling pointers from `Clear()`-style methods that deleted contents without clearing the container) were found and fixed by this exact conversion.

## Known issues (tracked in `doc/`)
- `doc/FIXME.txt`: access violation when map scroll goes past viewport boundaries.
- `doc/TODO.txt`: no isometric/hexagonal map support; missing some gamepad face-button bindings; per-layer parallax scrolling and per-object property management (opacity/visible/position) not implemented; sprite/layer draw-order still being finished.

## Git workflow

`main` is protected on GitHub: direct pushes are rejected ("Changes must be made through a pull request") and merging requires at least one approving review. Push a feature branch, open a PR with `gh pr create`, and either wait for a human review or merge with `gh pr merge --squash --delete-branch --admin` (GitHub blocks self-approval of your own PR via `gh pr review --approve`, so the admin override is the only way a solo maintainer can merge without a second reviewer). The `gh` CLI is installed and authenticated on this machine.
