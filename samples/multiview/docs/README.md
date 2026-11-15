# multiview sample

The same map - and the same character, Sunny - shown in three places at once, using `sunlight`'s views (`SunLight::TileMap::IView`, `TileMapRenderer::CreateView`):

- the **main view** — the renderer's own default view, showing the whole map (`FitToMap()`);
- a **minimap** (top right) — the whole map, small, with a translucent background;
- a **close-up** (right, below it) — a zoomed view whose camera **follows Sunny**: it stands still while Sunny walks inside it and scrolls when Sunny comes near its border;
- an **overlay** exactly over the main view — the same map without its sky, on a dark background — at opacity 0 until you cross-fade to it (`IView::SetOpacity`).

Sunny has one position, in map coordinates, and you walk it with the keyboard: it moves in all three views at once.

## How it works

Sunny is **one sprite in world space** (`Sprite::SetWorldSpace(true)`). By default a sprite's position is relative to the view that draws it and ignores the view's camera; a world-space sprite's position is a *map* position, and every view draws it where it draws the map at that position - each view adds its own camera and zoom, exactly as it does for a map tile. So the sample does not place or duplicate the sprite per view: it only moves Sunny, and moves the close-up's camera so that view follows him (the camera is the map point shown at the view's top-left; `SetCameraPosition` does not clamp, so the sample keeps it inside the map).

A view's opacity fades the view's **finished image**, not each thing in it: mid cross-fade the overlay's map still hides the main view's map under it, and at opacity 0 a view shows nothing while its sprites keep animating.

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
| `+` / `-` / `=` (keyboard or keypad) | Zoom the close-up in / out / reset its zoom |
| `1` | Show / hide the minimap |
| `2` | Show / hide the close-up |
| `3` | Mask the `sky` layer out of / back into the main view |
| `4` | Send the overlay behind / bring it in front of the main view (draw order; visible once it is faded in with `6`) |
| `5` | Close-up opacity: 1 → 0.5 → 0 → 1 |
| `6` | Cross-fade between the main view and the overlay (one second), one way then the other |

The keys are listed on screen too, under the close-up. Walking and zooming repeat while a key is held; the toggles (`1` to `6`) act once per press.
