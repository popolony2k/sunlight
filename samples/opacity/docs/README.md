# opacity sample

Shows per-object opacity: a Tiled object's own `opacity` custom property (a float from 0 to 1) overrides its layer's opacity for that object. Each row of the map is one kind of object, fading from fully opaque on the left to fully transparent on the right, with a label above it.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `opacity_test` executable under `build/samples/opacity/`, along with the shared libraries copied next to it.

## Running

`opacity_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/opacity/opacity_test samples/opacity/
```

## Controls

| Keys | Action |
| --- | --- |
| Arrow keys | Scroll the map |
| `Page Up` / `Page Down` | Zoom in / out |
| `Home` | Reset the zoom |
| `End` | Move the camera back to the map's origin |
| `Esc` | Quit |

These are `DefaultControllerBindings`, installed by the sample with `DefaultControllerBindings::Install`.

## What to look for

The green rectangle is the viewport's frame in the map.

- **1. Tile objects**: five house tiles with `opacity` 1.0, 0.75, 0.5, 0.25 and 0.0. The last one is invisible.
- **2. Shape objects**: five rectangle outlines with the same five values.
- **3. Text objects**: five labels with the same five values.
- **4. No property vs. invalid**: two tiles on a layer whose own opacity is 0.4. One has no `opacity` property; the other has `5`, outside 0..1, so it is ignored (with a warning on the console, once). Both fall back to the layer's 0.4 and look the same.

Shape and text objects are drawn with their layer's opacity too, when they have no property of their own.

## In the code

- The opacities are all in `resources/map/opacity.tmx`: nothing in [opacity_test.cpp](/samples/opacity/opacity_test.cpp) sets them.
- The sample mounts its own folder and `../shared` (for the label font) with `IFileSystem::Mount`, registers the font with `RegisterFont`, and starts the camera at (100, 100) so the frame fills the viewport.

## Assets

- `resources/map/opacity.tmx`, `tileset_house.tsx` and `images/house.png` (the house tileset from the [orthogonal sample](/samples/orthogonal/docs/README.md)).
- `../shared/fonts/caravellius8x8.fnt` and `.png`: Caravellius 8x8, designed by PopolonY2k and Leidson Campos A. Ferreira. Dual-licensed under the zlib License or the SIL Open Font License 1.1 (`../shared/fonts/OFL-caravellius8x8.txt`).
