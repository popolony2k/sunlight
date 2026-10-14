# Changelog

All notable changes to this project are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

Versions are `0.MINOR.PATCH`: a patch release only fixes bugs; a minor release adds features
and, while the version is below 1.0, may break compatibility - every such change is marked
**Breaking**. Each version is a git tag with a [GitHub release](https://github.com/popolony2k/sunlight/releases)
(source plus Linux, Windows and macOS archives). Releases 0.2.0 to 0.21.0 are described only in
their GitHub release notes. History before 0.1.0 (the 2021-2023 prototype) is in the git log.

## [Unreleased]

The master plan ([`doc/MASTER_PLAN.md`](doc/MASTER_PLAN.md)), developed on `feat/master_plan`.
Each item is described in full in the plan; what each change means for code built on sunlight
(Scarab, Caravellius) is listed in [`doc/BREAKAGES.md`](doc/BREAKAGES.md).

### Breaking

- `stCoordinate2D`, `stSize2D` and `stDimension2D` moved from `SunLight::TileMap` to
  `SunLight::Base` (`base/primitives.h`), with no aliases.
- `IEngine` gains pure virtuals `LoadFont`, `BeginClip`/`EndClip`, `DrawLine`,
  `DrawEllipseOutline`, `DrawTextureRotated` and `DrawTextRotated`, and loses `SetPixel`;
  `IFont` gains `DrawTextRotated`. Backend implementers must update.
- `RendererConfig::bUseDefaultKeyHandler` is removed and the classic constructor loses its fifth
  parameter: `TileMapRenderer(width, height, title, targetFps)`. The camera/zoom key bindings are
  no longer installed by default - call `DefaultControllerBindings::Install(map, controllerManager)`.
- `SetWindowBackgroundColor` takes a `SunLight::Base::stColor` instead of a packed `uint32_t`.
- `KEY_MENU` is 5 (was 82), matching raylib; key codes are translated by name.
- `BaseCommand` has a virtual destructor, and the command enum moved to `scripting/command.h`.
- The per-stick dead-zone constants are replaced by one `DEFAULT_AXIS_DEADZONE` (same value),
  configurable per pad and axis.
- `stLayer::nOpacity` (`GetLayer`/`SetLayer`) is 0..255, 255 = opaque. The old conversions were
  not inverses: an opaque layer read 256, and `SetLayer(255)` hid the layer.

### Added

- **Isometric, staggered and hexagonal maps** load and render, each behind an `IMapProjection`
  (orthogonal maps too), with visible-tile culling.
- **Objects:** text objects, drawn with fonts registered through `TileMapRenderer::RegisterFont`
  (new `IFont` module); tile objects (own size or stretched, animated; their flip bits are not
  drawn yet); point objects. Polylines, polygons, rectangles and ellipses take a `line_width`,
  points a `point_size` (map units, scaled by zoom). Rectangles, polylines, polygons, ellipses,
  tile and text objects honour their Tiled rotation.
- **Layers:** Tiled per-layer parallax (`parallaxx`/`parallaxy`, the map's parallax origin) for
  tile, object and image layers and world-space sprites, plus `SetLayerParallax`; an object's own
  `opacity` property overrides its layer's; `Sprite::SetZOrder` orders sprites within a layer.
- **Input:** `IControllerManager` (`GetControllerManager()`), connected-gamepad list and hot-plug
  events, `PlayerSlotManager` (player slots to pads), `ActionMap` (named, rebindable actions with
  save/load), dead zones per pad and axis, and gamepad rumble (`IGamepadHaptics`, backed by SDL2 -
  a new static dependency; call `Update()` once per frame so a timed rumble stops).
- **A drawing surface without a map:** `IShapeSurface` (`DrawRectangle`, `DrawEllipse`,
  `DrawPolyline`, `DrawPolygon`, `DrawPoint`, zoom-aware), and sprites registered with no map
  loaded are drawn.
- Samples: `isometric`, `isometric_haptics`, `staggered`, `hexagonal`, `parallax`, `text`,
  `objects`, `shapes`, `multiview_overlap`, `opacity`, `zorder`, `surface` and `input`, each with a
  `docs/README.md`; shared fonts in `samples/shared/fonts`.
- CI: an AddressSanitizer job on Linux (with leak detection), a Doxygen job that fails on any
  warning (Doxygen pinned to 1.17.0, also for the published docs), and suffixed tags
  (`v0.39.0-beta.1`) published as prereleases.

### Changed

- Shapes are drawn by the backend inside an engine clip (the software clip is gone), so their
  pixels differ slightly from the old per-pixel routines. A polyline's width now scales with
  the zoom (2 pixels at zoom 2).
- Objects that used to be invisible are drawn: text, tile and point objects. Rotated objects are
  drawn rotated. Shape and text objects now respect their layer's opacity.
- Tiles, image layers and sprites are snapped to whole pixels (`Helper::RoundHalfUp`), removing a
  seam between tiles at non-integer zoom and a 1-pixel gap at some camera positions.
- `AddGamePad` ignores a pad that was already added (it used to dispatch it twice).
- `LoadMap` over a loaded map frees the old map first; a map that fails to load leaves the loaded
  one in place. Registered sprites stay registered, as before.
- A listener or input handler added during a dispatch starts with the next frame; a map listener
  removed during one is not called again.
- `samples/tilemaprenderer` is renamed `samples/orthogonal` (`orthogonal_test`).

### Fixed

- A tile lookup past the map's far edge read beyond the tile array (the scroll-past-boundary
  FIXME).
- `ScriptProcessor` deleted derived commands through a non-virtual `BaseCommand*`.
- `FileSystem::Mount` crashed when it was the first call (it now sets PhysFS up itself).
- `MoveCameraDown`/`Right` could not return the camera to its origin after one step up/left, and
  `MoveCameraLeft`/`Up` let the camera scroll past the map's edge (the limit ignored the
  viewport's position).
- The left stick's Y axis used the right stick's dead zone.
- Sprites registered with no map loaded leaked their per-layer lists at `Stop()`.
- `LoadMap` over a loaded map leaked the old map, its textures and Resource Manager.
- Map listeners, collision listeners and input handlers were walked while the callback could
  change the list (undefined behaviour: a crash, a skipped or a doubled call).
- Every Doxygen warning.
- A TTF/OTF font was loaded with the 95 ASCII glyphs alone (raylib's `LoadFont`), so every accented
  letter in it was drawn as `?`. It is now loaded with ASCII, Latin-1 and Latin Extended-A; a
  bitmap (`.fnt`) font is unchanged.
- The `objects`, `text`, `opacity` and `shapes` samples could not scroll: they relied on the camera
  keys the renderer no longer installs. They now call `DefaultControllerBindings::Install`.

### Removed

- `IEngine::SetPixel`, `SUNLIGHT_SOFTWARE_CLIP` and `base/clipmode.h`.
- `RendererConfig::bUseDefaultKeyHandler`.
- `doc/TODO.txt`, `doc/FIXME.txt` and `doc/MISSING_FEATURES.md` (all in `doc/MASTER_PLAN.md`).

## [0.38.0] - 2026-09-21

### Added

- Fullscreen at window creation: `RendererConfig::bFullscreen` and `fullscreenStrategy`, handed to
  `IWindow::Create` (two new defaulted parameters) and applied at every `Start()`; an unknown
  strategy is rejected. New `IWindow::GetFullscreenStrategy()` and
  `IDrawSurface::GetFullscreenStrategy()`. The null backend accepts and ignores both.

### Fixed

- `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR` stepped one frame past the last one every cycle; it
  now shows each frame once (period N).
- `TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT` showed its last frame twice and frame 0 three
  times (period 2N + 1); it is now a ping-pong that shows each end once (period 2N - 2), so it runs
  faster at the same delay. The other modes are unchanged.

## [0.37.0] - 2026-09-21

### Added

- `Sprite::GetTextureSequenceSize`, `SetTextureSequenceDelay` and `ClearTextureSequence` change a
  sequence after it was built (also `TextureMap::GetTextureCount`/`SetDelay`). A delay of -1 means
  a held frame.

### Fixed

- `TextureMap` kept a `std::deque` iterator as its cursor across appends: a use-after-free once a
  sequence grew past a block of entries. The cursor is an index now.

## [0.36.0] - 2026-09-21

### Added

- World-space sprites: `Sprite::SetWorldSpace(true)` makes a sprite's position a map position,
  drawn by each view with its own camera, exactly where that view draws the map (off by default).
  Also `BaseCanvas::GetCameraOffset` and `IsOnScreen`. Colliders are not affected.

### Fixed

- In the multi-view frame, a sprite advances in the first pass where it is on screen (it used to
  advance in the first pass that reached it, even off screen).

## [0.35.0] - 2026-09-21

### Added

- `samples/multiview`: one map and one world-space character in three views (main, minimap,
  close-up).

### Changed

- **Breaking:** view handles are `std::shared_ptr<IView>` (`CreateView` returns one, `GetView`
  returns one or empty), and `RemoveView(handle)` is added. A handle held after its view or
  renderer is gone is inert (`IView::IsRemoved()`) instead of dangling. Migration:
  `auto view = r.CreateView(rect);` and `view->GetId()` for the id.

### Fixed

- A sprite and its renderer can be destroyed in either order: each unregisters from the other
  (`BaseCanvas::ChildRemoved`, `Sprite::SetParent`). A sprite added to a second renderer is
  forgotten by the first.
- `~Sprite` read canvases the caller may already have destroyed; it no longer touches them.
  `Sprite::Unload()` still unloads them.

## [0.34.0] - 2026-09-21

### Added

- Extra views are drawn: one pass per visible view, in draw order. New on `IView`:
  `SetVisible`, `SetDrawOrder`, background control (`SetClearBackground`, `SetBackgroundColor`,
  `UseMapBackgroundColor`), a per-view layer mask (`ShowLayer`, `ShowOnlyLayers`,
  `ShowAllLayers`, `IsLayerShown`) and `FitToMap()`. Also `Viewport::GetZoomLimits` and
  `TileMapRenderer::GetClearBackground`. Each sprite advances once per frame.

## [0.33.1] - 2026-09-21

### Fixed

- `MoveCameraUp`/`MoveCameraLeft` crashed when no map was loaded (renderer and views).

## [0.33.0] - 2026-09-21

### Added

- `IView::GetScrollStepSize` and `TileMapRenderer::GetScrollStepSize`.

## [0.32.0] - 2026-09-21

### Added

- Views: every renderer has a default view (`GetDefaultView()`) and can create more
  (`CreateView`, `GetView`, `RemoveView`, `GetViewCount`). A view holds a viewport, camera and
  scroll step, and runs the renderer's own camera code. New pure virtuals on `ITileMap`. Extra
  views are not drawn yet.

## [0.31.0] - 2026-09-21

### Fixed

- A map's external tilesets (`.tsx`) and templates (`.tx`) are read through `SunLight::FileSystem`,
  so they load from a mounted archive and pass through the read filter.
- The samples failed to load their map when given an absolute base path.

## [0.30.0] - 2026-09-21

### Added

- `TextureCanvas`/`Sprite` frame step split into `Advance()` (animation state, once per frame) and
  `Draw()` (may run several times). `Update()` is both, unchanged.

## [0.29.0] - 2026-09-21

### Changed

- **Breaking:** a viewport's `size` is a width/height everywhere: the viewport is
  `[pos, pos + size)`. Clipping used to read it as the far edge when `pos` was not `(0, 0)`.
  Migrate such a viewport by subtracting `pos` from `size`.

### Fixed

- `TileMapToTileMatrix` used the viewport's `pos.x` for the row.

## [0.28.0] - 2026-09-21

### Added

- `Viewport::GetEnableUserZoom()`.

### Fixed

- `MoveCameraUp` bounded vertical scrolling by the viewport's width.
- `Viewport::SetMinZoom`/`SetMaxZoom` could only narrow the limits, and the maximum was exclusive.
  Both bounds are now inclusive positions, validated against the whole scale.

## [0.27.0] - 2026-09-21

### Added

- Public zoom-scale constants in `base/viewport.h` (`ZOOM_STEP`, `ZOOM_POS_MIN`/`MAX`/`COUNT`/
  `DEFAULT`, `ZOOM_FACTOR_MIN`/`MAX`).

### Fixed

- `Viewport::GetZoomFactor(nZoomPos)` ignored its argument.

## [0.26.1] - 2026-09-21

### Fixed

- The null backend's virtual time drifted (300 frames at 60 FPS read 4.999999999999988 s). Frames
  are counted, so whole seconds are exact.

## [0.26.0] - 2026-09-21

### Added

- Null (headless) backend, `RENDERER_BACKEND_NULL`: no display or GPU, virtual time, real image
  sizes from file headers. `RendererConfig` gains `framePacing` and `nMaxFrames`; also
  `General::VirtualClock`.

### Changed

- A renderer created without a viewport gets one covering the render area (`LoadMap` used to
  crash dividing by zero).

## [0.25.0] - 2026-09-21

### Added

- `General::Clock`: a process-wide, replaceable source of "now", read by script waits, sprite
  frame timing and tile animation.

### Removed

- **Breaking:** the `IEngine::FullscreenStrategy` aliases; use `SunLight::Window::FullscreenStrategy`.

## [0.24.0] - 2026-09-20

### Added

- `RendererConfig`, one value struct for creating a renderer, with `Validate()`, and the checked
  `TileMapRenderer::Create(config, &error)`.

### Changed

- **Breaking (backend implementers):** `IEngine::OnWindowClosing` is replaced by
  `IWindow::AddCloseHandler`/`RemoveCloseHandler`.
- `FullscreenStrategy` moved to `SunLight::Window` (the `IEngine` names kept as aliases).
- `Start()` applies the configured exit key instead of resetting it to ESC.

## [0.23.0] - 2026-09-20

### Added

- `MockWindow` and lifecycle tests: `Start()`/`Run()`/`Stop()` are unit-tested end to end.

### Changed

- **Breaking (backend implementers):** the window operations moved from `IEngine` to a new
  `IWindow` (`src/window/`); `IEngine` gained `ClearBackground` and `DrawFPS`. `TileMapRenderer`
  no longer calls raylib directly.

## [0.22.0] - 2026-09-19

### Added

- `GetElapsedTime()`: wall-clock seconds, on `IEngine`, `IDrawSurface` and `TileMapRenderer`.

## [0.2.0] - [0.21.0]

Described in their [GitHub release notes](https://github.com/popolony2k/sunlight/releases).

## [0.1.0] - 2026-07-22

### Added

- `IEngine` backend abstraction (`src/engines/`), with `GetApplicationDirectory()`.
- `TileMapRenderer::SetWindowTitle()`; right-stick handling for `GAMEPAD_BUTTON_RIGHT_FACE_*`.
- Samples `collision` and `gamepad`, and real docs for `sprite` and `tilemaprenderer`.
- GitHub Actions CI on Linux, macOS and Windows.
- A doctest unit test suite, with mock engine, sound and tile map seams.
- `cmake --install` produces a usable `sunlightConfig.cmake` package.

### Changed

- `libxml2` and `tmx` pinned to release tags.
- Owning raw pointers converted to `std::unique_ptr`.

### Fixed

- The `install()` rule, and the package config's paths when consumed through `FetchContent`.
- `TextureCanvas::Load()` checked the height twice instead of width and height.
- `ScriptProcessor`: `LOOP_CMD` ignored its repeat count, and a missing label crashed
  `GOTO_LABEL_CMD`/`END_LOOP_CMD` (now reported through `OnError()`).
- The `KeyboardKey` name clash in `TileMapRenderer::SetExitKey`.

[Unreleased]: https://github.com/popolony2k/sunlight/compare/v0.38.0...HEAD
[0.38.0]: https://github.com/popolony2k/sunlight/compare/v0.37.0...v0.38.0
[0.37.0]: https://github.com/popolony2k/sunlight/compare/v0.36.0...v0.37.0
[0.36.0]: https://github.com/popolony2k/sunlight/compare/v0.35.0...v0.36.0
[0.35.0]: https://github.com/popolony2k/sunlight/compare/v0.34.0...v0.35.0
[0.34.0]: https://github.com/popolony2k/sunlight/compare/v0.33.1...v0.34.0
[0.33.1]: https://github.com/popolony2k/sunlight/compare/v0.33.0...v0.33.1
[0.33.0]: https://github.com/popolony2k/sunlight/compare/v0.32.0...v0.33.0
[0.32.0]: https://github.com/popolony2k/sunlight/compare/v0.31.0...v0.32.0
[0.31.0]: https://github.com/popolony2k/sunlight/compare/v0.30.0...v0.31.0
[0.30.0]: https://github.com/popolony2k/sunlight/compare/v0.29.0...v0.30.0
[0.29.0]: https://github.com/popolony2k/sunlight/compare/v0.28.0...v0.29.0
[0.28.0]: https://github.com/popolony2k/sunlight/compare/v0.27.0...v0.28.0
[0.27.0]: https://github.com/popolony2k/sunlight/compare/v0.26.1...v0.27.0
[0.26.1]: https://github.com/popolony2k/sunlight/compare/v0.26.0...v0.26.1
[0.26.0]: https://github.com/popolony2k/sunlight/compare/v0.25.0...v0.26.0
[0.25.0]: https://github.com/popolony2k/sunlight/compare/v0.24.0...v0.25.0
[0.24.0]: https://github.com/popolony2k/sunlight/compare/v0.23.0...v0.24.0
[0.23.0]: https://github.com/popolony2k/sunlight/compare/v0.22.0...v0.23.0
[0.22.0]: https://github.com/popolony2k/sunlight/compare/v0.21.0...v0.22.0
[0.2.0]: https://github.com/popolony2k/sunlight/releases/tag/v0.2.0
[0.21.0]: https://github.com/popolony2k/sunlight/releases/tag/v0.21.0
[0.1.0]: https://github.com/popolony2k/sunlight/releases/tag/v0.1.0
