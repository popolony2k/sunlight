# Breakages for the master plan

What the master-plan work changes for the peers that use sunlight: Scarab (the Lua engine on top of
sunlight) and Caravellius (the game built on Scarab). Nothing here has been sent to either session
yet. Peers are contacted when the master plan is released, as agreed with the owner.

This file lives on `feat/master_plan` only. It is removed when the plan is merged to `main`, after
the peers have been told what changed.

**Status:** `merged` = on `feat/master_plan`, unreleased. `pending` = on the working branch, not yet
merged.

## Merged to feat/master_plan

| # | Change | Who it affects | What to do at integration |
|---|--------|----------------|---------------------------|
| 1 | **A2** `LoadMap` refuses isometric, staggered and hexagonal maps with a clear error. | Any map that is not orthogonal. All three maps in Caravellius and Scarab are orthogonal, checked. | Nothing today. Tell the game authors that a non-orthogonal map now fails to load. |
| 2 | **A3** `KEY_MENU` changes value from 82 to 5, to match raylib. Key translation is now by name. | Scarab exposes `KEY_MENU` to Lua by value (`src/lua/luainputapi.cpp`). Scripts that use the name are safe. Saved bindings or hard-coded 82 break. Caravellius does not use `KEY_MENU`. | Check Scarab's saved input settings for the number 82. |
| 3 | **A5** `doc/TODO.txt`, `doc/FIXME.txt` and `doc/MISSING_FEATURES.md` are removed. | Nothing. No link to them was found in Scarab or Caravellius. | Nothing. |
| 4 | **A6** The collision lookup no longer returns a tile for a position past the far edge of the map. It used to read past the tile array and return a wrong tile. | Any game with a sprite in the last strip of a map, at any camera position. Caravellius has not hit it so far (the maintainer's observation, not a test). | Re-run Caravellius's gameplay fingerprint at the pin bump, as its own notes require. |
| 5 | **A7** `BaseCommand` gets a virtual destructor, so the command classes have a different layout. A new header `scripting/command.h` holds the command enum. | Scarab must rebuild against the new headers. Anything that includes the command types directly needs the new path. No Lua-visible change. | Rebuild Scarab. Update any direct include of the command types. |
| 6 | **B1** `IEngine` gains `LoadFont`, `BeginClip` and `EndClip`. `DrawText` and `MeasureText` are unchanged. | Only implementers of `IEngine`: the raylib and null backends and the test mocks. `IDrawSurface` and Scarab's Lua text API are unchanged. | Nothing for Scarab's scripts. A new `IEngine` backend would implement the three methods. |
| 7 | **B1** `TileMapRenderer::RegisterFont(family, path, bold, italic)` is a new public method. Text objects draw with the registered font, `??????` when none is registered. | Nothing existing. New capability for games with text objects. | Scarab could expose it to Lua later, as a new function. |
| 8 | **B1** Text objects are drawn; before this they were invisible. | Any map with text objects now shows them. | Tell the map authors. Check that text objects are not used as hidden data. |
| 9 | **B1** `IFont` is a new interface in its own module, namespace `SunLight::Font`, file `src/font/ifont.h`. | Only code that uses `IFont` directly. | Nothing for Scarab or Caravellius. |
| 10 | **B1** `SetWindowBackgroundColor` takes `SunLight::Base::stColor` instead of a packed `uint32_t`. The window area takes that colour only once the call is made, so existing games are unchanged. | Nothing calls the setter yet. | Nothing today. Scarab would call it with a `stColor` if it exposes the setter. |
| 11 | **B2** Tile objects (`OT_TILE`) are drawn: at their own size, or stretched into their box, clipped to the viewport, and animated. Before this they were invisible. | Any map with tile objects now shows them. | Tell the map authors, as for text objects. |
| 12 | **PhysFS** `FileSystem::Mount` sets PhysFS up itself, so it can be the first call. Before, a mount before any `Init` or read crashed. | Code that mounts before `Init` (it crashed before). Scarab calls `Init(argv[0])` first, so it is unchanged. | Nothing. |
| 13 | **Fonts** The sample fonts now live in `samples/shared/fonts`. | Samples only. No API change. | Nothing. |
| 14 | **G1 Namespace** `stCoordinate2D`, `stSize2D` and `stDimension2D` move from `SunLight::TileMap` to `SunLight::Base` (file `src/base/primitives.h`). No aliases are kept. | Scarab has 12 references to them (`src/main.cpp`, `src/lua/luaspriteapi.cpp`, `src/lua/luatilemapapi.cpp`, `src/lua/luarendererapi.cpp`, `src/lua/luaviewapi.cpp`, `src/lua/luacameraapi.cpp`). Scarab will not compile until they are renamed. | Rename `SunLight::TileMap::` to `SunLight::Base::` for these three types in Scarab. Do this when integration starts. |

## Pending (on the working branch, not merged)

| # | Change | Who it affects | What to do at integration |
|---|--------|----------------|---------------------------|
| 15 | **Engine clip** `SUNLIGHT_SOFTWARE_CLIP` and `src/base/clipmode.h` are removed: the engine clip is the only mode. `IEngine::SetPixel` is removed. `IEngine` gains `DrawLine` and `DrawEllipseOutline`. Shapes are drawn by the backend, so their pixels differ slightly from the old per-pixel routines (compared on the shapes sample only). Tile and text objects are drawn whole and cut by the clip; the tests cover that, not a pixel comparison. | Scarab and Caravellius call neither `SetPixel` nor the removed renderer methods (checked in their code; only their docs mention it). Any other `IEngine` implementer must drop `SetPixel` and implement `DrawLine` and `DrawEllipseOutline`. | Nothing for scripts. Check any game that draws shapes at the pin bump: shape edges may differ by a pixel. |
| 16 | **C1** A polyline's edges are scaled by the zoom: its width is the `line_width` property (default 1 map unit) times the zoom, rounded half-up, at least 1 pixel. So a polyline is 2 pixels wide at zoom 2 with no property set, where it was 1 before. A `line_width` above 1 draws the line as spans, and a polygon takes the same property (C2): its corners get round joins. A rectangle takes the same property (C3): its corners get round joins. An ellipse takes it too (C4): a wider outline is a ring. | No peer project references a polyline (checked in their code and maps). Any game that draws polylines at a zoom other than 1 will see them wider. | Nothing for scripts. Check any polyline in a game's maps at its own zoom before the pin bump. |

| 17 | **D1a** A rectangle, polyline or polygon with a rotation in Tiled is drawn turned about its stored point. Before, the rotation was ignored. | Any map with a rotated rectangle, polyline or polygon now shows it turned. Objects with no rotation are unchanged. | Nothing for scripts. Check rotated shapes in a game's maps at the pin bump. |

| 18 | **D1d** `IEngine` gains `DrawTextureRotated` and `DrawTextRotated`, and `IFont` gains `DrawTextRotated` (all pure virtual). A tile or text object with a rotation in Tiled is drawn turned. | Any other implementer of `IEngine` or `IFont` (the raylib and null backends and the test mocks are updated) must implement the new methods. Maps with rotated tile or text objects now show them turned. | Nothing for scripts. A new backend implements the three methods. Check rotated tile and text objects in a game's maps at the pin bump. |

## Checked: no change for the peers

- `IDrawSurface` and the Lua text API (`set_font`, `draw_text`, `measure_text`).
- Scarab's startup order (`Init` before mounts).
- Caravellius's own Lua font wrapper (`Text`), since `set_font` keeps its meaning.
- Scarab and Caravellius code calls no `SetPixel` (their docs mention the old viewport behaviour only).
