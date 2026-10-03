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

## Phase D: object rotation

| # | Item | Status |
|---|------|--------|
| D1 | Apply Tiled object rotation to shapes, text and tile objects | TODO |

**D1: object rotation.** libtmx parses each object's `rotation`, but `DrawObjects`
ignores it. Rotate each shape's points about its pivot before drawing, so rotated shapes
also get the thick-primitive work from Phase C. The pivot must follow Tiled's convention,
which differs between rectangle, tile and text objects, so it is verified against Tiled
before implementation.
- Test items: rotation 0 is byte-identical to today; a 90 degree rectangle matches a
  reference; a rotated text object is drawn at the rotated position; a rotated tile object
  uses Tiled's pivot; a rotated thick outline matches the reference from Phase C.

## Phase E: isometric, staggered and hexagonal maps

| # | Item | Status |
|---|------|--------|
| E1 | Isometric rendering (tile to pixel, pixel to tile) | TODO |
| E2 | Staggered rendering | TODO |
| E3 | Hexagonal rendering | TODO |

This is the largest feature. It needs its own design before implementation: coordinate
conversion in both directions, `TileMapToTileMatrix` for each orientation, culling of
off-screen tiles, and the draw order of tiles and sprites. Each orientation needs a
small test map, and a real-window check by the owner.

- Test items per orientation: tile-to-pixel and pixel-to-tile round-trip for every tile
  of a small map; drawn positions match a reference; the viewport clip matches the
  orthogonal rule; the existing orthogonal harnesses are unchanged.

## Phase F: input configuration

| # | Item | Status |
|---|------|--------|
| F1 | Device list and hot-plug events | TODO |
| F2 | Player slots: claim, release, reassign after unplug | TODO |
| F3 | Action map: named actions, rebinding, load and save | TODO |
| F4 | Tunable dead zones and thresholds per pad and axis | TODO |
| F5 | Duplicate pad registration is ignored | TODO |
| F6 | Default handlers are opt-in, and a game can take over input | TODO |
| F7a | Rumble interface, factory, null and mock implementations (no real device) | TODO |
| F7b | Real rumble backend, once a platform that vibrates is chosen | TODO |

Today the renderer only polls gamepads a caller has added with `AddGamePad`. It does
not detect devices, react to connect or disconnect, or let a game remap actions.
Dead zones are fixed at 0.1 and triggers at -0.9. Default handlers are installed unless
`bUseDefaultKeyHandler` is false. Caravellius polls its own input through the Lua
functions that Scarab provides.

**Coordination rule for this phase:** any change to a sunlight input signature also
lists the Scarab wrappers and Caravellius call sites it affects, and those are updated in
the same coordination.

**F1: device list and hot-plug.** Expose the connected pads and their names, and raise
connect and disconnect events. This needs new `IInputHandler` methods, so the mock is
updated in the same change.
- Test items: a scripted connect and disconnect through the mock is reported once each;
  the null backend reports no devices.

**F2: player slots.** A pad is claimed by a slot, released on request, and reassigned
after an unplug. Slots are a sunlight concept, so the game decides how many there are.
- Test items: unplug and replug keeps the slot; releasing a slot frees the pad; claiming
  an already claimed pad is refused.

**F3: action map.** Named actions bound to keys, buttons or axes, with rebinding at run
time and load and save. Sunlight provides the mechanism. Each game names its own actions
and keeps its bindings in its own files. The file format is decided before implementation.
- Test items: a rebind round trip; a saved file reloads to the same bindings; a
  conflicting binding is reported.

**F4: dead zones and thresholds.** Configurable per pad and per axis, with today's values
as the defaults.
- Test items: a value exactly at the boundary behaves as specified; defaults match today.

**F5: duplicate pads.** A pad added twice is dispatched once.
- Test items: adding the same id twice produces one dispatch per event.

**F6: opt-in default handlers.** The camera handlers become opt-in, and the docs explain
how a game takes over input.
- Test items: with the defaults off, no camera handler fires; the samples still work.

**F7a: rumble interface.** A separate `IGamepadHaptics` interface with `IsSupported`,
`SetVibration(pad, left, right, duration)` and `Stop(pad)`, strengths from 0 to 1. It
gets a factory, a null implementation that reports unsupported, and a recording mock.
Nothing is wired to a real device.
- Test items: a game that checks `IsSupported` can skip an effect; calls reach the mock
  with the exact arguments; the null implementation never errors.

**F7b: rumble backend.** A real implementation, once a platform that vibrates is chosen.
raylib's desktop build does not vibrate, so this may need another library or a custom
backend. Real behaviour can only be proven here, so this item stays open until then.
- Test items: on the chosen platform, each motor responds and stops on `Stop`; a
  disconnected pad reports unsupported.

## Later

### L1: per-layer camera and parallax
Decisions taken:
- Factors come from two sources: Tiled's `parallaxx`/`parallaxy` (libtmx already reads
  them) and a code API (`SetLayerParallax`). The code value overrides the Tiled value.
- A sprite follows its layer's factor only if it is in screen space. World-space
  sprites stay in map coordinates.
- Each layer's camera offset is `camera × factor`, and the map's parallax origin is
  respected. A factor of 1 is unchanged.

Test items:
- A factor of 1 is byte-identical to today.
- A factor of 0.5 moves the layer half as far as the camera, against a reference.
- The parallax origin shifts the layer as expected.
- The clamp and margins keep each layer's visible area inside its bounds.
- Each view applies the factor to its own camera.
- The code API overrides the Tiled value.
- A screen-space sprite follows its layer's factor; a world-space sprite does not.

### L2: per-object opacity
Decision taken: option A. The object's custom `opacity` property (0 to 1) is used when
present. Tiled objects have no opacity field of their own, but custom properties can be
set on any object in the editor. When the property is absent, the object uses its
layer's opacity, which is the current behaviour.

Test items:
- With no property, output is identical to the layer opacity.
- A property of 0.5 halves the alpha of the drawn colour.
- An invalid value logs a warning and falls back to the layer opacity.
- A text object follows the same rule once B1 is done.

### L3: sprite draw order within a layer
- Add `SetZOrder(int)` to `Sprite`, default 0.
- Sort each layer's sprites with a stable sort, only when a z-order changed. Equal
  values keep insertion order.
- Z-order applies only within a layer. Layers already order sprites between each other.

Test items:
- With every z-order at 0, output is byte-identical to today.
- A higher z-order is drawn after a lower one on the same layer.
- The sort is stable: equal values keep insertion order.
- Sprites on different layers keep the layer order, whatever their z-values.

## Docs, samples and tests

Done last, once the features they describe are settled: tidy the CHANGELOG (its
Unreleased sections have grown long), update the README and sample docs, and extend the
samples and tests where a feature needs a demonstration.

## Process

- Items are closed one at a time, in phase order, unless the owner decides otherwise.
- Each item is closed only when its test items pass and the owner has confirmed it.
- Item 7 of the original review (manual review of documentation and code) stays manual
  until an open-source tool exists to help. It should be revisited at that point.
