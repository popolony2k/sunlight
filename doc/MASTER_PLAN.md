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
| E3 | Hexagonal rendering, as its own `IMapProjection` implementation once E0 is DONE | TODO |

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
