# worlds sample

Two **worlds** - two different maps, each with its own Sunny - on screen at once, using `sunlight`'s worlds (`SunLight::TileMap::IWorld`, `ITileMap::CreateWorld`, `IView::SetWorld`):

- the **left half** is the renderer's default view: it always shows the **default world**, the cloud map, loaded with the renderer's own `LoadMap`;
- the **right half** is an extra view showing **world B**, the isometric map, made with `CreateWorld` and filled with `IWorld::LoadMap` / `IWorld::AddSprite`;
- an **overlay** over the left half also shows world B, at opacity 0, so the left half can cross-fade from one world to the other (`IView::SetOpacity`);
- a **grid world**: four maps placed side by side in one world with `IWorld::AddMap` - the cloud map three times and the isometric map in the bottom-right corner, orientations mixed - shown on the right with key `1`.

## How it works

A world holds a map, the sprites registered on its layers and its collision rules. Worlds are isolated: each has its own coordinates (its map's pixels), and a sprite in one is never drawn in, nor collides with anything in, another. Each Sunny is a world-space sprite of its own world, so WASD and the arrows move two characters that never meet.

A view shows one world. The default view always shows the default world, so the renderer's own single-map calls (`LoadMap`, `AddSprite`, `GetTile`, ...) and its camera calls always agree on what they act on. Extra views show any world: `SetWorld`, then `FitToMap` or `SetCameraPosition` to place the camera.

A world can hold several maps at offsets (`IWorld::AddMap( path, x, y )`), as a Tiled `.world` places them: each map's pixel box at its offset, whatever its orientation. Their layers are matched by **name** - one world layer id per name (`IWorld::GetLayerId`) - and drawn layer by layer across every map, a sprite once after its layer of every map, so the grid world's Sunny walks over the seams. The camera's limits and `FitToMap` use the box every map fits in.

`RemoveWorld` sends every view showing that world back to the default world, frees its map, and **detaches** its sprites: they are no longer drawn or colliding, but they are still the game's, neither destroyed nor emptied. Every live world updates every frame, shown or not.

## Building

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

## Running

Like the other samples, `worlds_test` resolves its resources (copies of the [multiview](/samples/multiview/docs/README.md) sample's cloud map and the [isometric](/samples/isometric/docs/README.md) sample's map, and Sunny) relative to a base path passed as `argv[1]`. Point it at this sample's own directory:

```shell
./build/samples/worlds/worlds_test samples/worlds/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` `A` `S` `D` | Walk the cloud world's Sunny |
| Arrow keys | Walk the isometric world's Sunny |
| `I` `J` `K` `L` | Walk the grid world's Sunny (it starts where the four maps meet) |
| `1` | The right view shows world B, the default world, then the grid world |
| `2` | Cross-fade the left half to world B (one second), and back |
| `3` | Remove world B (its views go back to the default world) / create it again |

The keys are listed on screen too, under the right view, with the number of worlds, the world the right view shows (B, cloud or grid), and the cross-fade's opacity. The grid world's Sunny (IJKL) is only seen while the right view shows the grid. Walking repeats while a key is held; the toggles act once per press.
