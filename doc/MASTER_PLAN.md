# sunlight master plan

This is the working plan for the library after v0.38.0. Each item has a status, a
scope, and the test items that must pass before it is closed. Items are closed one at
a time, in phase order. Status values: `TODO`, `IN PROGRESS`, `DONE`.

Every change follows the project rules in `CLAUDE.md`: tests first (a regression test
must fail on the old code), the full suite passing, the suite run under
AddressSanitizer for anything touching lifetimes, the byte-identical trace harnesses
for anything touching drawing, and a PR per change (or per consolidated set of
changes).

## Phase A: low risk, no API change

| # | Item | Status |
|---|------|--------|
| A1 | AddressSanitizer job in CI | TODO |
| A2 | Reject non-orthogonal maps with a clear error | TODO |
| A3 | Compile-time check for the key mapping | TODO |
| A4 | Document `Concurrent::Timer` hazards in its header | TODO |
| A5 | Correct `doc/TODO.txt` against the code | TODO |
| A6 | Reproduce the scroll-past-boundary FIXME, then close or fix it | TODO |

**A1: AddressSanitizer in CI.** Add one Linux job (Clang, `-fsanitize=address`) running
the full test suite. Linux is chosen because it is the cheapest runner and catches the
same memory errors.
- Test items: the job runs the whole suite and fails on any ASan report; a deliberate
  use-after-free in a throwaway test fails the job.

**A2: reject non-orthogonal maps.** Maps with `orientation` other than `orthogonal`
(isometric, staggered, hexagonal) are rejected by `LoadMap` with a clear message,
instead of loading and drawing in the wrong places.
- Test items: an isometric, a staggered and a hexagonal test map each fail `LoadMap`
  with the message; an orthogonal map loads as before; the byte-identical harnesses are
  unchanged.

**A3: key mapping check.** `RayLibInputHandler::GetKeyPressed` casts sunlight's key
enum to raylib's. Add `static_assert` checks for each key value, so the build fails if
raylib renumbers a key. No runtime cost.
- Test items: the build passes today; a deliberate mismatch fails to compile.

**A4: Timer hazard documentation.** `Concurrent::Timer` runs its callback on a
background thread in real time and skips ticks when the main thread holds the lock.
The header will state this, and say that the callback must not touch engine or Lua
state except through the mutex. Keep the class: it is useful for independent time-based
tasks.
- Test items: documentation only; the header is reviewed against the code.

**A5: correct `doc/TODO.txt`.** Remove or correct items that are implemented (per-object
visibility and position, layer visibility, opacity and offset) and mark what is still
open (per-object opacity, sprite draw order within a layer, parallax and per-layer
camera, isometric and hexagonal maps).
- Test items: each line is checked against the code.

**A6: scroll-past-boundary FIXME.** `SetCameraPosition` deliberately does not clamp.
Reproduce the reported access violation with a test that moves the camera outside the
map. Then either fix it (clamp, or guard the draw path) or document that callers must
keep the camera in bounds. Caravellius is not affected, because the game clamps the
camera itself.
- Test items: a failing reproduction test exists before the fix; after the fix it
  passes; if documented instead, a test pins the documented behaviour.

## Phase B: tile, text and remaining objects

| # | Item | Status |
|---|------|--------|
| B1 | Draw Tiled text objects (`OT_TEXT`) | TODO |
| B2 | Draw Tiled tile objects (`OT_TILE`) | TODO |
| B3 | Warn on unknown object and layer types | TODO |

**B1: text objects.** Use the existing `IEngine::DrawText` and font loading. Respect the
object's font size, colour and position.
- Test items: a text object is drawn at its position through the mock engine's text
  event; the viewport clip applies; a map without text objects is unchanged.

**B2: tile objects.** An object that references a tile by global id is drawn as that
tile's image at the object's position.
- Test items: a tile object draws the expected source rectangle and destination; a
  tile object crossing the viewport edge is clipped.

**B3: unknown types.** Object types that are not handled, and unknown layer types, log
a warning once. Point objects are handled in Phase C5, not here.
- Test items: an unknown type logs once; known types log nothing.

## Phase C: thick primitives

| # | Item | Status |
|---|------|--------|
| C1 | Thick lines, with square caps | TODO |
| C2 | Thick polylines and polygons | TODO |
| C3 | Thick rectangles | TODO |
| C4 | Thick ellipses | TODO |
| C5 | Points as squares | TODO |

**Decisions already taken:**
- Stroke width scales with zoom: screen width is `thickness × zoomFactor`, rounded
  half-up, minimum 1.
- Width comes from code (a `thickness` argument, default 1) and from the map (a custom
  `line_width` property on the object or its object group, default 1).
- Angled lines use perpendicular width.
- Caps are square. Each end extends by `floor((width − 1) / 2)`, so width 1 adds
  nothing and output stays unchanged.
- The viewport boundary rule is unchanged: a pixel is drawn only if it is strictly
  inside the viewport, so the top and left edge are excluded, as they are today.
- The engine never clips. The renderer clips each span before calling
  `IEngine::DrawFilledRectangle`, so no new engine method is needed. A native primitive
  is only considered later, if the measured span count is too high.

**Implementation approach.** Each stroke is drawn as one filled span per scanline,
clipped to the viewport before the engine is called. A per-pixel `SetPixel` loop is not
used, because it costs one engine call per pixel, and a single engine call would bypass
the viewport boundary.

**C1: thick lines.** Perpendicular span code and the square cap rule.
- Test items: width 1 is byte-identical to today's `LineBresenham`, including lines on
  the top and left viewport edges; widths 2 to 5 at several angles match a brute-force
  reference rasterizer; a line crossing each viewport edge and a corner is cut at the
  reference's pixels; caps extend by the stated amount and nothing at width 1; at zoom 2 a
  width-3 line is 6 pixels wide, with the rounding checked at a fractional zoom; the number
  of `DrawFilledRectangle` calls equals the number of spans and is far below the pixel
  count.

**C2: thick polylines and polygons.** Joins have no gaps.
- Test items: a sharp corner has no gap, checked against the reference; a polygon's
  closing segment is drawn only with more than two points, as today; width 1 output is
  byte-identical.

**C3: thick rectangles.** Four thick edges.
- Test items: matches the reference, including shared corners; width 1 output is
  byte-identical.

**C4: thick ellipses.** A ring between an outer and an inner radius.
- Test items: matches a reference ring; width 1 output is byte-identical to the midpoint
  output; an ellipse crossing the boundary is clipped at the reference's pixels.

**C5: points.** A point of size `s` at zoom `z` is a filled square of side
`round(s × z)`, minimum 1. This also closes the point-object gap from B.
- Test items: the side length follows the rule; a point on the viewport edge follows the
  strict rule; a Tiled point object draws as a point.

**Across all of Phase C**
- Byte-identical harnesses at default width (the animation and camera traces).
- Full suite under AddressSanitizer.
- Mutation checks: each rule has a deliberate mutant (wrong cap length, wrong edge rule,
  wrong zoom rounding) that a test must catch.

## Phase D: isometric, staggered and hexagonal maps

| # | Item | Status |
|---|------|--------|
| D1 | Isometric rendering (tile to pixel, pixel to tile) | TODO |
| D2 | Staggered rendering | TODO |
| D3 | Hexagonal rendering | TODO |

This is the largest feature. It needs its own design before implementation: coordinate
conversion in both directions, `TileMapToTileMatrix` for each orientation, culling of
off-screen tiles, and the draw order of tiles and sprites. Each orientation needs a
small test map, and a real-window check by the owner.

- Test items per orientation: tile-to-pixel and pixel-to-tile round-trip for every tile
  of a small map; drawn positions match a reference; the viewport clip matches the
  orthogonal rule; the existing orthogonal harnesses are unchanged.

## Later

| # | Item | Status |
|---|------|--------|
| L1 | Per-layer camera and parallax | TODO |
| L2 | Per-object opacity | TODO |
| L3 | Sprite draw order within a layer | TODO |

These need their own design before implementation.

## Docs, samples and tests

Done last, once the features they describe are settled: tidy the CHANGELOG (its
Unreleased sections have grown long), update the README and sample docs, and extend the
samples and tests where a feature needs a demonstration.

## Process

- Items are closed one at a time, in phase order, unless the owner decides otherwise.
- Each item is closed only when its test items pass and the owner has confirmed it.
- Item 7 of the original review (manual review of documentation and code) stays manual
  until an open-source tool exists to help. It should be revisited at that point.
