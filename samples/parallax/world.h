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
 #include <memory>
 #include <string>


 /**
 * @brief World class implementation - L1 of the master plan (per-layer camera and parallax),
 * demonstrated as a top-down "eternal map": Sunny's own sprite stays fixed at the viewport's
 * centre (screen space, never moved) and what actually moves is the camera, recentred every
 * step on Sunny's own logical world position - the same side-scroller technique samples/
 * sprite's own MoveCameraLeft/Right/Up/Down demonstrates, just driven by a tracked position
 * instead of the renderer's own scroll-step boundary. Four marker objects (wrap_left/right/
 * top/bottom, the same mechanism Caravellius already uses on its own side - sunlight itself
 * has no built-in map-wrap) bound a rectangle Sunny's logical position is kept inside: crossing
 * one teleports her to the opposite marker, carrying over the exact overflow so there is no
 * stutter, and the camera follows in the very same step, so the wrap reads as instantaneous.
 * Four tile layers, back to front: "terrain" (factor 1 - grass with a few scattered
 * "mountain"-shaded patches, a generated placeholder since no real mountain tileset was
 * available among the reused assets); "buildings" (also factor 1, but a SEPARATE layer drawn
 * right after terrain, not baked into it - a house's own tile art has transparent corners
 * around its peaked roof, so it needs an opaque layer already drawn behind it for those
 * corners to show grass instead of the renderer's raw clear colour, confirmed live: baking
 * the houses directly into terrain, overwriting the grass tile outright, showed a black box
 * around each one); "sky" (factor 0.25, a sparse scatter of a generated solid-colour puff);
 * "clouds" (factor 0.5, a sparse scatter of real cloud art reused from samples/sprite's own
 * sky.png - the compact single-tile puff and the wider two-tile cluster, specifically, the
 * two shapes confirmed not to read as a clipped/incomplete fragment at in-game scale, unlike
 * a couple of others in the same sheet). Sunny's own sprite is attached to "clouds" - the
 * LAST layer - purely for draw order (a sprite draws right after its own container layer's
 * tiles): not because she is logically part of the clouds, but because that is what keeps her
 * drawn on top of everything, never covered by a drifting cloud that happens to land on her
 * fixed screen position (confirmed live: translucent clouds still visibly obscured her when
 * she was attached to "terrain" instead, the layer she actually stands on). Every layer's own
 * pattern repeats at exactly its own effective shift distance at the wrap boundary, not a
 * single shared period, and both "terrain" and "buildings" share the same period (they move
 * at the same factor, so the same effective shift) - the actual requirement for every layer to
 * loop seamlessly at once, not just the one whose factor happens to be 1.
 */
class World {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;

    // Sunny's own logical position in the map's pixel space - the single source of truth the
    // camera is recomputed from every step; her own sprite never moves (screen space, fixed at
    // the viewport's centre throughout).
    float  m_fWorldX = 0.0f;
    float  m_fWorldY = 0.0f;

    // The wrap rectangle, read once from the map's own marker objects at load time.
    float  m_fWrapLeft   = 0.0f;
    float  m_fWrapRight  = 0.0f;
    float  m_fWrapTop    = 0.0f;
    float  m_fWrapBottom = 0.0f;

    void MovePlayerUp( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerDown( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerLeft( SunLight :: Input :: ControllerType type, int nId );
    void MovePlayerRight( SunLight :: Input :: ControllerType type, int nId );
    void ZoomIn( SunLight :: Input :: ControllerType type, int nId );
    void ZoomOut( SunLight :: Input :: ControllerType type, int nId );
    void ResetZoom( SunLight :: Input :: ControllerType type, int nId );

    bool LoadSprites( void );
    bool LoadWrapMarkers( void );
    void WrapWorldPosition( void );
    void UpdateCameraOnSunny( void );
    void RecenterSunnyOnScreen( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );
};

 #endif // __WORLD_H__
