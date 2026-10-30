# tint sample

Shows layer tint at runtime. A Tiled layer's `tintcolor` multiplies into what it draws. The sample reads each layer with `GetLayer`, changes `stLayer::tint` (or the group's `nOpacity`) and writes it back with `SetLayer`; the change shows from the next frame. A group layer passes its own tint and opacity to everything inside it, as Tiled draws a group.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `tint_test` executable under `build/samples/tint/`, along with the shared libraries copied next to it.

## Running

`tint_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/tint/tint_test samples/tint/
```

## Controls

Each key acts once per press.

| Keys | Action |
| --- | --- |
| `1` | Next tint of the tile layer |
| `2` | Next tint of the image layer |
| `3` | Next tint of the object layer |
| `4` | Next tint of the group |
| `5` | Next opacity of the group |
| `0` | Every layer back to its authored tint and opacity |
| `Esc` | Quit |

A tint key cycles through the layer's authored tint, then white (no tint), red, green, blue and gold. The opacity key cycles through the group's authored opacity, then 255, 191, 64 and 0 (of 255).

## What to look for

The art is neutral grey, so a layer shows its tint as its own colour. The HUD at the bottom shows each layer's current tint and the group's opacity.

- **1. Tile layer**: grey stone and checker tiles, authored with an orange tint (`#ffc080`).
- **2. Image layer**: a grey banner, authored with a mint tint (`#80ffc0`).
- **3. Object layer**: tile objects (the house and grey tiles), authored with a lilac tint (`#c0a0ff`). The house's red roof turns darker under the tint, never brighter: a tint multiplies.
- **4. Group**: a group authored with a blue tint (`#80c0ff`) and opacity 0.5, over grey diagonal stripes that are not in it.
  - Its tile layer (the checker) takes the group's tint and opacity: the stripes show through it.
  - Its yellow rectangle, ellipse and text take only the opacity: they stay yellow whatever the group's tint, and fade with the group.
  - At opacity 0 the whole group disappears; at 255 the stripes are hidden.
- The white labels are on a layer outside every tint: they never change.

## In the code

- The authored tints and the group's opacity are in `resources/map/tint.tmx`.
- [tint_test.cpp](/samples/tint/tint_test.cpp) reads each layer's authored values once with `GetLayer`, and every key press goes through `GetLayer`, a change and `SetLayer`. It sets the input dispatch to `INPUT_DISPATCH_MODE_REACTIVE`, so a held key acts once.
- One font, loaded with `LoadFont`, is both the HUD's (`SetFont`) and the labels' (`RegisterFont`, the family `Sans`).

## Assets

- `resources/map/images/tiles.png` (two 32 x 32 grey tiles, a stone and a checker) and `images/banner.png` (a grey 320 x 128 panel): made for this sample, neutral grey so a tint shows as its own colour.
- `resources/map/tileset_house.tsx` and `images/house.png`: the house tileset from the [orthogonal sample](/samples/orthogonal/docs/README.md).
- `../shared/fonts/AnonymousPro-Bold.ttf`: Anonymous Pro, by Mark Simonson, under the SIL Open Font License 1.1 (`../shared/fonts/OFL-anonymouspro.txt`).
