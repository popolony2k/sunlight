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
 #include "input/igamepadhaptics.h"
 #include <cstdint>
 #include <memory>
 #include <set>
 #include <string>
 #include <utility>


 /**
 * @brief World class implementation - isometric collision check, with gamepad rumble feedback
 * (F7b of the master plan) added on top of the plain isometric sample it is otherwise an exact
 * copy of: Sunny walks the isometric map's own diamond lattice exactly as there, but a blocked
 * step into a "stone" obstacle now also rumbles a connected gamepad (if any - IsSupported gates
 * every call, so this sample behaves identically with no gamepad attached). The rumble's
 * strength scales with how many consecutive steps Sunny had already taken in that same
 * direction before the wall stopped her - a bump right after turning is minor, a bump at the
 * end of a long straight run is strong - tracked explicitly (m_nRunUpSteps/m_nLastDelta*) since
 * this sample's grid movement has no analog speed of its own to derive it from. Confirmed live
 * against a real controller: a naive "bump every blocked call" rumbled continuously for as long
 * as the key stayed held (TryMoveStep re-attempts, and re-blocks, every single frame it is
 * down), so a bump only fires on the first blocked frame of a given hold
 * (m_BlockedThisHold), not every frame after. A connected gamepad's D-pad/left stick
 * drives Sunny too, the same movement a keyboard does (AddGamePad/SetUserGamePadEventHandler in
 * Run(), binding the same MovePlayer* handlers).
 */
class World : public SunLight :: Collision :: ICollisionListener,
              public SunLight :: TileMap :: ITileMapListener {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;

    // Declared after m_pRenderer (destructed before it, in reverse declaration order) - it
    // holds a reference into m_pRenderer's own IInputHandler, which must outlive it.
    std :: unique_ptr<SunLight :: Input :: IGamepadHaptics>     m_pHaptics;

    // Sunny's own grid cell, tracked explicitly rather than re-derived from her pixel position
    // (TileMapToTileMatrix) - that inverse is exact only at a cell's own corner (see above), not
    // reliable mid-animation, so the logical cell she occupies is state this class owns instead.
    int                                                        m_nSunnyRow = 4;
    int                                                        m_nSunnyCol = 2;

    // The direction (row/col delta) of Sunny's most recent successful step, and how many
    // consecutive steps she has taken in that exact direction - the "run-up" TriggerWallBump
    // scales its rumble strength from. Reset to a fresh run of 1 on any direction change,
    // frozen (untouched) by a blocked step so a same-direction retry after bumping a wall
    // still reports zero run-up for that wall, not a stale one from before the turn.
    int                                                        m_nLastDeltaRow = 0;
    int                                                        m_nLastDeltaCol = 0;
    int                                                        m_nRunUpSteps   = 0;

    // Every direction (row/col delta) already blocked since Sunny last moved - a held key
    // re-attempts the same blocked step every frame (see m_bAnimating's own comment below for
    // why nothing else paces a blocked attempt the way it paces a successful one), so without
    // this a bump would re-fire, and re-arm its own duration, every single frame it stays held -
    // felt as one continuous, never-stopping rumble instead of one short pulse. A set, not just
    // the last direction: a gamepad's stick held at a diagonal sends BOTH of its directions every
    // frame (ControllerManager turns each axis past the deadzone into its own D-pad direction), so
    // against a corner the two blocked directions alternated, each looking new, and the sample
    // bumped twice a frame (found live with an Xbox controller: 395 bumps in a few seconds, the
    // frame rate down to about 40). Cleared on any successful move (she is no longer stuck
    // against anything).
    std :: set<std :: pair<int, int>>                          m_BlockedThisHold;

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
    // but only once the target is confirmed clear (see the class's own doc comment). A step
    // blocked by a "stone" obstacle triggers a rumble instead (see TriggerWallBump), scaled by
    // the run Sunny had already built up in that same direction.
    void TryMoveStep( int nDeltaRow, int nDeltaCol );

    // Rumbles a connected gamepad (a no-op if none is - IsSupported gates it) to mark a step
    // blocked by a "stone" obstacle, strength scaled by nRunUpSteps - the number of consecutive
    // steps Sunny had already taken in that exact direction before this one was refused.
    void TriggerWallBump( int nRunUpSteps );

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
    void OnCollision( SunLight :: Collision :: Collider* pFirst, SunLight :: Collision :: Collider* pSecond );
    void OnCollision( SunLight :: Collision :: Collider* pFirst, SunLight :: TileMap :: stTile* pSecond );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap );
    void OnStop( void );
};

 #endif // __WORLD_H__
