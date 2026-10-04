# text sample

Shows Tiled text objects (`OT_TEXT`), registered fonts in several families and styles, the text in three languages, and the clipping that cuts text at the viewport's edges. A map with only text objects and one frame; the camera starts so that each of the four edges of the viewport crosses a label.

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

**Clipping at the edges**
- **`cut at the LEFT edge`**, starts just left of the frame: its first letter is cut in half by the left edge.
- **`cut at the TOP edge`**, crosses the top edge: its upper part is cut off.
- **`cut at the RIGHT edge ->`**, runs past the right edge: its end is cut.
- **`cut at the BOTTOM edge`**, crosses the bottom edge: its lower part is cut.
- **`corner`**, at the top-left, is cut on two sides when the view is scrolled slightly.

**Languages** (the same kind of sentence in three languages, in the top block)
- **`English: the quick brown fox jumps`**
- **`Português: coração, ação, é, õ, ã`**, with accents.
- **`Español: ¡Hola, mañana! ¿Qué tal?`**, with `ñ`, `¡` and `¿`.

**Layout**
- **`Wrapped, centred text...`**, is wrapped at the box's width and centred on both axes.
- **`right / bottom`**, is aligned to the box's right and bottom edges.
- **`first line` / `second line`**, is two lines, from one line break in the text.

**Families and styles**
- **`Bold face: Caravellius 8x8`**, uses the bold face of `Sans`.
- **`Mono italic: Anonymous Pro Italic`** and **`Mono bold italic`**, use the italic and bold-italic faces of `Mono`.
- **`Press Start 2P: ção, ñ`**, uses the `Pixel` family.
- **`Sans italic: not registered`** and **`Serif: not registered`**, have no registered font, so they show `??????` in their colour. A warning is written to the terminal once for each.

A text that is entirely outside the viewport is not drawn at all.

## Fonts

All the fonts are under the SIL Open Font License 1.1 or the zlib License, and each has its licence text in `resources/fonts/`.

- `caravellius8x8.fnt` and `caravellius8x8_bold.fnt` (with their `.png` atlases): Caravellius 8x8, regular and bold, designed by PopolonY2k and Leidson Campos A. Ferreira. Dual-licensed under the zlib License or the SIL Open Font License 1.1 (`OFL-caravellius8x8.txt`). It covers the accented letters used in the labels.
- `AnonymousPro-Regular.ttf`, `AnonymousPro-Bold.ttf`, `AnonymousPro-Italic.ttf` and `AnonymousPro-BoldItalic.ttf`: Anonymous Pro, Copyright (c) 2009, Mark Simonson. SIL Open Font License 1.1 (`OFL-anonymouspro.txt`).
- `pressstart2p-regular.ttf`: Press Start 2P, by The Press Start 2P Project Authors. SIL Open Font License 1.1 (`OFL-pressstart2p.txt`).
