# multiview sample

The same map shown in three places at once, using `sunlight`'s views (`SunLight::TileMap::IView`, `TileMapRenderer::CreateView`):

- the **main view** — the renderer's own default view, scrolled and zoomed with the keyboard;
- a **minimap** (top right) — the whole map (`FitToMap()`), with the sprite's layer masked out (`ShowLayer(id, false)`) and a translucent background;
- a **close-up** (right, below it) — a second camera on the map at a higher zoom, sprite included.

It also shows what a view does *not* do: a sprite belongs to its layer, so a view shows the sprite exactly when it shows that layer, and its position is relative to the viewport that draws it.

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
| Arrow keys | Scroll the main view (sprite-relative inversion, like the other samples) |
| `Page Up` / `Page Down` | Zoom the main view out / in |
| `W` `A` `S` `D` | Scroll the close-up view |
| `1` | Show / hide the minimap |
| `2` | Show / hide the close-up |
| `3` | Mask the `sky` layer out of / back into the main view |
| `4` | Send the minimap behind / bring it in front of the main view (draw order) |
