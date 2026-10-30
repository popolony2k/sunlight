# sunlight

<div align="center">
<img src="/resources/sunny.png" alt="sunlight logo" title="sunny">

[![CI](https://github.com/popolony2k/sunlight/actions/workflows/ci.yml/badge.svg)](https://github.com/popolony2k/sunlight/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/popolony2k/sunlight)](https://github.com/popolony2k/sunlight/releases/latest)
[![License: zlib](https://img.shields.io/badge/license-zlib-blue.svg)](https://github.com/popolony2k/sunlight/blob/main/LICENSE)

`sunlight` is an open source C++17 library for making 2D games from [Tiled](https://www.mapeditor.org/) maps: it loads and draws the map, and manages the sprites, collisions, input, sound and scripted sequences of a game built on it.

[raylib](https://www.raylib.com/) was used for graphic backend, but the project aims to be extended to SDL and many others. Rendering is exposed through an `IEngine` interface (`src/engines/`) so a new backend only needs its own implementation plus one line in the engine factory, without touching the rest of the codebase.
</div>

## Table of Contents :pushpin:
* [API documentation](https://popolony2k.github.io/sunlight/) :book:
* [Changelog](https://github.com/popolony2k/sunlight/blob/main/CHANGELOG.md) :scroll:
* [License](https://github.com/popolony2k/sunlight/blob/main/LICENSE) (zlib) :page_facing_up:
* [Features](#features-sparkles)
* [Requirements](#requirements-memo)
    - [CMake 3.24](#cmake-324)
    - [vcpkg](#vcpkg-windows-only)
    - [LibXML2](#libxml2)
    - [zlib](#zlib)
    - [PhysFS](#physfs)
    - [SDL2](#sdl2)
    - [raylib on linux](#raylib-dependencies-on-linux)
* [HowTo](#howto-bulb)
    - [Using samples](#using-samples)
    - [Running tests](#running-tests)
    - [Configuring VsCode](#configuring-vscode)
* [Samples](#samples-sunny)

## Features :sparkles:

* **Tiled maps:** orthogonal, isometric, staggered and hexagonal maps; tile layers (with tile animation), image layers, group layers and object layers; external tilesets and object templates; per-layer opacity, tint and parallax.
* **Objects:** rectangles, ellipses, polylines and polygons with a line width, and points with a size; text objects with registered fonts; tile objects; Tiled's rotation (for everything but points) and a per-object opacity.
* **Sprites:** animated sprites with several animation modes and sequences, drawn on a map layer or with no map at all, in screen or map (world) space, ordered within a layer.
* **Views:** several views of one map at once (e.g. a minimap or a close-up), each with its own camera, zoom and layer mask.
* **Collisions:** collider-to-collider and collider-to-tile rules, with a listener for the hits.
* **Input:** keyboard and gamepad handlers, connected-gamepad list and hot-plug, player slots, rebindable named actions saved to a file, dead zones per pad and axis, and gamepad rumble.
* **Drawing without a map:** shapes, sprites, text and filled rectangles on a plain drawing surface, for menus and HUDs.
* **Sound and scripting:** a sound manager, and a script processor for timed sequences such as cutscenes.
* **Resources:** a virtual filesystem over loose folders or archives (PhysFS), with an optional read filter (e.g. decryption).
* **Headless runs:** a null backend that needs no display, with virtual time, for tests and automation.

## Requirements :memo:

In order to make `sunlight` run as desired, it is necessary to install the dependencies listed below beforehand:

### CMake 3.24 

You may install the package on its official [page](https://cmake.org/). Higher versions are also supported.

- For **Linux**

    Ubuntu/Debian users can install the package by using the command `apt-get` on their Linux shell.

    It is also recommended to install by using the standard package manager offered by your prefered distro.

- For **MacOS**

    Mac users may obtain the package through [MacPorts](https://www.macports.org/) or [Homebrew](https://brew.sh/).

### vcpkg (Windows only)

Use [vcpkg](https://github.com/microsoft/vcpkg) to integrate C/C++ libraries using Visual Studio.

### LibXML2

By default, the project's CMakeLists will automatically search LibXML and its dependencies (LibIconv). For those who might want to use the library installed on your system, the following parameter should be used to call CMake:
```shell
cmake -DUSE_LOCAL_LIBXML2
```

### zlib

zlib can be installed by using your standard package manager.

The installation steps for which OS are listed below.

- **Ubuntu/Debian family**

```shell
apt-get install zlib1g-dev
```

- **Windows** (vcpkg)

```bat
.\vcpkg\vcpkg install zlib:x64-windows
```

- **MacOS**

    Install zlib through [MacPorts](https://www.macports.org/) or [Homebrew](https://brew.sh/).

### PhysFS

[PhysFS](https://icculus.org/physfs/) backs `sunlight`'s virtual filesystem (mounting a loose directory or an archive so every resource read — textures, sound, tilemaps — goes through one path space). Like LibXML2/tmx/raylib, it is fetched and built automatically via `FetchContent`; no separate install is required.

### SDL2

[SDL2](https://www.libsdl.org/) is used for **gamepad rumble only**. raylib reads the gamepads through GLFW, and GLFW has no vibration API at all: raylib's own `SetGamepadVibration()` is an empty stub on desktop platforms, which only logs that it is not available. SDL2's game controller API can vibrate a pad, so `sunlight`'s rumble backend (`IGamepadHaptics`, `src/backends/sdl/`) uses it. Everything else - the window, drawing, sound and reading the buttons and sticks - stays with raylib.

Like PhysFS, SDL2 is fetched and built automatically via `FetchContent` (pinned to `release-2.32.10`), as a static library with its video, audio and render parts turned off, so it adds no separate library to ship and no separate install is required.

### raylib dependencies on Linux

You may find the detailed instructions by clicking [here](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) and following the steps regarding the operational system you're using.

## HowTo :bulb:

### Using samples

By default, `sunlight` doesn't build the projects' samples. 

In order to make it compile with samples, it is necessary to open the CMakeLists on the project's root to change the option from **OFF** to **ON**, as shown below:
```cmake
option(BUILD_LIBRARY_SAMPLES "Build SunLight library samples" ON)
```

>[!NOTE]
>You may also use the following command line on your system's shell:
>```shell
>cmake -DBUILD_LIBRARY_SAMPLES=ON
>```

### Running tests

Unit tests are also opt-in, using [doctest](https://github.com/doctest/doctest) fetched automatically via CMake:
```shell
cmake -DBUILD_LIBRARY_TESTS=ON
cmake --build build --target sunlight_tests
./build/tests/sunlight_tests
```

>[!NOTE]
>You may also run the tests through CTest:
>```shell
>ctest --test-dir build
>```

### Configuring VsCode

If you intend to use VsCode as your IDE, follow the instructions shown [here](https://github.com/popolony2k/sunlight/blob/main/doc/vscode/README.md).

## Samples :sunny:

Each sample takes its own directory as `argv[1]` and resolves its resources relative to it, e.g.:
```shell
./build/samples/sprite/sprite_test samples/sprite/
```

### Getting started

* [orthogonal](https://github.com/popolony2k/sunlight/blob/main/samples/orthogonal/docs/README.md) — the minimal setup: loads an orthogonal Tiled map and drives the renderer's camera and zoom, no sprites.
* [sprite](https://github.com/popolony2k/sunlight/blob/main/samples/sprite/docs/README.md) — builds on `orthogonal` by adding an animated `Sprite` on top of the map.
* [collision](https://github.com/popolony2k/sunlight/blob/main/samples/collision/docs/README.md) — builds on `sprite` by making the sprite player-controlled and colliding it with a static obstacle via `CollisionManager`.
* [gamepad](https://github.com/popolony2k/sunlight/blob/main/samples/gamepad/docs/README.md) — builds on `collision` by giving two characters their own controls on one gamepad: the left stick/D-pad drives Sunny, the right stick/face buttons a second sprite.

### Map orientations

* [isometric](https://github.com/popolony2k/sunlight/blob/main/samples/isometric/docs/README.md) — an isometric map: Sunny walks from cell to cell, blocked by obstacle tiles, and the camera follows.
* [staggered](https://github.com/popolony2k/sunlight/blob/main/samples/staggered/docs/README.md) — the same on a staggered map, where every other row is shifted by half a tile.
* [hexagonal](https://github.com/popolony2k/sunlight/blob/main/samples/hexagonal/docs/README.md) — the same on a hexagonal map, moving in six directions.

### Objects and layers

* [text](https://github.com/popolony2k/sunlight/blob/main/samples/text/docs/README.md) — Tiled text objects: registered fonts, wrap and alignment, and the clipping that cuts text at the viewport's edges.
* [objects](https://github.com/popolony2k/sunlight/blob/main/samples/objects/docs/README.md) — tile objects: their own size, resized (stretched), cut by the viewport edges, and animated.
* [shapes](https://github.com/popolony2k/sunlight/blob/main/samples/shapes/docs/README.md) — rectangles, ellipses, polylines, polygons and points: clipping, line widths and rotation, plus a benchmark map.
* [opacity](https://github.com/popolony2k/sunlight/blob/main/samples/opacity/docs/README.md) — a per-object `opacity` property on tile, shape and text objects, and the fallback to the layer's opacity.
* [parallax](https://github.com/popolony2k/sunlight/blob/main/samples/parallax/docs/README.md) — layers scrolling at different speeds (Tiled's per-layer parallax) around a walking Sunny, on a map that wraps.
* [zorder](https://github.com/popolony2k/sunlight/blob/main/samples/zorder/docs/README.md) — four overlapping sprites on one layer, each brought to the front with `Sprite::SetZOrder`.
* [blend](https://github.com/popolony2k/sunlight/blob/main/samples/blend/docs/README.md) — blend modes (normal, add, multiply, subtract): a glow and a shadow sprite with their own mode (`Sprite::SetBlendMode`), and panels drawn by the game between `BeginBlendMode` and `EndBlendMode`, over a map or with no map.
* [tint](https://github.com/popolony2k/sunlight/blob/main/samples/tint/docs/README.md) — layer tint at runtime through `GetLayer`/`SetLayer` on a tile, an image and an object layer, and a group's tint and opacity reaching the layers inside it.

### Views

* [multiview](https://github.com/popolony2k/sunlight/blob/main/samples/multiview/docs/README.md) — the same map and the same character shown in three places at once (a main view, a minimap, and a zoomed close-up that follows the character), demonstrating `sunlight`'s multiple views (`TileMapRenderer::CreateView`) and world-space sprites (`Sprite::SetWorldSpace`).
* [multiview_overlap](https://github.com/popolony2k/sunlight/blob/main/samples/multiview_overlap/docs/README.md) — `multiview` with the views overlapping the main view, testing the clipping at each view's edges.

### Input and rumble

* [input](https://github.com/popolony2k/sunlight/blob/main/samples/input/docs/README.md) — rebindable named actions saved to a file (`ActionMap`), gamepads joining as players (`PlayerSlotManager`), connect/disconnect events, and a live stick dead zone.
* [isometric_haptics](https://github.com/popolony2k/sunlight/blob/main/samples/isometric_haptics/docs/README.md) — the `isometric` sample with a gamepad, which rumbles when Sunny walks into a wall (`IGamepadHaptics`).

### Scripting and sound

* [scriptprocessor](https://github.com/popolony2k/sunlight/blob/main/samples/scriptprocessor/docs/README.md) — a scripted "stage intro" cutscene demonstrating `ScriptProcessor`: queued commands load a stage, slide sprites on screen, and play/pause/resume a tone via `SoundManager`, exercising every control-flow command (`WAIT_CMD`, `WAIT_SPRITES_QUEUE_EMPTY`, `LOOP_CMD`/`END_LOOP_CMD`, `LABEL_CMD`/`GOTO_LABEL_CMD`).

### Without a map

* [surface](https://github.com/popolony2k/sunlight/blob/main/samples/surface/docs/README.md) — `TileMapRenderer` as a plain drawing surface: shapes, a sprite and a HUD, with no map loaded.
