/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

 #ifndef __WORLD_H__
 #define __WORLD_H__

 #include "renderer/tilemaprenderer.h"
 #include "sprite/sprite.h"
 #include "collision/icollisionlistener.h"
 #include "tilemap/itilemaplistener.h"
 #include <cstdint>
 #include <memory>
 #include <string>


 /**
 * @brief World class implementation - isometric collision check (E1 follow-up): Sunny walks the
 * isometric map's own diamond lattice, one grid cell at a time along its own two axes (not plain
 * screen up/down/left/right, which would look orthogonal), smoothly animated between cells
 * (OnUpdate). A move only starts if the target cell is clear - checked directly (GetTile against
 * the "obstacles" layer) rather than relying on AddColliderToTileRule's own per-frame check during
 * the animation, since a sprite mid-flight between two cells sits at a position the isometric
 * lattice's overlapping cell bounding boxes can resolve ambiguously (see E1b); AddColliderToTileRule
 * stays registered as a safety net for whenever Sunny is actually at rest on a cell, which is where
 * its own lookup is reliable. The camera follows her only once she reaches the viewport's own edge
 * (not every step), same as TileMapRenderer's own A6-clamped MoveCameraUp/Down/Left/Right already
 * stop at the map's limits.
 */
class World : public SunLight :: Collision :: ICollisionListener,
              public SunLight :: TileMap :: ITileMapListener {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;

    // Sunny's own grid cell, tracked explicitly rather than re-derived from her pixel position
    // (TileMapToTileMatrix) - that inverse is exact only at a cell's own corner (see above), not
    // reliable mid-animation, so the logical cell she occupies is state this class owns instead.
    int                                                        m_nSunnyRow = 4;
    int                                                        m_nSunnyCol = 2;

    // A key held down fires its handler every frame; TryMoveStep ignores a call while an
    // animation from the previous step is still playing (see m_bAnimating) - that alone paces
    // movement to one cell per __MOVE_ANIM_MS, no separate cooldown needed.
    bool                                                       m_bAnimating = false;
    int64_t                                                    m_nAnimStartMs = 0;
    SunLight :: Base :: stCoordinate2D                        m_AnimFrom { 0, 0 };
    SunLight :: Base :: stCoordinate2D                        m_AnimTo   { 0, 0 };

    // MoveCameraUp/Down/Left/Right apply their own full scroll step in a single instant call -
    // TryMoveStep still calls them (needs their own A6 clamp to know the true, legal result), but
    // immediately reverts the camera and replays the same before/after span smoothly here, in the
    // same GetCameraPosition()-reported terms, over the same span as m_AnimFrom/m_AnimTo - so the
    // world doesn't visibly snap in one frame while Sunny is still gliding to her own target.
    SunLight :: Base :: stCoordinate2D                        m_CamAnimFrom { 0, 0 };
    SunLight :: Base :: stCoordinate2D                        m_CamAnimTo   { 0, 0 };

    // Moves Sunny by exactly one grid cell along the isometric lattice's own two axes (nDeltaRow,
    // nDeltaCol each -1/0/+1, matching stMatrixPosition's row/col), converted to the matching
    // screen diagonal via the same CellTopLeft step confirmed in E1a/E1b - never a screen-straight
    // up/down/left/right pixel nudge, which is what made Sunny look like she was walking an
    // orthogonal grid. Scrolls the camera instead, on whichever axis/axes the step would carry
    // her past the viewport's own edge; a plain grid move animates smoothly to the target cell,
    // but only once the target is confirmed clear (see the class's own doc comment).
    void TryMoveStep( int nDeltaRow, int nDeltaCol );

    void MovePlayerUp( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerDown( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerRight( SunLight :: Input :: ControllerType type, int nId );
    void ZoomIn( SunLight :: Input :: ControllerType type, int nId );
    void ZoomOut( SunLight :: Input :: ControllerType type, int nId );
    void ResetZoom( SunLight :: Input :: ControllerType type, int nId );

    bool LoadSprites( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );

    // SunLight :: Collision :: ICollisionListener
    void OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: Collision :: Collider *pSecond );
    void OnCollision( SunLight :: Collision :: Collider *pFirst, SunLight :: TileMap :: stTile *pSecond );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap );
    void OnStop( void );
};

 #endif // __WORLD_H__
