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
| C5 | Points as squares | DONE |

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

**C5: points.** A point of size `s` at zoom `z` is a filled square of side `round(s × z)`, minimum 1. This also
closes the point-object gap from B.
- Decided while building it:
  - The size comes from the object's `point_size` property (int or float), default one map unit, read the same way as
    `line_width`.
  - The square is drawn with `IEngine::DrawFilledRectangle` inside the shape's clip, not with `SetPixel` (removed in R1).
    Its top-left corner is at the point's position.
  - The side uses the same rule as a line's width (`Shape::ScreenLineWidth`).
- Test items, in `tests/test_thick_lines.cpp` (the "Points" suite, 4 cases):
  - A point is a filled square of side `round(size × zoom)`, with its top-left at the point.
  - The side rounds half-up: size 1.25 at zoom 2 is 3 pixels.
  - A point is at least one pixel, and a point with no `point_size` is one map unit.
  - A point on the viewport's top-left edge is cut by the strict clip, which starts one pixel in.
- Finding: libtmx makes any object with a `height` attribute a rectangle, even a zero-sized one. A point is written
  as a `<point/>` child with no width or height attribute, as Tiled writes it.
- Mutation checks: floor instead of half-up, and no one-pixel minimum, each fail their tests.
- Full suite under AddressSanitizer: 378 test cases pass with no reports. Every sample builds.
- Not yet checked by eye: `samples/shapes/resources/map/thick.tmx` has three points at the bottom right, sizes 4, 10 and 2.5.

**Across all of Phase C**
- Byte-identical harnesses at default width (the animation and camera traces).
- Full suite under AddressSanitizer.
- Mutation checks: each rule has a deliberate mutant (wrong cap length, wrong edge rule,
  wrong zoom rounding) that a test must catch.

## Phase D: object rotation

| # | Item | Status |
|---|------|--------|
| D1a | Rotate rectangle, polyline and polygon outlines about their pivot | DONE |
| D1b | Rotate ellipses (rotated ring: the inside test runs in the ellipse's own frame) | DONE |
| D1c | Points: not applicable. Tiled 1.11 does not offer rotation for point objects | N/A |
| D1d | Rotate tile and text objects (needs `IEngine` to draw with a rotation) | DONE |

**Verified pivots (Tiled 1.11.0, checked by hand on a test map).** Each object turns about its stored X and Y,
which is the point Tiled keeps when a rotation is set. Rotation is clockwise on screen, with a 90° turn for
each test:
- Rectangle (120 × 80 at 100, 100): pivot is the top-left corner. After 90° it sits left of and below the
  point, covering x 20–100 and y 100–220. Confirmed.
- Ellipse (160 × 80 at 100, 100): pivot is the top-left corner. After 90° its centre is at (60, 180), left
  of and below the point. Corrected from (60, 160): the unrotated centre is (180, 140), not (160, 140).
- Tile object (32 × 32 at 100, 100): pivot is the bottom-left corner. After 90° it sits right of and below
  the point, covering x 100–132 and y 100–132. Confirmed.
- Point: Tiled does not rotate it. D1c is not applicable.
- Polyline and polygon points are relative to the object's X and Y, so they turn about that point.

**D1d: rotated tile and text objects (DONE).** A new `IEngine::DrawTextureRotated` draws one texture turned about
an anchor, with an origin offset (raylib's `DrawTexturePro`). A new `IEngine::DrawTextRotated` and `IFont::DrawTextRotated`
draw a line of text turned about its top-left corner (`DrawTextPro`). A turned tile is one texture turned about its
bottom-left corner, Tiled's pivot. A turned text block turns each line's top-left corner about the block's top-left
corner, and the line turns with it. A turned text block is never culled by its unturned box.
- Test items, in `tests/test_rotated_tiles_text.cpp` (the "Rotated tiles and text" suite, 3 cases):
  - A tile turned 90°: one turned texture, anchored at the object's point, origin (0, height).
  - A two-line text block turned 90°: the first line at the pivot, the second line swung to the left.
  - A text block turned into view, although its unturned box is outside the viewport, is drawn.
- Mutation checks: the tile's origin at the top-left corner, culling by the unturned box, and the lines turned the
  wrong way each fail their case.
- Breaking: `IEngine` and `IFont` gain pure virtual methods (BREAKAGES item 18).
- Sample: `samples/shapes/resources/map/rotation.tmx` has turned tiles and text at 90° and 45°. Checked by eye: it works.
- Full suite under AddressSanitizer: 390 test cases pass with no reports.

**D1a: rotated shape outlines (IN PROGRESS).** Each point of a rectangle, polyline or polygon turns about the
object's stored point in map units, before the zoom is applied. The turn is clockwise on screen, and it is exact
for quarter turns: cosine and sine within 1e-12 of zero count as zero (`Shape::Rotate`). An object with no
rotation gets 0, so its output is unchanged.
- Test items, in `tests/test_thick_lines.cpp` (the "Rotated shapes" suite, 4 cases):
  - A rectangle turned 90°: four hairline edges at the expected corners, x 30–110 and y 110–230.
  - A polyline turned 90°: a point to the right of the stored point goes below it.
  - A thick triangle and a thick rectangle turned 90°: the spans equal the reference of the turned corners.
- A polyline turned 45°: its end lands on the truncated pixel (145, 145), and a thick one matches the reference.
- Rotation 0: the existing shape tests cover this (the output for an unrotated object is unchanged).
- Mutation checks: ignoring the rotation, and flipping the direction, each fail the four rotated cases.
- Sample: `samples/shapes/resources/map/rotation.tmx` (a rectangle, a polyline and a polygon, each with a pivot
  marker). Checked by eye: the shapes turn clockwise about their markers.
- Full suite under AddressSanitizer: 382 test cases pass with no reports.

**D1b: rotated ellipses (DONE).** The ring is tested in the ellipse's own frame: each pixel centre is turned
back by the ellipse's rotation before the outer and inner tests. The centre is the box's centre, turned about the
stored point. A one-pixel ring is used when the ellipse is turned; an unturned one-pixel outline stays the engine's.
- Test items, in `tests/test_thick_lines.cpp` (the "Rotated ellipses" suite, 3 cases):
  - A thick ellipse turned 45°: the spans equal the reference ring in its own frame.
  - A one-pixel ellipse turned 90°: a one-pixel ring, not the engine's outline.
  - An unturned one-pixel ellipse: still the engine's outline.
- Mutation checks: ignoring the frame's turn fails the 45° and 90° cases. Turning the frame the other way fails the
  45° case. (Flipping only the sign of the perpendicular coordinate changes nothing, because it is squared.)
- Sample: `samples/shapes/resources/map/rotation.tmx` has a 45° ellipse, with a pivot marker. Checked by eye: it works.
- Full suite under AddressSanitizer: 387 test cases pass with no reports.

**Test map.** The Tiled files are in `Spool/test` for now. Once D1a is done, the same map goes to
`samples/shapes/resources/map/` with a copy of the tileset image beside it.

- D1a test items: rotation 0 is byte-identical to today; a 90° rectangle matches the reference of its rotated
  corners; a rotated polyline and polygon turn about the object's X and Y.
- D1b test items: a 90° ellipse matches a reference ring in its own frame; a rotated thick outline matches the
  C4 reference.
- D1d test items: a rotated tile object uses the bottom-left pivot; a rotated text object uses the top-left pivot.

## Phase E: isometric, staggered and hexagonal maps

`TileMapRenderer` has no seam today between drawing a tile map and drawing an *orthogonal* one: the
grid math is woven into the class itself. Isometric, staggered and hexagonal maps need that math behind
an interface, so the renderer keeps everything that is not projection-specific (the view/camera/clip
machinery, sprites, collision dispatch, shape/text/tile drawing, animation, the frame loop), and each
map orientation supplies its own tile geometry.

| # | Item | Status |
|---|------|--------|
| E0 | Projection seam: `IMapProjection` + `OrthogonalProjection`, with visible-tile culling. No behavior change | DONE |
| E1a | Verify Tiled's isometric conventions by hand (tile anchor, object coordinate space, map pixel bounds, scroll step) | DONE |
| E1b | `IsometricProjection`: tile to screen (diamond math), screen to tile matrix, map pixel size, default scroll step | DONE |
| E1c | An isometric sample map and a real-window check by the owner | DONE |
| E2a | Verify Tiled's staggered conventions by hand, all 4 `stagger_axis`/`stagger_index` combinations | DONE |
| E2b | `StaggeredProjection`: tile to screen, screen to tile matrix, map pixel size, default scroll step | DONE |
| E2c | A staggered sample map and a real-window check by the owner | DONE |
| E3a | Verify Tiled's hexagonal conventions by hand, both `stagger_axis` values, both `stagger_index` values, and the effect of `hexsidelength` | DONE |
| E3b | `HexagonalProjection`: tile to screen, screen to tile matrix, map pixel size, default scroll step | DONE |
| E3c | A hexagonal sample map and a real-window check by the owner | DONE |

**E0: the projection seam (DONE).** What is orthogonal-specific today, found by reading the renderer rather
than assumed: the map's pixel size (`m_nMapWidth`/`m_nMapHeight`, 23 call sites that only ever treat it as an
opaque number); a tile layer cell's screen position (`DrawLayer`'s `(col * tile_width, row * tile_height)`,
using the resolved tile's own tileset size, not the map's); a tile matrix cell's view-space rectangle
(`GetTile`, consumed only by `CollisionManager`); the inverse, view-space to tile matrix
(`TileMapToTileMatrix`); and the default scroll step (`CreateView`, `LoadMap`, and - found during review -
`MoveCameraUp`/`MoveCameraLeft`'s scroll-boundary clamp, which read the map's tile size directly as a rounding
term). `DrawLayer`'s loop structure, the alignment switch, the rest of the scroll clamps and everything
outside these five buckets stay in `TileMapRenderer` unchanged - they already only consume the buckets' output
as opaque values, never the map's orientation itself.

- `IMapProjection` lives in its own module, `renderer/projection/` (the `shape/`/`map/` pattern), with
  `OrthogonalProjection` as the only implementation for now. `TileMapRenderer` picks one in `LoadMap` from
  `m_pTmxMap -> orient`, the same shape as `EngineFactory`/`WindowFactory` pick a backend. An orientation with
  no implementation keeps today's clear error.
- **Visible-tile culling** is part of E0, not deferred: `DrawLayer` currently drew the whole grid every frame
  and relied on the clip. A sixth projection method, `VisibleTileRange`, bounds the loop to the tiles that can
  be seen, padded by a fixed one tile either side (`__CULL_PADDING_TILES`) - `DrawLayer` always draws a cell at
  its resolved tile's own tileset size, never a per-tile image-size override, so the padding needs no larger
  margin than that.
- **Safety net for mixed tile sizes.** If a layer's tiles come from a tileset whose own declared size differs
  from the map's declared size, a culled range computed from a single step size can genuinely miss visible
  cells far from the origin - a scale mismatch that grows with distance, not a fixed-offset case any padding
  could fix. `LoadMap` checks this once (`m_bUniformTileGrid`, comparing every tileset's own size against the
  map's); `DrawLayer` falls back to the exact pre-E0 full-grid loop whenever they disagree, trading the
  performance win for guaranteed correctness on that (rare, exotic) map.
- Test items, in `tests/test_tileculling.cpp` (the "Tile culling" suite, 4 cases): a map much larger than the
  viewport produces a bounded tile count, not `width x height`; a map whose tilesets disagree on tile size
  falls back to visiting every cell; the same layout with matching tile sizes is still exact once every cell is
  visible; and a mismatched tileset smaller than the map's declared size is not wrongly excluded near the far
  edge - the case that specifically needs the uniform-grid fallback, not just the padding.
- Mutation checks: removing the padding fails the first case; disabling the uniform-grid check fails the last.
  Each was confirmed by reverting the fix, seeing the test fail, then restoring it.
- Full suite (394 cases) passes under AddressSanitizer with no reports. Every sample builds cleanly.

**E1a: verify Tiled's isometric conventions (DONE).** The same practice as the object-rotation pivots: checked
by hand in Tiled before any isometric formula was written, not assumed - and it caught the same kind of trap
again. A probe map was built with distinct solid-color tiles (not real art - faint/translucent tile images
turned out to be unmeasurable at the pixel level), exported to a PNG with Tiled 1.11's "Export As Image", and
measured pixel-by-pixel:

- **Map pixel bounds**, confirmed exact at two map sizes (160x80 for a 3x2 map, 224x112 for a 3x4 map, both
  64x32 tiles): `(mapWidthCells + mapHeightCells) * tileWidth/2` by `... * tileHeight/2`.
- **A tile layer cell's own top-left corner** (its `tileWidth x tileHeight` bounding box), confirmed exact
  across all 18 cells of both maps:
  `screenX = (col - row + mapHeightCells - 1) * (tileWidth/2)`, `screenY = (col + row) * (tileHeight/2)`.
  The lattice step always uses the MAP's own declared tile size, never a resolved tile's own image size - a
  probe tileset with 32x32 tile images on a 64x32 declared grid still landed exactly on the 64x32 lattice.
  A formula half-remembered from Tiled's own source (`mapHeightCells * tileWidth/2`, no `-1`) looked plausible
  and matched the FIRST map size tested (2 cells tall) - only testing a second, differently-tall map (4 cells)
  caught it as wrong, the same lesson as the comment-reasoning mistakes this file already warns about.
- **Tile *object* coordinates** (a stamped tile on the map, not a tile layer cell) follow a related but
  distinct, separately-measured formula, confirmed exact at three typed values (not click-placed - Tiled's
  Insert Tile tool does not snap to the isometric grid by default, which produced unusable noise at first):
  `objScreenX = storedX - storedY + (mapHeightCells - 1) * (tileWidth/2) + tileHeight/2`,
  `objScreenY = (storedX + storedY) / 2`, at the object's own bottom-left corner (Tiled's usual tile-object
  anchor, already used by `DrawTileObject`). This answers E1a's open question - object coordinates ARE
  projected, not plain pixel space - but `IMapProjection` does not yet have a method for it; no projection
  consumer needs isometric object placement yet, so it is a tracked follow-up, not part of E1b.
- Default scroll step is a design choice, not a Tiled convention to verify: `IsometricProjection` uses one
  full tile step, the same convention `OrthogonalProjection` already uses.

**E1b: `IsometricProjection` (DONE).** Implements all six `IMapProjection` methods with the formulas E1a
confirmed. `TileDrawPosition` gained a `tmx_map *pMap` parameter it did not have in E0 - a gap E1a's own
findings exposed: the isometric lattice step needs the map's own tile_width/tile_height, which was not
available to that method before (`OrthogonalProjection` never needed it, since orthogonal steps by the
resolved tile's own size, already passed in). `ViewToTileMatrix`/`VisibleTileRange` invert the confirmed
formula algebraically (solving the two linear equations for col/row), at the same AABB precision
`TileMapToTileMatrix` already uses for orthogonal - not a true diamond hit-test, which no consumer needs today.
`m_bUniformTileGrid`'s mixed-tileset fallback (E0) still applies uniformly regardless of orientation; isometric
does not actually share orthogonal's risk (its lattice step never depends on a resolved tile's own size), so
the fallback is occasionally more conservative than isometric strictly needs, but never incorrect.

- Test items, in `tests/test_isometricprojection.cpp`: `MapPixelSize` and `TileDrawPosition` against the exact
  pixel values E1a measured, for both map sizes (not values this engine's own formula could have produced by
  construction - independently observed facts); a layer offset still shifts the result; a resolved tile's own
  size does not move the lattice position (unlike orthogonal); `ViewToTileMatrix` round-trips every cell of a
  3x4 map (probed one pixel inside each cell's own top-left corner, not its bounding box's center - adjacent
  cells' boxes overlap by half their width/height on this lattice, so a center-probe can land exactly on a
  neighbor's own corner and round-trip to the wrong cell, a trap the first version of this test fell into); out
  -of-bounds coordinates are refused; `DefaultScrollStep` matches orthogonal's convention; `VisibleTileRange`
  covers the whole map for a whole-map rectangle, is bounded (not the full grid) for a small one, and clamps to
  the grid for an oversized one. A renderer-integration test loads a real isometric map and checks `DrawLayer`
  produces the exact measured positions end to end.
- Mutation check: removing the lattice's height-dependent shift term fails 5 of the 12 isometric test cases
  (including the renderer-integration one), confirmed by reverting the fix, seeing the tests fail, then
  restoring it.
- `tests/test_maporientation.cpp` updated: isometric now loads (a new, dedicated test case) instead of being
  refused; staggered and hexagonal are still refused, and the "refused map doesn't block the next one" case
  now uses hexagonal as its refused example instead of isometric.
- Full suite (407 cases) passes under AddressSanitizer with no reports. Every sample builds cleanly.

**E1c: isometric sample map and real-window check (DONE).** `samples/isometric/` (new sample, mirroring
`samples/tilemaprenderer/`'s structure and the same pan/zoom keys): an 8x6 checkerboard of two green diamond
shades, with a red marker tile at (row 0, col 0) and the opposite corner (row 5, col 7) - the diamond's own top
and bottom vertices per the confirmed formula, not its left/right ones, since those sit `tileWidth/2` off
center on each side. Diamond tile art generated for this sample specifically (an actual 64x32 diamond polygon
with transparent corners, not a solid square) - E1a's probe tiles were plain squares, fine for pixel
measurement but not for a visual check of real diamond tiling. Checked by eye by the owner: every tile reads
as one continuous, seamless diamond mosaic across the whole 8x6 grid, no gaps, no overlapping or misaligned
seams, and the two marker tiles land exactly where the formula predicts.

**E2a: verify Tiled's staggered conventions (DONE).** Same practice as E1a: measured against a real Tiled
export (`tmxrasterizer`, Tiled 1.11.0's own CLI rasterizer - pixel-identical to its GUI "Export As Image",
scriptable, so every combination below was rendered and measured mechanically rather than read off a single
screenshot by eye) rather than assumed. A staggered map has two independent settings libtmx exposes on
`tmx_map` that orthogonal and isometric never touch - `stagger_axis` (`SA_X`/`SA_Y`: whether columns or rows
are the staggered ones) and `stagger_index` (`SI_EVEN`/`SI_ODD`: which parity of them is shifted) - so all 4
combinations were verified, not just Tiled's own default, since E1a already showed a formula can match one
case by coincidence and still be wrong. A probe tileset of 6 solid, non-square 64x48 colour tiles (avoiding
any 2:1 or square coincidence that could hide a mixed-up width/height term) was placed at 5 known matrix
positions (the 4 corners and one interior cell) on 3 map sizes per combination - a baseline, one that varies
only row count, one that varies only column count, so a row-dependent and a column-dependent term cannot be
confused with each other, the same isolation E1a's two differently-tall isometric probes used. 12 maps in
total, each rendered and measured pixel-exactly by script.

- **A tile layer cell's own top-left corner**, confirmed exact across every combination and all 3 sizes:
  - `stagger_axis = SA_Y` (rows are staggered): `screenY = row * (tileHeight / 2)`; `screenX = col * tileWidth`,
    plus `tileWidth / 2` when the row's own parity matches the stagger index (an even row for `SI_EVEN`, an
    odd row for `SI_ODD`).
  - `stagger_axis = SA_X` (columns are staggered): `screenX = col * (tileWidth / 2)`; `screenY = row * tileHeight`,
    plus `tileHeight / 2` when the column's own parity matches the stagger index - the exact mirror of the Y
    case, confirmed independently rather than assumed from the symmetry.
- **Map pixel bounds**, confirmed exact at all 3 sizes per axis (index-independent - both `stagger_index`
  values produced the same overall image size for a given axis and map size):
  - `stagger_axis = SA_Y`: `(mapWidthCells * tileWidth + tileWidth / 2)` by `((mapHeightCells + 1) * tileHeight / 2)`.
  - `stagger_axis = SA_X`: `((mapWidthCells + 1) * tileWidth / 2)` by `(mapHeightCells * tileHeight + tileHeight / 2)`.
- **Cell bounding boxes overlap their neighbours along the staggered axis**, found while measuring, not assumed:
  a row's (or column's) pitch is only half a tile step, not a full one, so consecutive rows/columns on the
  staggered axis genuinely overlap by half their own rectangle - confirmed directly in the rendered probes,
  where a later-drawn cell's rectangle visibly painted over the bottom (or right) half of an earlier one in
  the overlap region, leaving its top-left corner (the value the formulas above report, and the only part
  `TileDrawPosition` needs) unaffected. This is the same shape of ambiguity E1b found for isometric's diamond
  lattice: a point-based inverse lookup (`ViewToTileMatrix`) cannot be exact from the bounding box alone near
  the overlap, and needs the same AABB-precision approach already used there, not a true per-shape hit test.
- **Tile *object* coordinates** (a stamped object, not a layer cell) were not measured - same deferral E1a
  made for isometric object placement: no projection consumer needs staggered object placement yet, so this
  is a tracked follow-up, not part of E2b.
- Default scroll step is a design choice, not a Tiled convention to verify: `StaggeredProjection` is expected
  to use the same one-full-tile-step convention `OrthogonalProjection` and `IsometricProjection` already use.

**E2b: `StaggeredProjection` (DONE).** Implements all six `IMapProjection` methods with the formulas E2a
confirmed. Unlike isometric, `TileDrawPosition` does not depend on the map's own width or height at all - a
cell's own top-left corner depends only on its row/column and the tile size, confirmed directly from the
probe data (the same corner landed at the exact same pixel across all 3 map sizes). `LoadMap`'s orientation
switch gains an `O_STA` case, and its refusal message (stale since E1b - it still said "only orthogonal maps
are supported" after isometric was already accepted) is corrected to name all three supported orientations.

- `ViewToTileMatrix` started from the same two-candidate, nearest-corner-wins approach `IsometricProjection`
  uses for its own overlapping lattice, reasoning the staggered lattice's cells overlap their neighbour the
  same way. Mutation-testing that reasoning - forcing the method to always keep its first, un-disambiguated
  candidate rather than ever considering the second - left every round-trip test still passing: a single
  direct floor against the cell's own line shift is already exact for the only precision this method
  promises (a point near a cell's own anchor corner). The second candidate and its distance comparison were
  dead, untested complexity once this was found, so they were removed rather than kept.
- `VisibleTileRange` is derived per axis (not guessed) from each axis's own two possible shifts (0 or half a
  tile): the unshifted axis reduces to the exact floor/ceil orthogonal already uses, and the shifted axis
  takes the loosest bound either shift value can produce, so no row/column parity in range is missed. An
  early version's margin test only checked a rectangle starting exactly at the map's own edge, where the
  margin's effect is clamped away regardless of whether it is there - a gap found by mutation-testing that
  test itself (removing the margin left it passing); the fix added a second, interior-rectangle test per axis
  that the margin's removal does actually fail.
- Test items, in `tests/test_staggeredprojection.cpp` (20 cases): `MapPixelSize` against the exact measured
  values for both axes across all 3 map sizes, and a dedicated case confirming it does not depend on
  `stagger_index`; `TileDrawPosition` against the exact measured values for all 4 axis/index combinations,
  plus the no-map-size-dependency finding, a layer-offset case and a resolved-tile-size-independence case;
  `ViewToTileMatrix` round-trips every cell of two differently-staggered 4x3 grids and refuses an
  out-of-bounds coordinate; `DefaultScrollStep`; `VisibleTileRange` covers the whole map for both axes, is
  bounded for a small rectangle, clamps for an oversized one, and (the two added cases above) does not drop a
  cell whose own span starts one pitch before an interior rectangle, on either axis. A renderer-integration
  test loads a real staggered map (libtmx's own default axis/index, confirmed in E2a to be `SA_Y`/`SI_ODD`)
  and checks `DrawLayer` produces the formula-derived positions end to end.
- `tests/test_maporientation.cpp` updated: staggered now loads (a new, dedicated test case, replacing its
  half of the old combined "staggered and hexagonal are refused" case) instead of being refused; hexagonal
  alone is refused now.
- Mutation checks, each confirmed by reverting the fix, seeing the specific tests fail, then restoring it:
  flipping the shift-parity condition fails 7 of 18 `TileDrawPosition`/size-independence cases; removing the
  `MapPixelSize` row-count term fails its own 3 assertions; using a full tile step instead of a half-tile
  pitch in `ViewToTileMatrix` fails the round-trip suite; removing `VisibleTileRange`'s margin fails the two
  interior-rectangle cases added for exactly that purpose.
- Full suite (434 cases) passes under AddressSanitizer with no reports. Every sample builds cleanly.

**E2c: staggered sample map and real-window check (DONE).** `samples/staggered/` (new sample, mirroring
`samples/tilemaprenderer/`'s structure and pan/zoom keys, `-`/`=` to zoom like `samples/isometric/`): an 8x6
grid, `SA_Y`/`SI_ODD` (Tiled's own default), two checkerboard shades, a red marker at (row 0, col 0) and a
gold one at the opposite corner (row 5, col 7). First checked with plain solid-color 64x48 rectangles - the
same choice E1a made for its own probe tiles, the clearest way to see the raw grid math with no art to second
-guess - and confirmed by eye as a clean, continuous brick/staggered weave with no gaps or misaligned seams,
matching `tmxrasterizer`'s own independent render pixel-for-pixel. The owner then asked for diamond tile art
(transparent corners inscribed in each 64x48 rectangle) so the sample would visually read as a mosaic the
same way E1c's isometric sample does - confirmed working: the diamonds interlock into a seamless lattice with
no gaps or misaligned seams, the practical payoff of Tiled's own stated reason for the staggered orientation
("allows a map based on isometric tiles to still have an overall rectangular shape" - a staggered map's half
-tile row pitch is exactly a diamond's own half-height, which is what makes the two techniques produce an
identical mosaic from different storage shapes).
- Two issues found and fixed during the owner's check, both sample-only (no `StaggeredProjection` or
  `TileMapRenderer` change): the sample's `Run()` omitted the base `tilemaprenderer` sample's
  `SetScrollStepSize(1, 1)` call, so held-key panning moved a full tile (64px) per frame instead of 1px -
  fixed by adding the same call. Vertical panning appeared not to work at the default zoom; confirmed correct,
  not a bug - the map's own pixel height ((rows + 1) * tileHeight / 2 = 168) is shorter than the viewport at
  that zoom, so there is nothing to scroll to, the same "viewport taller than map" case `test_camera.cpp`
  already covers for the orthogonal renderer.

**E3a: verify Tiled's hexagonal conventions (DONE).** Same practice as E1a/E2a: measured against real
`tmxrasterizer` exports, not assumed. A hexagonal map carries one parameter beyond `stagger_axis`/
`stagger_index` that staggered maps never had: `hexsidelength`, the length of the hexagon's own two parallel
"flat" edges (perpendicular to the stagger axis) - so the pitch along the staggered axis can no longer be a
fixed half-tile step the way staggered's was; it has to be derived from `hexsidelength` directly. 10 probe
maps were built and measured: the primary combination (`stagger_index = SI_ODD`, `hexsidelength = 20`) at 3
map sizes per axis (a baseline, a row-count-only variant, a column-count-only variant, the same isolation
E1a/E2a used), plus a second `hexsidelength` value (10) and `SI_EVEN` at one size each per axis, to isolate
each parameter's own effect rather than risk conflating them.

- **The axis that is NOT staggered behaves exactly like `StaggeredProjection`, unaffected by
  `hexsidelength`:** for `SA_Y` (pointy-top hexagons, rows staggered), `screenX = col * tileWidth`, plus
  `tileWidth / 2` when the row's own parity matches the stagger index - the identical formula and shift
  amount E2a measured for staggered, confirmed unchanged across both tested `hexsidelength` values. `SA_X`
  mirrors this on `screenY`/column parity, likewise unaffected by `hexsidelength`.
- **The staggered axis's pitch is `(tileDimension + hexsidelength) / 2`, not a fixed half-tile step:** for
  `SA_Y`, `screenY = row * (tileHeight + hexsidelength) / 2` - confirmed exact at both measured
  `hexsidelength` values (20 and 10) and all 3 map sizes, with no row-count-dependent term (unlike isometric's
  lattice). `SA_X` mirrors this on `screenX` against `tileWidth`.
- **Map pixel bounds**, confirmed exact at all 3 sizes and both `hexsidelength` values per axis, and found to
  be index-independent (matching E2a's equivalent staggered finding): `SA_Y`'s width is the same
  `cols * tileWidth + tileWidth / 2` staggered already uses (hexside-independent, since that axis's formula
  is unaffected); its height is `(rows * (tileHeight + hexsidelength) + (tileHeight - hexsidelength)) / 2`.
  `SA_X` mirrors both onto the opposite axis.
- **Internal cross-check, not by design but a reassuring coincidence of the derivation:** every formula above
  reduces exactly to `StaggeredProjection`'s own, already-verified formula when `hexsidelength = 0` (the
  pitch `(tileDimension + 0) / 2` is staggered's own half-tile step, and the bounds formula's extra term
  likewise collapses to staggered's `(count + 1) * half` shape). This was not assumed going in - it fell out
  of the independently-measured numbers - but it is a strong signal the two derivations agree where they
  should.
- **Cell bounding boxes overlap their neighbour on the staggered axis** whenever `hexsidelength` is less than
  the tile's own dimension on that axis (the normal case for an actual hexagon shape, confirmed directly in
  the rendered probes, the same overlap `StaggeredProjection`'s own doc comment already describes) - so
  `ViewToTileMatrix` is expected to need the same AABB-precision approach, though E2b's own mutation-testing
  finding (a single direct floor was enough, the two-candidate scheme was unnecessary complexity) should be
  re-verified for hexagonal rather than assumed to carry over unchanged.
- Tile *object* coordinates were not measured - the same deferral E1a/E2a made for isometric and staggered:
  no projection consumer needs hexagonal object placement yet.
- Default scroll step is a design choice, not a Tiled convention to verify: `HexagonalProjection` is expected
  to use the same one-full-tile-step convention the other three projections already use.

**E3b: `HexagonalProjection` (DONE).** Implements all six `IMapProjection` methods with the formulas E3a
confirmed. The non-staggered axis's formula and `VisibleTileRange` bound are identical to `StaggeredProjection`'s
own (hexsidelength plays no part there); the staggered axis replaces the fixed half-tile pitch with
`StaggerPitch` ((tileDimension + hexsidelength) / 2), including in `VisibleTileRange`'s own bound on that axis,
which uses the general, unsimplified intersection formula rather than `StaggeredProjection`'s "-1" shortcut -
that shortcut relied on the pitch being exactly half the tile dimension, no longer true once hexsidelength
isn't 0.

- `ViewToTileMatrix` started from the single direct floor `StaggeredProjection` settled on (after its own
  mutation-testing found a two-candidate, nearest-corner scheme was unneeded complexity) rather than
  reintroducing that complexity unverified - E3a's own note said this needed re-checking, not assuming it
  would carry over. Confirmed by mutation testing: forcing the wrong pitch (a plain `tileHeight` instead of
  `StaggerPitch`) fails the round-trip suite, so the simplification is working correctly here too, not merely
  unexercised.
- Test items, in `tests/test_hexagonalprojection.cpp` (26 cases): `MapPixelSize` against the exact measured
  values for both axes across all 3 map sizes, index-independence, and a dedicated case confirming it changes
  with `hexsidelength`; `TileDrawPosition` against the exact measured values for both axes, both indices and
  two `hexsidelength` values, plus the no-map-size-dependency finding, a layer-offset case and a
  resolved-tile-size-independence case; a dedicated cross-check instantiating both `HexagonalProjection`
  (hexsidelength 0) and `StaggeredProjection` side by side and asserting their `MapPixelSize`/`TileDrawPosition`
  agree exactly across a whole grid - not designed in, but a direct consequence of E3a's own
  hexsidelength-0-matches-staggered finding, now a standing regression test against the two ever drifting
  apart; `ViewToTileMatrix` round-trips every cell of three differently-configured 4x3 grids (two axes, two
  hexsidelengths) and refuses an out-of-bounds coordinate; `DefaultScrollStep`; `VisibleTileRange` covers the
  whole map for both axes, is bounded for a small rectangle, clamps for an oversized one, and does not drop a
  cell whose own span starts one pitch before an interior rectangle, on either axis. A renderer-integration
  test loads a real hexagonal map (libtmx's own default axis/index, `SA_Y`/`SI_ODD`, with an explicit
  `hexsidelength` - unlike `stagger_axis`/`stagger_index`, this attribute has no libtmx default) and checks
  `DrawLayer` produces the formula-derived positions end to end.
- `tests/test_maporientation.cpp` updated: hexagonal now loads (a new, dedicated test case). The "refused
  map" tests needed a different real-world case once hexagonal joined orthogonal/isometric/staggered as
  accepted - found while updating them: libtmx's own parser (`tmx_xml.c`) rejects both an unrecognized
  orientation string AND a missing `"orientation"` attribute at parse time (two separate `tmx_err` calls), so
  `TileMapRenderer::LoadMap`'s own orientation switch can never actually see an orientation it does not
  recognize - the default case added to that switch is unreachable in practice, the same finding B3 made for
  `L_NONE`/`OT_NONE`. (An earlier draft of this same comment claimed the opposite - that a missing attribute
  left `O_NONE` to reach the switch - based on reading only the attribute's "value present but invalid"
  branch and missing its `else`; caught by checking the actual runtime error message against the full source
  before trusting the first read, the same discipline this file's own "Doc comments must be verified" section
  asks for.) The "refused map" tests now use a TMX missing its `orientation` attribute entirely, which
  genuinely is refused - by libtmx's own parser, before `LoadMap`'s switch is ever reached - the same path its
  other "cannot open extern tileset" parse-failure cases already exercise.
- Mutation checks, each confirmed by reverting the fix, seeing the specific tests fail, then restoring it:
  removing `hexsidelength` from `StaggerPitch` fails 10 of 25 cases; removing `MapPixelSize`'s extra term
  fails 3 cases, including the `StaggeredProjection` cross-check; flipping the shift-parity condition fails
  10 cases; using a plain tile dimension instead of `StaggerPitch` in `ViewToTileMatrix` fails its round-trip
  cases.
- Full suite (461 cases) passes under AddressSanitizer with no reports. Every sample builds cleanly.

**E3c: hexagonal sample map and real-window check (DONE).** `samples/hexagonal/` (new sample) - at the
owner's request, this reproduces the isometric sample's full Sunny collision/movement demonstration (smooth
cell-to-cell animation, raised obstacle tiles with authored Tile Collision Editor shapes, an edge-triggered
camera clamped to the map's limits, `AddColliderToTileRule` wired as a demonstrated-but-not-acted-on safety
net, a centered sprite) rather than E1c/E2c's plain pan/zoom viewer - the same structure
`samples/isometric/world.cpp` already had, ported to hexagonal geometry. An 8x6 grid, `SA_Y`/`SI_ODD`,
60x50 tiles, `hexsidelength=20` (explicit - E3a/E3b found this attribute has no libtmx default, unlike
`stagger_axis`/`stagger_index`), true hexagon tile art (transparent corners, vertices at the tile's own edges
per E3a's confirmed formula) for both the ground checkerboard and a raised, shaded obstacle cluster.

- **One real adaptation, not a copy-paste**: a hex cell has six neighbors (NW/NE/W/E/SW/SE), not the diamond
  and staggered lattices' four (row±1, col±1 only) - so movement needed a genuine neighbor-offset formula, not
  just a renamed version of the isometric sample's `TryMoveStep`. The offset (which diagonal moves by which
  column, depending on the current row's own parity) was derived and cross-checked directly against
  `HexagonalProjection`'s own confirmed `CellTopLeft` formula - computing which neighboring cell's screen
  position is actually closest - rather than assumed from a remembered hex-grid convention, the same
  "verify, don't assume" standard this project applies to Tiled's own documented rules. Bound to `Q`/`E`/`A`/
  `D`/`Z`/`C` (a common hex-grid keyboard layout: the QWEASDZXC block's own two diagonal rows plus level
  left/right), since WASD/arrows alone cannot name six directions.
- **A second real adaptation**: isometric's tile height (32) happened to equal Sunny's own sprite size, so
  only an X centering offset was needed; a hex tile's bounding box (60x50) differs from her 32x32 sprite on
  BOTH axes, so this sample needed its own `__SUNNY_CENTER_OFFSET_Y` too, not just reused the X-only pattern.
- Checked by eye by the owner: confirmed working on the first try, no iteration needed (unlike the isometric
  sample's own first pass, which went through several rounds of movement/collision/camera fixes before the
  owner confirmed it).
- **Follow-up (DONE, as its own change after E3 merged):** `samples/staggered/` upgraded to the same Sunny
  collision/movement structure, so all three new orientations now demonstrate the same sample consistently.
  Porting it required checking, not assuming, whether isometric's `TryMoveStep(row±1, col±1)` logic would
  carry over: it would not have. In isometric, row/col ARE the diamond's own diagonal screen axes, so a plain
  ±1 step is itself diagonal; staggered stores a plain rectangular array where col is a true screen-horizontal
  axis, so the two same-row neighbors (`col±1`) are actually two diamond-widths apart on screen (confirmed
  numerically against `CellTopLeft` itself: distance 64 vs the real touching neighbors' distance 40), not a
  real touch. Staggered needed the same parity-aware neighbor logic E3c's own hex sample used (NW/NE/SW/SE),
  just four directions instead of hex's six, since a diamond has no true "side" neighbor the way a hexagon's
  flat edges give it one. Confirmed working by the owner in a real window.
  Both the staggered and hexagonal obstacle tiles were then simplified from a raised, shaded 3D block (the
  isometric sample's own obstacle art style) to a plain flat fill matching the ground tiles' own shape, at
  the owner's request - the 3D look wasn't wanted on either of these two orientations.

## Phase F: input configuration

| # | Item | Status |
|---|------|--------|
| F0a | Extract input dispatch/registration into `IControllerManager` + `ControllerManager`, owned by `TileMapRenderer` (mirroring `ICollisionManager`). No behavior change | DONE |
| F0b | Extract the default camera/zoom bindings into an opt-in `DefaultControllerBindings` preset, taking `ITileMap&` + `IControllerManager&` | DONE |
| F0c | Record the Scarab/Caravellius breakage in `doc/BREAKAGES.md` | DONE |
| F1 | Device list and hot-plug events | TODO |
| F2 | Player slots: claim, release, reassign after unplug | TODO |
| F3 | Action map: named actions, rebinding, load and save | TODO |
| F4 | Tunable dead zones and thresholds per pad and axis | TODO |
| F5 | Duplicate pad registration is ignored | TODO |
| F6 | Default handlers are opt-in, and a game can take over input | N/A, subsumed by F0b - once default bindings are a separate, un-auto-installed preset, there is nothing left for this item to do |
| F7a | Rumble interface, factory, null and mock implementations (no real device) | TODO |
| F7b | Real rumble backend, once a platform that vibrates is chosen | TODO |

Today the renderer only polls gamepads a caller has added with `AddGamePad`. It does
not detect devices, react to connect or disconnect, or let a game remap actions.
Dead zones are fixed at 0.1 and triggers at -0.9. Default handlers are installed unless
`bUseDefaultKeyHandler` is false. Caravellius polls its own input through the Lua
functions that Scarab provides.

**Coordination rule for this phase:** any change to a sunlight input signature also
lists the Scarab wrappers and Caravellius call sites it affects, and those are updated in
the same coordination. Phase F stages through its own branch, `feat/complete_f_plan`,
created from `feat/master_plan` - every sub-item merges there first, and only once the
whole phase is finished does `feat/complete_f_plan` itself merge into `feat/master_plan`
(the same staging pattern Phase E used with `feat/complete_e_plan`).

**F0: the controller-manager seam.** `TileMapRenderer` today owns three logically distinct
layers under one roof, confirmed by reading the renderer rather than assumed: raw input
polling (`IInputHandler`/`InputHandlerFactory`, already a clean, swappable backend - nothing
to change here); the event dispatch/registration machinery (the key/gamepad handler lists,
the gamepad list, the analog-stick-to-DPad-button translation with its deadzone constants,
`AddGamePad`/`SetUserKeyEventHandler`/`SetUserGamePadEventHandler`/`GetInputHandler`, and the
per-frame dispatch itself, `HandleUserInput`/`HandleKeyEvent`); and the default camera/zoom
key bindings (`InitalizeDefaultUserInputHandlers` plus the `InputKeyUp/Down/Left/Right/
PageUp/PageDown/Home/End` callbacks, which just call `MoveCameraUp/Down/Left/Right`,
`ZoomIn`/`ZoomOut`, `ResetZoom`, `ResetCamera` - methods `ITileMap` already declares as pure
virtuals, confirmed by reading `tilemap/itilemap.h`, so a preset wired against `ITileMap&`
needs no new interface surface for this part). Every reference to the second layer's own
members (`m_pInputHandler`, `m_GamePadList`, the two handler lists,
`m_pNullInputEventHandler`) was checked across the whole file: they are touched only by this
small cluster of methods, never by sprite/view/collision code - a clean extraction, not a
tangle.

- `F0a` moves the second layer into `SunLight::Input::IControllerManager` (+ concrete
  `ControllerManager`), in its own module (`input/controllermanager.*`, the same shape as
  every other `I*`/concrete/factory split in this codebase):

  ```cpp
  enum InputDispatchMode {
      INPUT_DISPATCH_MODE_ACTIVE,    // continuous IsKeyDown/IsGamepadButtonDown polling, fires every frame held
      INPUT_DISPATCH_MODE_REACTIVE   // fires once per key-press edge (GetKeyPressed)
  };

  class IControllerManager {
      public:
      virtual ~IControllerManager( void ) = default;
      virtual void AddGamePad( int nGamePadId ) = 0;
      virtual void SetUserKeyEventHandler( KeyboardKey evt, INPUT_EVENT_HANDLER handler ) = 0;
      virtual void SetUserGamePadEventHandler( GamepadButton evt, INPUT_EVENT_HANDLER handler ) = 0;
      virtual IInputHandler& GetInputHandler( void ) = 0;
      virtual void SetInputDispatchMode( InputDispatchMode mode ) = 0;
      virtual InputDispatchMode GetInputDispatchMode( void ) const = 0;   // new: no equivalent getter exists today
      virtual void Update( void ) = 0;   // per-frame dispatch, renamed from HandleUserInput
  };
  ```

  `InputDispatchMode` moves from `SunLight::Renderer` (where it was named `ViewControlMode` -
  a camera-flavored name for what has only ever been an input dispatch strategy, not a camera
  concern) into `SunLight::Input`, alongside the class that actually owns dispatching.
  `ControllerManager` is directly constructible, no factory - the same shape
  `CollisionManager` already uses (no backend variation to swap; the only real swap point,
  `IInputHandler`, already has its own factory and test-override hook,
  `InputHandlerFactory::SetCreator`, confirmed sufficient for a `MockInputHandler` without
  needing a second factory layer here). `TileMapRenderer` owns one internally
  (`std::unique_ptr<IControllerManager>`), exposes it via `GetControllerManager()` (mirroring
  `GetCollisionManager()` exactly), and its own frame loop calls
  `m_pControllerManager -> Update()` where it previously called `HandleUserInput()` directly -
  same shape as it already calls `m_CollisionManager.Update()`.
  - Test items: a new `tests/test_controllermanager.cpp`, using a new `tests/mock_input.h`
    (`MockInputHandler`, swapped in via `InputHandlerFactory::SetCreator`, the same pattern
    `mock_engine.h`/`mock_window.h` already use) - every public method's behavior matches
    today's `TileMapRenderer` dispatch exactly: `ACTIVE` mode fires continuously while a key/
    button is held, `REACTIVE` mode fires once per press; gamepad analog sticks translate to
    DPad-equivalent dispatch at the documented deadzones (now named constants, not the inline
    literals `HandleUserInput` has today); a duplicate `AddGamePad` call is confirmed to
    double-fire, matching today's actual behavior exactly (checked against the source: no
    dedup exists) - a pinning test for the bug F5 later fixes, not a claim it is already fixed
    here, since F0a is a pure extraction with no behavior change; the
    gamepad-overrides-keyboard-when-handled precedence holds.
  - This item directly unblocks T3 ("direct tests for `Run()`'s input... dispatch"), stuck
    TODO until now because the dispatch logic had no seam to mock.

**F0a (DONE).** `IControllerManager`/`ControllerManager` (`src/input/icontrollermanager.h`,
`src/input/controllermanager.{h,cpp}`) implemented and wired in exactly as planned above.
Every one of `TileMapRenderer`'s old public input methods (`AddGamePad`,
`SetUserKeyEventHandler`, `SetUserGamePadEventHandler`, `GetInputHandler`,
`SetViewControlMode`) kept its exact signature, now a one-line delegation to the internally
owned `m_pControllerManager`; `m_ViewControlMode` as a stored member was removed entirely
(now redundant - the manager stores its own dispatch mode), with a small, explicit
`ToInputDispatchMode` conversion (not a `static_cast` relying on the two enums' ordinal
values happening to agree) bridging the renderer's own `ViewControlMode` to the new
`InputDispatchMode` at the two call sites (construction, `SetViewControlMode`). No sample or
existing test needed a single change - confirmed by building all of them unmodified.
- Test items, in `tests/test_controllermanager.cpp` (10 cases) using a new
  `tests/mock_input.h` (`MockInputHandler`, swapped in via
  `InputHandlerFactory::SetCreator`, the same pattern `mock_sound.h`'s `MockSoundFixture`
  already uses for a factory that returns a fresh instance per call): `ACTIVE` mode fires
  every frame a key is held and stops firing once released; `REACTIVE` mode dispatches to
  whichever handler matches `GetKeyPressed()`'s current value; `KEY_NULL`'s fallback handler
  fires only when nothing else handled that frame; a gamepad analog stick beyond/within the
  deadzone does/doesn't dispatch; a duplicate `AddGamePad` call is confirmed to double-fire,
  pinning today's real behavior (checked against the source: no dedup exists) rather than
  assuming F5's eventual fix is already in place; gamepad activity suppresses keyboard
  dispatch that frame even with no gamepad handler registered for the specific
  stick/button involved (confirmed against the source: `bEventHandled` is set directly from
  axis movement detection, before the handler-matching loop runs); `GetInputHandler` returns
  the same instance the manager was constructed with; `SetInputDispatchMode`/
  `GetInputDispatchMode` round-trip.
- Mutation check: widening the left-stick X deadzone from 0.1 to 0.9 fails 2 of the 10 cases
  (the deadzone case itself, and the gamepad-suppresses-keyboard case, which also depends on
  axis movement being detected) - confirmed by reverting the fix, seeing the tests fail, then
  restoring it.
- Full suite (471 cases) passes under AddressSanitizer with no reports. Every sample and the
  full existing suite builds and passes unmodified - the strongest form of "no behavior
  change" this item could demonstrate.

- `F0b` moves the third layer into `SunLight::Input::DefaultControllerBindings`, a stateless
  preset (a static method, no instance - nothing here varies by backend or needs mocking):

  ```cpp
  class DefaultControllerBindings {
      public:
      // Wires WASD/arrow keys to camera moves, PageUp/PageDown to zoom, Home/End to
      // reset-zoom/reset-camera, on both keyboard and every gamepad already added to
      // controllerManager - exactly today's InitalizeDefaultUserInputHandlers behavior,
      // just external and opt-in now.
      static void Install( SunLight :: TileMap :: ITileMap &map, IControllerManager &controllerManager );
  };
  ```

  `TileMapRenderer` no longer auto-installs anything; `RendererConfig::bUseDefaultKeyHandler`
  is removed, and the classic `TileMapRenderer(width, height, title, targetFps)` constructor
  drops its fifth parameter. A game that wants the old defaults calls
  `DefaultControllerBindings::Install( *pRenderer, pRenderer -> GetControllerManager() )`
  once, explicitly, itself.
  - Test items: `Install` wires exactly the documented keys/buttons - driven through a mock
    `IControllerManager`/`IInputHandler` (or the real `ControllerManager` against the null
    backend, same style `test_camera.cpp` already uses), each key/button fires the matching
    `ITileMap` camera/zoom call exactly once.

**F0b (DONE).** `DefaultControllerBindings` (`src/input/defaultcontrollerbindings.{h,cpp}`)
implemented exactly as planned, wiring the same 16 bindings (8 keyboard, 8 gamepad)
`InitalizeDefaultUserInputHandlers` used to install. Removing the auto-install meant checking,
not assuming, how many call sites would break: all 8 samples' own `world.cpp` files pass
`bUseDefaultKeyHandler=false` already (confirmed by grep before removing anything), so none of
them relied on the removed mechanism - but two more direct constructor calls outside any
`world.cpp` (`samples/multiview/multiview_test.cpp`,
`samples/multiview_overlap/multiview_overlap_test.cpp`) and 41 more inside the test suite
(`tests/test_rendererconfig.cpp` x6, `tests/test_nullbackend.cpp` x1,
`tests/test_tilemaprenderer.cpp` x18, `tests/test_tilemaprenderer_lifecycle.cpp` x16 - the exact
counts confirmed from the diff itself, not estimated) still passed the now-removed fifth
constructor argument positionally, found only once the build actually failed on each in turn -
a reminder that grepping for a named parameter (`bUseDefaultKeyHandler`) misses every call site
using positional arguments instead. None of these were testing the removed parameter's own
effect; all 43 just needed the trailing argument dropped. Separately,
`samples/tilemaprenderer/world.cpp` was checked and confirmed to use a deliberately *inverted*
camera mapping (`KEY_UP` to `MoveCameraDown`, `KEY_PAGE_UP` to `ZoomOut` - the documented
sprite-relative convention), not the literal default mapping, so it does not call
`DefaultControllerBindings::Install` - no sample does, since none actually wants the literal
default bindings.
- Test items, in `tests/test_defaultcontrollerbindings.cpp` (3 cases), using the existing
  `MockTileMap` (extended with call counters on its 8 camera/zoom methods, additive - no
  existing test depends on their old no-op bodies) and `mock_input.h`: arrow keys dispatch the
  matching `MoveCamera*` call; PageUp/PageDown/Home/End dispatch `ZoomIn`/`ZoomOut`/
  `ResetZoom`/`ResetCamera`; a gamepad's DPad and right-face buttons dispatch the same calls.
- Mutation check: swapping `KEY_UP`'s binding from `MoveCameraUp` to `MoveCameraDown` fails the
  arrow-key test (both assertions on that pair), confirmed by reverting the fix, seeing it
  fail, then restoring it.
- Full suite (474 cases) passes under AddressSanitizer with no reports. Every sample builds
  cleanly.

- `F0c`'s scope narrowed once F0a kept every old `TileMapRenderer` input method as a permanent
  delegating wrapper (not removed, never needed removing) and F0b found no sample wanted
  `DefaultControllerBindings::Install` anyway: nothing further needs updating in this repo. The
  remaining, genuinely mechanical step is recording the Scarab/Caravellius breakage (the
  classic constructor's dropped fifth parameter, `RendererConfig::bUseDefaultKeyHandler`'s
  removal, and `ViewControlMode`'s rename/relocation to `SunLight::Input::InputDispatchMode`)
  in `doc/BREAKAGES.md`, per this project's standing peer-impact convention.
  - Test items: none beyond what F0a/F0b already proved - this item is pure documentation.

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
