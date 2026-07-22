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
 #include "tilemap/itilemaplistener.h"
 #include "sprite/sprite.h"
 #include "scripting/scriptprocessor.h"
 #include "scripting/iscriptlistener.h"
 #include "sound/soundmanager.h"
 #include <memory>
 #include <string>


 /**
 * @brief World class implementation.
 *
 * Demonstrates SunLight :: Scripting :: ScriptProcessor by driving a small
 * "stage intro" cutscene: a queued script loads a stage, slides two sprites
 * onto screen, plays/pauses/resumes a background tone through SoundManager,
 * then settles into an idle loop using every control-flow command the
 * processor offers (WAIT_CMD, WAIT_SPRITES_QUEUE_EMPTY, LOOP_CMD/
 * END_LOOP_CMD, LABEL_CMD/GOTO_LABEL_CMD) while Sunny walks a continuous
 * circle and the monkey patrols a rectangle in the background, both
 * independent of the script's own timing.
 */
class World : public SunLight :: Scripting :: IScriptListener,
              public SunLight :: TileMap :: ITileMapListener {

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteSunny;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasSunny;
    std :: unique_ptr<SunLight :: Sprite :: Sprite>             m_pSpriteMonkey;
    std :: unique_ptr<SunLight :: Canvas :: TextureCanvas>      m_pCanvasMonkey;
    SunLight :: Scripting :: ScriptProcessor                    m_Script;
    SunLight :: Sound :: SoundManager                           m_SoundManager;
    std :: string m_strBasePath;

    // Per-sprite "slide onto screen" animation state.
    bool  m_bSunnySliding;
    int   m_nSunnyTargetX;
    bool  m_bMonkeySliding;
    int   m_nMonkeyTargetX;

    // Sunny's idle "walk in a circle" animation state - starts once her
    // entrance slide finishes and runs forever after that, independent of
    // the script's own LOOP_CMD/GOTO_LABEL_CMD idle cycle.
    bool   m_bSunnyCircling;
    float  m_fSunnyCircleAngle;

    // The monkey's idle "patrol a rectangle" animation state - same idea,
    // starts once her entrance slide finishes. m_nMonkeyPatrolEdge tracks
    // which of the rectangle's four edges she's currently walking
    // (0=top, 1=right, 2=bottom, 3=left).
    bool  m_bMonkeyPatrolling;
    int   m_nMonkeyPatrolEdge;

    void ZoomIn( SunLight :: Input :: ControllerType type, int nId );
    void ZoomOut( SunLight :: Input :: ControllerType type, int nId );
    void ResetZoom( SunLight :: Input :: ControllerType type, int nId );
    void StopSongKey( SunLight :: Input :: ControllerType type, int nId );

    bool LoadSprites( void );
    void LoadStage( uint16_t nStageId );
    void StartSpriteEntrance( uint16_t nSpriteId );
    void UpdateSunnyCircle( void );
    void UpdateMonkeyPatrol( void );
    void BuildScript( void );

    public :

    World( std :: string strBasePath );

    bool Run( void );

    // SunLight :: Scripting :: IScriptListener
    void OnCommand( SunLight :: Scripting :: Commands cmd, uint16_t nEventId );
    void OnError( std :: string strError );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap );
    void OnStop( void );
};

 #endif // __WORLD_H__
