# shapes sample

The four shapes Tiled can draw as objects - rectangle, ellipse, polyline and polygon - each drawn by the backend's own routine, in a map larger than the viewport.

## Building

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build --target shapes_test
```

## Running

```shell
./build/samples/shapes/shapes_test samples/shapes/ [map file] [frames] [fps] [zoom]
```

Only the sample directory is required. The optional arguments:

- `map file`: a map in `resources/map/`, `shapes.tmx` by default (see "The maps" below).
- `frames`: stop after that many frames and print the average time per frame, for benchmarks. `0` (the default) runs until `Esc`.
- `fps`: the target frame rate.
- `zoom`: the starting zoom factor, a multiple of 1/16 (`2` is twice the size).

For example, `shapes_test samples/shapes/ thick.tmx 0 60 2` opens the thick-lines map at zoom 2.

## Controls

| Keys | Action |
| --- | --- |
| Arrow keys | Scroll the map |
| `+` / `-` or `Page Up` / `Page Down` | Zoom in / out |
| `=` or `Home` | Reset the zoom |
| Keypad `+` / `-` / `=` | Zoom in / out / reset |
| `End` | Move the camera back to the map's origin |
| `Esc` | Quit |

The arrow keys, `Page Up`/`Page Down`, `Home` and `End` are `DefaultControllerBindings`, installed by the sample with `DefaultControllerBindings::Install`.

`+` and `=` are one key on the keyboard (`+` is Shift and `=`), and raylib reports the physical key, so the sample tells them apart by whether a Shift key is held.

## The maps

- `shapes.tmx` (the default): the four shapes and how the viewport clips them, below.
- `thick.tmx`: polylines, a polygon, a rectangle and an ellipse with a `line_width` property (whole and fractional widths), and points with a `point_size`. Zoom in and out to see the widths scale with the zoom.
- `rotation.tmx`: shapes with a Tiled rotation, each turned clockwise about its stored point (the small square marked "pivot").
- `stress.tmx`: 72 shapes of four kinds in a grid larger than the viewport, for benchmarks with the `frames` argument.

## What to look for (shapes.tmx)

- **1 rectangle** and **2 ellipse** start inside the view, at the top. Scroll them out through each edge: they must stop at the view's edge.
- **3 polyline** and **4 polygon** start in the middle of the view. Scroll them out to the left, right, top and bottom.
- **5 far rectangle** and the far ellipse are outside the view at the start. Scroll right and down to bring them in, then past the edge again.

A shape must never be drawn outside the view it belongs to, and each edge must stop exactly at the viewport's border.
