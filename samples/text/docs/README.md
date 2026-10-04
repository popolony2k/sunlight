# text sample

Shows Tiled text objects (`OT_TEXT`) and the clipping that cuts them at the viewport's edges. A map with only text objects and one frame; the camera starts so that each of the four edges of the viewport crosses a label.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `text_test` executable under `build/samples/text/`, along with the shared libraries copied next to it.

## Running

`text_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/text/text_test samples/text/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | Scroll the view up |
| `S` / `Down Arrow` | Scroll the view down |
| `A` / `Left Arrow` | Scroll the view left |
| `D` / `Right Arrow` | Scroll the view right |
| `Esc` | Quit |

## What to look for

The green rectangle is the viewport's frame in the map. The viewport is the area inside it; anything drawn outside it must not be visible.

- **`<- cut at the LEFT edge`**, starts left of the frame: its first letters are cut at the left edge.
- **`cut at the TOP edge`**, crosses the top edge: its upper part is cut off.
- **`cut at the RIGHT edge ->`**, runs past the right edge: its end is cut.
- **`cut at the BOTTOM edge`**, crosses the bottom edge: its lower part is cut.
- **`corner`**, is at the top-left, so it is cut on two sides when the view is scrolled slightly.
- **`Wrapped, centred text...`**, is wrapped at the box's width and centred on both axes.
- **`right / bottom`**, is aligned to the box's right and bottom edges.
- **`Bold face`**, uses the bold face of `Sans` (Caravellius 8x8 bold).
- **`Press Start 2P`**, uses the `Pixel` family (Press Start 2P), a second registered family.
- **`italic is not registered`** and **`Mono is not registered`**, have no registered font, so they show `??????` in their colour. A warning is written to the terminal once for each.
- **`first line` / `second line`**, is two lines, from one line break in the text.

A text that is entirely outside the viewport is not drawn at all.

## Fonts

- `resources/fonts/caravellius8x8.fnt` and `caravellius8x8_bold.fnt` (with their `.png` atlases): Caravellius 8x8, designed by PopolonY2k and Leidson Campos A. Ferreira. Dual-licensed under the zlib License or the SIL Open Font License 1.1 (`resources/fonts/OFL-caravellius8x8.txt`).
- `resources/fonts/pressstart2p-regular.ttf`: Press Start 2P, by The Press Start 2P Project Authors. SIL Open Font License 1.1 (`resources/fonts/OFL-pressstart2p.txt`).
