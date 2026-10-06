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
| A1 | AddressSanitizer job in CI | DONE |
| A2 | Reject non-orthogonal maps with a clear error | DONE |
| A3 | Key codes match raylib by name, with compile-time tables | DONE |
| A4 | Document `Concurrent::Timer` hazards in its header | DONE |
| A5 | Consolidate planning docs into this file (remove TODO, FIXME, MISSING_FEATURES) | DONE |
| A6 | Reproduce the scroll-past-boundary FIXME, then close or fix it | DONE |
| A7 | Fix `ScriptProcessor` deleting derived commands through `BaseCommand*` (found by ASan in A1) | DONE |

**A1: AddressSanitizer in CI.** Add one Linux job, using the default GCC with
`-fsanitize=address`, that builds and runs the full test suite with leak detection. It is
Linux-only to keep build time down; Linux is the fastest runner and catches the same
memory errors. AddressSanitizer also exists in MSVC (`/fsanitize=address`), so a Windows
job can be added later if the Linux job finds problems that are worth checking there too.
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

**A5: consolidate planning docs.** This file is the only plan. Remove `doc/TODO.txt`,
`doc/FIXME.txt` and `doc/MISSING_FEATURES.md`, moving any open item into this file first
(their content is already here: the FIXME is A6, the TODO items are in C, D, E and L,
and the open gaps are in "Docs, samples and tests"). Update the "Known issues" section
of `CLAUDE.md` to point here. The CHANGELOG keeps its historical mention of
`MISSING_FEATURES.md`, which is history rather than a live reference.
- Test items: none of the three files exists; `CLAUDE.md` points to this file; every open
  item from the three files appears here with a status.

**A6: scroll-past-boundary FIXME.** `SetCameraPosition` deliberately does not clamp, but
the camera was not the cause. The collision lookup (`TileMapToTileMatrix`, then `GetTile`)
checked row and column only against zero, so a sprite near the far edge produced a row or
column at the map's size and `GetTile` read `gids[]` past its end. ASan reports this as a
heap-buffer-overflow at `GetTile`, with the camera at its default position: the viewport
origin alone pushes a sprite in the last strip of the map past the edge. A camera offset
widens that strip. Fixed: the lookup refuses positions off the map, `GetTile` returns an
empty tile for them without reading, and the tile-animation path and gid table have the
same bound checks. Tests: `tests/test_tilelookup_bounds.cpp` (the lookup and `GetTile` cases)
and `tests/test_tilelookup_tilesets.cpp` (a gid past the tileset, and an animation frame past
it, drawn through the renderer). Each fails on the old code and passes on the fix.
- Peer impact: the earlier note that Caravellius is unaffected because the game clamps the
  camera was wrong - the bad read does not depend on the camera. Any game whose sprites
  reach the last strip of the map can hit it, and the fix changes that collision result
  from a wrong tile to no tile. Caravellius has not hit it so far (the maintainer's
  observation, not a test). A stronger check against Caravellius is planned at the end of
  the master plan, with its session.

**A7: ScriptProcessor command deletion.** Commands are queued as `BaseCommand*` but are
larger derived structs, and `BaseCommand` had no virtual destructor. Deleting through the
base pointer is undefined behaviour; the ASan job reports it as `new-delete-type-mismatch`
in `ScriptProcessor::Clear()`. The fix is a virtual destructor on `BaseCommand`.
- Test items: the ASan job passes on the fixed code and failed on the old code; the
  scripting tests pass unchanged.

## Phase B: tile, text and remaining objects

| # | Item | Status |
|---|------|--------|
| B1 | Draw Tiled text objects (`OT_TEXT`) | DONE |
| B2 | Draw Tiled tile objects (`OT_TILE`) | DONE |
| B3 | Warn on unknown object and layer types | DONE |

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

**B3: unknown types, closed with nothing to warn about.** libtmx gives every layer and object kind the renderer can receive: its layer kinds are the tile layer, object group, image layer and group, and it skips any other element while parsing. Its object kinds cover all seven Tiled shapes, and it turns an object with no shape into a point. So the `L_NONE` and `OT_NONE` cases cannot occur; they remain only so the switch covers every enumerator. Unknown element names are dropped by libtmx before the renderer sees them, so there is nothing for a warning in the renderer to catch.

## Prerequisites

Done before Phase C, because the thick primitives work on these shapes and on the engine's clip. These are API
changes, so they are not part of Phase A. Their peer impact is recorded in [doc/BREAKAGES.md](BREAKAGES.md).

| # | Item | Status |
|---|------|--------|
| R1 | The engine clip is the only clip mode: `SUNLIGHT_SOFTWARE_CLIP`, the software cut and the per-pixel primitives are removed; `IEngine` gains `DrawLine` and `DrawEllipseOutline`, and loses `SetPixel` | DONE |
| G1 | Move `stCoordinate2D`, `stSize2D` and `stDimension2D` from `SunLight::TileMap` to `SunLight::Base` (`base/primitives.h`), so sprites and canvases use them too | DONE |

**R1: engine clip only.** Committed as `55ba435` on `refactor/engine-clip-only`, with the shapes benchmark mode and `samples/shapes/resources/map/stress.tmx`. Shapes are drawn by the backend, and tiles, text and shapes are cut by the engine's clip. Stress map on matched Release builds: about 1.39 ms per frame, against 2.26 ms with the software method.
- Test items, all met:
  - The full suite passes: 356 test cases (`tests/test_shape_engine_calls.cpp` adds five: the four edges of a rectangle at thickness 1, one ellipse outline with its centre and radii, a polyline with no closing edge, a polygon with its closing edge, and a two-point polygon with none).
  - Three deliberate faults each fail the test written for them: the edge inset (`1` to `0`), the edge thickness (`1` to `2`), and the polygon closing rule (`> 2` to `> 1`). The renderer file was restored byte-for-byte after each.
  - The full suite passes under AddressSanitizer, with no reports.
  - Every sample builds.
  - The clip's edge rule is checked by the clip test in `tests/test_viewport_semantics.cpp`.
- Verified by eye, not by test: the nested clip stack in `RaylibEngine` (it needs a real window), and the samples' output (text, shapes, multiview).
- Not repeatable: the pixel comparison of the shapes sample against the software method. It was done before the flag was removed, so the owner should take it as confirmed.
- `doc/BREAKAGES.md` item 15 is written.

**G1: shared geometry.** The three integer structs are plain geometry, not tile-map data, and sprites and
canvases already use them. Moved to `SunLight::Base` with no aliases kept, so Scarab's references are
renamed when integration starts. `stRectangle` (floating-point) is not merged with them yet.
- Test items: the full suite passes; the text, objects and tilemap samples build; Scarab's 12 references
  are listed in `doc/BREAKAGES.md` item 14.

## Phase C: thick primitives

| # | Item | Status |
|---|------|--------|
| C1 | Thick lines, with square caps | DONE |
| C2 | Thick polylines and polygons | DONE |
| C3 | Thick rectangles | DONE |
| C4 | Thick ellipses | DONE |
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
- The engine clips. The renderer draws each span with `IEngine::DrawFilledRectangle` inside
  the viewport's engine clip (`BeginClip`/`EndClip`), so no new engine method is needed. A native primitive
  is only considered later, if the measured span count is too high.

**Implementation approach.** Each stroke is drawn as one filled span per scanline,
inside the viewport's engine clip. A per-pixel draw loop is not used, because it costs one
engine call per pixel.

**C1: thick lines.** Spans through the engine clip, as decided. Width 1 stays on `IEngine::DrawLine`, so it
is the engine's line, not `LineBresenham` (removed in R1).
- Decided while building it:
  - A stroke's width comes from the object's `line_width` property, as an int or a float. The group's property is not
    read: libtmx 1.10 does not keep an object group's properties, so the group fallback in the plan cannot be built.
  - Only polylines take the width in C1. Rectangles and polygons stay one pixel until C2 and C3.
  - A pixel is in a stroke when its centre is inside the rectangle around the axis, half-open on every side. The axis
    runs through the centres of the end pixels. A boundary tolerance of 1e-9 decides centres on an edge in exact arithmetic.
  - Screen width is the map width times the zoom, rounded half-up, at least 1 (`Shape::ScreenLineWidth`, in `src/renderer/shape/shapeprimitives.h`). The thick-line span code lives there too.
- Test items, in `tests/test_thick_lines.cpp` (9 cases):
  - Width 2, width 3 with its one-pixel caps, and zoom 2 with width 3 (six rows): hand-derived spans.
  - Zoom 1.5 with width 3: 4.5 rounds up to 5 rows.
  - Width 1 and no property: the engine's line, no spans.
  - A diagonal line: one span per row, each row once, far fewer spans than pixels.
  - Widths 2 to 5 at four angles: the spans cover exactly the pixels a separate reference (computed from the raw
    endpoints) accepts.
  - A line left of the viewport still produces spans past its edge: the engine's clip does the cutting.
- Mutation checks (each fails its test): cap length `(w - 1) / 2` to `w / 2`; the edge rule excludes `+h` inclusively;
  the zoom rounding drops the half-up.
- Full suite under AddressSanitizer: 365 test cases pass with no reports.
- Not yet covered: the spans are not compared with the reference for a line crossing a viewport edge or corner. The
  engine's clip is what cuts them there, and the clip itself is tested by the viewport tests.
- Not yet measured: the cost of a long thick line that runs far off the viewport (its rows are computed in full).
- `doc/BREAKAGES.md` item 16 is written: polylines at a zoom other than 1 are wider than before.

**C2: thick polylines and polygons.** Joins have no gaps.
- Decided while building it:
  - The join is a round join: a disc of half the width at each vertex that has two segments. A closed polygon joins
    every vertex, including the one where the closing edge meets the first point. An open polyline joins only its
    interior vertices.
  - A polygon takes the same `line_width` property as a polyline (from its object). Width 1 stays the hairline path, so
    its output does not change.
  - One path renderer draws both (`Shape::DrawStrokedPath`). A single segment gives the same output as C1.
  - Square caps are only at the two ends of an open path. At a join the round join is the corner: a cap there would
    cut a flat step into its arc.
- Test items, in `tests/test_thick_lines.cpp` (the "Thick joins" suite, 3 cases):
  - A sharp corner, where the second segment doubles back over the first: the spans equal the reference of the two
    strokes plus the join.
  - A closed triangle: the spans equal the reference with a join at all three corners.
  - A two-point polygon: one stroke, with no closing edge and no join.
- Mutation checks: removing the joins fails the sharp-corner and triangle cases; closing a two-point polygon fails the
  two-point case.
- Full suite under AddressSanitizer: 370 test cases pass with no reports. Every sample builds.
- Not yet checked by eye: `samples/shapes/resources/map/thick.tmx` has a width-3 polygon and a width-4 sharp corner.
- Not decided: whether the owner wants miter joins instead of round joins. Miter joins would need a limit for very sharp
  corners.
- `doc/BREAKAGES.md` item 16 covers polygons too.

**C3: thick rectangles.** Four thick edges.
- Decided while building it: a rectangle is a closed path of its four corners, drawn by the same path renderer as C2.
  Each corner is a round join, and no edge has an open end. A rectangle takes the object's `line_width` property.
  Width 1 stays the four hairline edges, so its output does not change.
- Test items, in `tests/test_thick_lines.cpp` (the "Thick rectangles" suite, 2 cases):
  - A thick rectangle matches the reference: four closed edges with a round join at each shared corner.
  - A thick rectangle has no open ends: no engine line is drawn, and the spans match the closed reference.
- Mutation check: removing the joins fails the rectangle cases, as well as the C2 corner and triangle cases.
- Full suite under AddressSanitizer: 372 test cases pass with no reports. Every sample builds.
- Not yet checked by eye: `samples/shapes/resources/map/thick.tmx` has a width-5 rectangle, bottom right.

**C4: thick ellipses.** A ring between an outer and an inner radius.
- Decided while building it: width 1 stays the engine's ellipse outline (`IEngine::DrawEllipseOutline`), as in C1, since
  `MidPointEllipse` is gone. A wider outline is a ring: a pixel is in it when it is strictly inside the outer ellipse
  (radius plus half the width) and not strictly inside the inner one (radius minus half the width). When the inner
  radius is not positive, the ring is a filled ellipse. The ellipse takes the object's `line_width` property.
- Test items, in `tests/test_thick_lines.cpp` (the "Thick ellipses" suite, 2 cases):
  - A thick ellipse is the ring between its outer and inner radius: the spans equal the reference ring, and no engine
    ellipse is drawn.
  - A width that reaches the centre gives a filled ellipse, matching the reference.
- Mutation check: ignoring the inner ellipse fails the ring case.
- Full suite under AddressSanitizer: 374 test cases pass with no reports. Every sample builds.
- Not yet checked by eye: `samples/shapes/resources/map/thick.tmx` has a width-4 ellipse at the bottom.
- Not covered by a test: an ellipse crossing the viewport edge is cut by the engine's clip; the test only compares the
  pixels inside the view's own rectangle, not the cut itself.

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

## Structure: a smaller TileMapRenderer

`src/renderer/tilemaprenderer.cpp` is about 3,300 lines, and it holds most of the renderer's work. The
helpers that were not part of the renderer have already moved to their own modules: `shapeprimitives`
(clip, stroke and screen width), `shapeobjects` (the `line_width` property), `externalresources` (external
tilesets and templates) and `maporientation` (orientation names). S1 moves the rest.

| # | Item | Status |
|---|------|--------|
| S1 | Split `TileMapRenderer` into one header and source per concern, so no single file holds more than one concern | TODO |

- Concerns to move, in this order: text objects; tile objects and their animation; object drawing (`DrawObjects`
  and its dispatch); view passes and multi-view rendering; map loading and its callbacks; the sprite registry; the
  default input handlers. Each move is one commit.
- Target: `tilemaprenderer.cpp` under 1,000 lines, and no new file over 600.
- No behaviour change in any step: the full suite passes, every sample builds, the AddressSanitizer run is clean,
  and the shapes stress benchmark stays within its usual spread.
- Public API of `TileMapRenderer` does not change. If a step has to change it, the change goes in
  `doc/BREAKAGES.md` first.
- Every value in new code is named, as in the other modules.
- Start after Phase C is closed, so the thick-line work is not moved underneath it.

## Docs, samples and tests

Done last, once the features they describe are settled.

| # | Item | Status |
|---|------|--------|
| T1 | Doxygen warnings: stale `@param` names, a stray `@ITileMapListener` and `\The` typo, unresolved `\link` targets | TODO |
| T2 | Direct tests for the map-loading pipeline (parsing a real `.tmx` through the mocks) | TODO |
| T3 | Direct tests for `Run()`'s input, update and collision dispatch (reachable now the frame loop can be driven) | TODO |
| T4 | Tidy the CHANGELOG, whose Unreleased sections have grown long | TODO |
| T5 | Update the README and sample docs; extend samples where a feature needs a demonstration | TODO |

- T1: each warning is fixed, and the Doxygen run reports no new warnings.
- T2 and T3: tests fail on a deliberate break of the loader or dispatch path.
- T4 and T5: checked against the features as they are when this item is reached.

## Out of scope for this plan

- **SDL backend.** The README says the project aims to extend to SDL. Only raylib exists
  today. The SDL backend is a large change of its own, and will be handled in a separate
  plan, not in this one.

## Design decisions

- **libtmx types in the public API (decided, as is).** `stLayer::pLayer`, `stTile::pTile`
  and `stMapInfo::pMap` expose libtmx's own structures. libtmx is a supported dependency,
  and these fields are for advanced use. Whether to hide them behind opaque handles is a
  possible future discussion, not part of this plan.

## Process

- Peer breakages: [doc/BREAKAGES.md](BREAKAGES.md) lists every change that affects Scarab or Caravellius, and what to do when integration starts. Add an entry for each such change as it is made. The file is removed when this plan is merged to `main`, after the peers have been told.
- Items are closed one at a time, in phase order, unless the owner decides otherwise.
- Each item is closed only when its test items pass and the owner has confirmed it.
- Item 7 of the original review (manual review of documentation and code) stays manual
  until an open-source tool exists to help. It should be revisited at that point.
