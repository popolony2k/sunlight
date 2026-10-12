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
 * "terrain" (factor 1, where Sunny stands - grass, a few scattered "mountain"-shaded patches
 * and houses) is the only opaque layer; "sky" and "clouds" are translucent washes drawn ON TOP
 * of it, each at their own slower parallax factor - authored directly in the TMX via Tiled's
 * own parallaxx/parallaxy, L1's own feature - so they read as drifting tints/cloud shadows
 * over the terrain rather than a hidden backdrop (an earlier version of this sample tried
 * layering sky/clouds BEHIND an opaque or partly-gapped terrain layer and, even after several
 * iterations, it never read clearly as depth - a translucent overlay on top of an always-fully-
 * visible terrain is both simpler and unambiguous).
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
