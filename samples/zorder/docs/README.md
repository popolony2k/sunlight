# zorder sample

Shows sprite draw order within a layer: four overlapping "cards", all registered on the same layer, and a key per card that brings it to the front of the stack with `Sprite::SetZOrder`.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `zorder_test` executable under `build/samples/zorder/`, along with the shared libraries copied next to it.

## Running

`zorder_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/zorder/zorder_test samples/zorder/
```

## Controls

| Keys | Action |
| --- | --- |
| `1` | Bring the red card to the front |
| `2` | Bring the green card to the front |
| `3` | Bring the blue card to the front |
| `4` | Bring the gold card to the front |
| `Esc` | Quit |

## What to look for

- The cards start stacked back to front as red, green, blue and gold, each 120 x 120 pixels and offset from the one before, so every card is partly covered.
- Pressing a card's key puts it on top at once, and it stays there until another card is brought forward. Pressing the same key again changes nothing visible: the card is already on top.
- All four cards are on the same layer, so this is draw order inside one layer. Layers are still drawn in map order, and a sprite is drawn with its own layer.

## In the code

- `World::LoadSprites()` ([world.cpp](/samples/zorder/world.cpp)) gives the cards z-orders 0 to 3 and registers them on layer 1 with `AddSprite`.
- `World::BringToFront()` gives the chosen card a z-order higher than any card has had so far. The renderer sorts each layer's sprites by z-order every frame, so the change shows on the next frame. Sprites with the same z-order keep the order they were added in.

## Assets

- `resources/map/zorder.tmx`: the labels (title and instructions).
- `resources/sprites/card_red.png`, `card_green.png`, `card_blue.png` and `card_gold.png`: the cards.
- `../shared/fonts/caravellius8x8.fnt` and `.png`: Caravellius 8x8, designed by PopolonY2k and Leidson Campos A. Ferreira. Dual-licensed under the zlib License or the SIL Open Font License 1.1 (`../shared/fonts/OFL-caravellius8x8.txt`).
