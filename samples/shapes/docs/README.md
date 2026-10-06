# shapes sample

The four shapes Tiled can draw as objects - rectangle, ellipse, polyline and polygon - each drawn by the backend's own routine, in a map larger than the viewport.

## Building

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build --target shapes_test
```

## Running

```shell
./build/samples/shapes/shapes_test samples/shapes/
```

## Controls

Arrow keys or `W` `A` `S` `D` scroll the view across the map. `Esc` quits.

## What to look for

- **1 rectangle** and **2 ellipse** start inside the view, at the top. Scroll them out through each edge: they must stop at the view's edge.
- **3 polyline** and **4 polygon** start in the middle of the view. Scroll them out to the left, right, top and bottom.
- **5 far rectangle** and the far ellipse are outside the view at the start. Scroll right and down to bring them in, then past the edge again.

A shape must never be drawn outside the view it belongs to, and each edge must stop exactly at the viewport's border.
