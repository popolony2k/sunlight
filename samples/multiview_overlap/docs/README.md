# multiview overlap sample

The [multiview sample](../../multiview/docs/README.md) with its views **overlapping the main view**, and with Sunny able to walk out past every border. It tests the clipping of sprites and tiles at each view's own edges, and that one view never draws over another.

- the **main view** (left, large) — the whole map, as in the multiview sample;
- the **minimap** (top right of the main view) — overlaps the main view's top-right corner;
- the **close-up** (bottom left of the main view) — overlaps the main view's bottom-left corner, and its camera follows Sunny.

Sunny can walk up to 1000 map pixels beyond the map on every side. That takes her past the edge of the main view, the minimap and the close-up. The close-up's camera stays inside the map, so Sunny leaves the close-up's rectangle once she is outside the map.

## Building

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

## Running

```shell
./build/samples/multiview_overlap/multiview_overlap_test samples/multiview_overlap/
```

## Controls

Arrow keys or `W` `A` `S` `D` walk Sunny. `1` hides or shows the minimap, `2` the close-up, `3` the map's second layer, `4` changes the minimap's draw order. `Page Up` and `Page Down` zoom the close-up. `Esc` quits.

## What to look for

- Walk Sunny across the edge of the **main view**: she should be cut exactly at the window's view edge, not drawn beyond it.
- Walk her across the **minimap's** and the **close-up's** edges. Each is cut at its own border, including where it overlaps the main view. Nothing from one view should appear inside another.
- Walk her outside the map: the close-up keeps its camera on the map's edge, so Sunny leaves its rectangle.
