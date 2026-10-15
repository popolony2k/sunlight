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
 #include "input/actionmap.h"
 #include "input/playerslotmanager.h"
 #include <array>
 #include <deque>
 #include <map>
 #include <memory>
 #include <string>
 #include <utility>


 /**
 * @brief World class implementation - the input configuration of Phase F of the master plan,
 * on a drawing surface with no map: named actions (ActionMap) moving a keyboard player and
 * rebound at run time, saved and loaded; gamepads claiming and releasing player slots
 * (PlayerSlotManager); connect/disconnect events (the controller manager's hot-plug handler);
 * and a stick dead zone changed live (SetAxisDeadzone). Everything is polled and drawn from
 * OnUpdate, which Run() calls once per frame inside the frame's render target.
 */
class World : public SunLight :: TileMap :: ITileMapListener  {

    // A square moved by actions: the keyboard player, or the player of one slot.
    struct stPlayer  {
        float  fX;
        float  fY;
    };

    std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  m_pRenderer;
    SunLight :: Input :: ActionMap                              m_ActionMap;
    SunLight :: Input :: PlayerSlotManager                      m_Slots;

    stPlayer                                                    m_KeyboardPlayer;
    std :: map<int, stPlayer>                                   m_SlotPlayers;       // by slot number

    // Rebinding: the index of the keyboard action waiting for its new key, or NO_REBIND.
    int                                                         m_nRebindAction;
    std :: string                                               m_strStatus;
    std :: deque<std :: string>                                 m_EventLog;          // hot-plug events, newest last
    float                                                       m_fDeadzone;

    // The previous frame's state of each key/button the sample reacts to once per press.
    std :: map<int, bool>                                       m_PrevKeyDown;
    std :: map<std :: pair<int, int>, bool>                     m_PrevButtonDown;    // by (pad id, button)

    std :: string                                               m_strBindingsFile;

    bool KeyPressedOnce( SunLight :: Input :: KeyboardKey key );
    bool ButtonPressedOnce( int nGamePadId, SunLight :: Input :: GamepadButton button );

    void BindDefaults( void );
    void HandleRebinding( void );
    void HandleSlots( void );
    void HandleDeadzone( void );
    void HandleSaveLoad( void );
    void MovePlayers( void );
    void ApplyDeadzone( int nGamePadId );
    void LogEvent( const std :: string& strEvent );

    void DrawPlayers( void );
    void DrawPanel( void );

    public :

    World( void );

    bool Run( void );

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap ) override;
    void OnStop( void ) override;
};

 #endif // __WORLD_H__
