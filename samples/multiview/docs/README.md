# multiview sample

The same map - and the same character, Sunny - shown in three places at once, using `sunlight`'s views (`SunLight::TileMap::IView`, `TileMapRenderer::CreateView`):

- the **main view** — the renderer's own default view, showing the whole map (`FitToMap()`);
- a **minimap** (top right) — the whole map, small, with a translucent background;
- a **close-up** (right, below it) — a zoomed view whose camera **follows Sunny**: it stands still while Sunny walks inside it and scrolls when Sunny comes near its border.

Sunny has one position, in map coordinates, and you walk it with the keyboard: it moves in all three views at once.

## How it works, and the limit it works around

A sprite's position is relative to the view that draws it: it is drawn at *position × zoom* from the view's origin and **ignores the view's camera** (the map's tiles do not). So a single sprite cannot sit on the same map spot in views with different zoom and cameras, nor follow a scrolling camera. This sample keeps **one Sunny sprite per view**, each registered on its own map layer, and each view's layer mask (`ShowLayer(id, false)`) lets through only its own Sunny. After every move each sprite is placed at *Sunny's map position − that view's camera*, which is exactly where the map is drawn at that position (this rule is covered by a unit test). The trade-off: masking a layer out of a view hides its tiles there too, so the three layers (`sunny_main`, `sunny_minimap`, `sunny_closeup`) are empty ones added to this sample's copy of the map for the purpose - masking a layer that held map content would make that content disappear from the other views.

Sprites that live in map coordinates and are drawn correctly by every view (one sprite, all views, camera-following for free) would remove the workaround; that is a possible engine feature, not something this sample can do today.

## Building

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

## Running

Like the other samples, `multiview_test` resolves its resources (a copy of the [sprite](/samples/sprite/docs/README.md) sample's map and sprite) relative to a base path passed as `argv[1]` — point it at this sample's own directory:

```shell
./build/samples/multiview/multiview_test samples/multiview/
```

## Controls

| Keys | Action |
| --- | --- |
| Arrow keys / `W` `A` `S` `D` | Walk Sunny |
| `Page Up` / `Page Down` | Zoom the close-up out / in (its camera re-follows Sunny) |
| `1` | Show / hide the minimap |
| `2` | Show / hide the close-up |
| `3` | Mask the `sky` layer out of / back into the main view |
| `4` | Send the minimap behind / bring it in front of the main view (draw order) |
