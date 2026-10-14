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

#include "world.h"
#include "input/icontrollermanager.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <vector>


#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __GAME_NAME                 "Input configuration test (no map)"

namespace  {

    namespace SI = SunLight :: Input;

    // The play area (left) and the information panel (right), in screen pixels.
    const int     __AREA_X             = 40;
    const int     __AREA_Y             = 60;
    const int     __AREA_W             = 720;
    const int     __AREA_H             = 800;
    const int     __PANEL_X            = 790;
    const int     __PANEL_Y            = 60;
    const int     __TITLE_Y            = 20;
    const int     __TITLE_FONT_SIZE    = 20;
    // raylib's built-in font is 10 pixels high: drawn at a whole multiple of that, every glyph is
    // scaled evenly and stays sharp (at 16 it was scaled by 1.6, and the letters ran together).
    const int     __FONT_SIZE          = 20;
    const int     __LINE_HEIGHT        = 26;
    const int     __SECTION_GAP        = 12;
    const size_t  __MAX_PAD_NAME       = 12;      // characters of a pad's name shown in the panel
    const unsigned char  __OPAQUE      = 0xFF;

    // The players: squares moved by actions, PLAYER_SPEED screen pixels per frame.
    const int     __PLAYER_SIZE        = 40;
    const float   __PLAYER_SPEED       = 4.0f;
    const float   __KEYBOARD_START_X   = 200.0f;
    const float   __KEYBOARD_START_Y   = 400.0f;
    const float   __SLOT_START_X       = 400.0f;
    const float   __SLOT_START_Y_STEP  = 150.0f;   // slot n starts at n x this
    const int     __LABEL_INSET        = 8;

    // Two player slots, numbered as players are: 1 and 2.
    const int     __FIRST_SLOT         = 1;
    const int     __SLOT_COUNT         = 2;

    // The stick dead zone, changed with [ and ].
    const float   __DEADZONE_STEP      = 0.05f;
    const float   __DEADZONE_MIN       = 0.0f;
    const float   __DEADZONE_MAX       = 0.9f;

    const size_t  __EVENT_LOG_LINES    = 6;
    const int     __NO_REBIND          = -1;
    const char   *__BINDINGS_FILE_NAME = "sunlight_input_bindings.txt";

    // The keyboard player's actions, rebound with F1..F4 (same order).
    const int     __KEY_ACTION_COUNT   = 4;
    const char   *__KEY_ACTIONS[__KEY_ACTION_COUNT]   = { "Left", "Right", "Up", "Down" };
    const SI :: KeyboardKey  __DEFAULT_KEYS[__KEY_ACTION_COUNT] = { SI :: KEY_LEFT, SI :: KEY_RIGHT, SI :: KEY_UP, SI :: KEY_DOWN };
    const SI :: KeyboardKey  __REBIND_KEYS[__KEY_ACTION_COUNT]  = { SI :: KEY_F1, SI :: KEY_F2, SI :: KEY_F3, SI :: KEY_F4 };

    // The pad players' actions: the left stick, one direction each. One action map serves every
    // pad - the pad to read is chosen when the action is polled.
    const char   *__PAD_LEFT           = "Pad left";
    const char   *__PAD_RIGHT          = "Pad right";
    const char   *__PAD_UP             = "Pad up";
    const char   *__PAD_DOWN           = "Pad down";
    const int     __NEGATIVE           = -1;
    const int     __POSITIVE           = 1;

    struct stRgb  { unsigned char nRed, nGreen, nBlue; };

    const stRgb   __AREA_COLOR         = { 0x20, 0x20, 0x28 };
    const stRgb   __TEXT_COLOR         = { 0xE0, 0xE0, 0xE0 };
    const stRgb   __TITLE_COLOR        = { 0xFF, 0xD0, 0x40 };
    const stRgb   __HIGHLIGHT_COLOR    = { 0x60, 0xD0, 0xFF };
    const stRgb   __LABEL_COLOR        = { 0x00, 0x00, 0x00 };
    const stRgb   __KEYBOARD_COLOR     = { 0xF0, 0xF0, 0xF0 };
    const stRgb   __SLOT_COLORS[__SLOT_COUNT] = { { 0x50, 0xD0, 0x60 }, { 0xF0, 0x90, 0x30 } };
    const stRgb   __OFFLINE_COLOR      = { 0x70, 0x70, 0x70 };   // a slot whose pad is unplugged

    /**
     * @brief A readable name for the keys a player is likely to bind. sunlight has no key-name
     * table of its own; letters and digits are their ASCII codes, as in raylib.
     */
    std :: string KeyName( int nKey )  {

        if( ( ( nKey >= SI :: KEY_A ) && ( nKey <= SI :: KEY_Z ) ) || ( ( nKey >= SI :: KEY_ZERO ) && ( nKey <= SI :: KEY_NINE ) ) )
            return std :: string( 1, ( char ) nKey );

        switch( nKey )  {
            case SI :: KEY_LEFT          : return "Left arrow";
            case SI :: KEY_RIGHT         : return "Right arrow";
            case SI :: KEY_UP            : return "Up arrow";
            case SI :: KEY_DOWN          : return "Down arrow";
            case SI :: KEY_SPACE         : return "Space";
            case SI :: KEY_ENTER         : return "Enter";
            case SI :: KEY_TAB           : return "Tab";
            case SI :: KEY_BACKSPACE     : return "Backspace";
            case SI :: KEY_LEFT_SHIFT    : return "Left Shift";
            case SI :: KEY_RIGHT_SHIFT   : return "Right Shift";
            case SI :: KEY_LEFT_CONTROL  : return "Left Ctrl";
            default                      : return "key " + std :: to_string( nKey );
        }
    }

    /**
     * @brief The keys this sample uses for itself, which a keyboard action may not take.
     */
    bool IsSampleKey( SI :: KeyboardKey key )  {

        static const std :: set<int>  sampleKeys = { SI :: KEY_F1, SI :: KEY_F2, SI :: KEY_F3, SI :: KEY_F4, SI :: KEY_F5, SI :: KEY_F9,
                                                      SI :: KEY_LEFT_BRACKET, SI :: KEY_RIGHT_BRACKET, SI :: KEY_ESCAPE };

        return sampleKeys.count( key ) > 0;
    }

    void Clamp( float &fValue, float fMin, float fMax )  {
        fValue = std :: max( fMin, std :: min( fMax, fValue ) );
    }
}

/**
 * @brief Whether a key went down this frame (it is down now, and was not the frame before).
 * Called every frame for every key the sample reacts to, so the previous state stays current.
 * @param key The key to check;
 */
bool World :: KeyPressedOnce( SunLight :: Input :: KeyboardKey key )  {

    bool  bDown = m_pRenderer -> GetInputHandler().IsKeyDown( key );
    bool  bPrev = m_PrevKeyDown[key];

    m_PrevKeyDown[key] = bDown;

    return bDown && !bPrev;
}

/**
 * @brief Whether a gamepad button went down this frame, per pad (see KeyPressedOnce).
 * @param nGamePadId The pad to check;
 * @param button The button to check;
 */
bool World :: ButtonPressedOnce( int nGamePadId, SunLight :: Input :: GamepadButton button )  {

    bool  bDown = m_pRenderer -> GetInputHandler().IsGamepadButtonDown( nGamePadId, button );
    bool  bPrev = m_PrevButtonDown[ { nGamePadId, button } ];

    m_PrevButtonDown[ { nGamePadId, button } ] = bDown;

    return bDown && !bPrev;
}

/**
 * @brief The starting bindings: the arrow keys for the keyboard player, the left stick for the
 * pad players.
 */
void World :: BindDefaults( void )  {

    for( int nAction = 0; nAction < __KEY_ACTION_COUNT; nAction++ )
        m_ActionMap.BindKey( __KEY_ACTIONS[nAction], __DEFAULT_KEYS[nAction] );

    m_ActionMap.BindGamepadAxis( __PAD_LEFT, SI :: GAMEPAD_AXIS_LEFT_X, __NEGATIVE );
    m_ActionMap.BindGamepadAxis( __PAD_RIGHT, SI :: GAMEPAD_AXIS_LEFT_X, __POSITIVE );
    m_ActionMap.BindGamepadAxis( __PAD_UP, SI :: GAMEPAD_AXIS_LEFT_Y, __NEGATIVE );
    m_ActionMap.BindGamepadAxis( __PAD_DOWN, SI :: GAMEPAD_AXIS_LEFT_Y, __POSITIVE );
}

/**
 * @brief F1..F4 pick a keyboard action to rebind; the next key pressed becomes its binding.
 * ActionMap::BindKey refuses a key another action already has, and the panel says which.
 */
void World :: HandleRebinding( void )  {

    SunLight :: Input :: IInputHandler  &input = m_pRenderer -> GetInputHandler();

    for( int nAction = 0; nAction < __KEY_ACTION_COUNT; nAction++ )  {
        if( KeyPressedOnce( __REBIND_KEYS[nAction] ) )  {
            m_nRebindAction = nAction;
            m_strStatus     = std :: string( "Press the new key for " ) + __KEY_ACTIONS[nAction];

            // The F key itself is in this frame's queue of pressed keys: drop it.
            while( input.GetKeyPressed() != SI :: KEY_NULL )
                ;
        }
    }

    if( m_nRebindAction == __NO_REBIND )
        return;

    for( SI :: KeyboardKey key = input.GetKeyPressed(); key != SI :: KEY_NULL; key = input.GetKeyPressed() )  {
        if( IsSampleKey( key ) )
            continue;

        const char  *szAction = __KEY_ACTIONS[m_nRebindAction];

        if( m_ActionMap.BindKey( szAction, key ) )
            m_strStatus = std :: string( szAction ) + " is now " + KeyName( key );
        else  {
            m_strStatus = "Refused: " + KeyName( key ) + " is in use";

            for( int nAction = 0; nAction < __KEY_ACTION_COUNT; nAction++ )  {
                SI :: stActionBinding  binding = m_ActionMap.GetBinding( __KEY_ACTIONS[nAction] );

                if( ( binding.type == SI :: ACTION_BINDING_KEY ) && ( binding.nCode == key ) )
                    m_strStatus += std :: string( " by " ) + __KEY_ACTIONS[nAction];
            }
        }

        m_nRebindAction = __NO_REBIND;
        break;
    }
}

/**
 * @brief F5 saves every binding to a file, F9 loads it back (ActionMap::Save/Load). The file is
 * in the system's temporary directory, so the sample never writes into its own folder.
 */
void World :: HandleSaveLoad( void )  {

    bool  bSave = KeyPressedOnce( SI :: KEY_F5 );
    bool  bLoad = KeyPressedOnce( SI :: KEY_F9 );

    if( bSave )
        m_strStatus = m_ActionMap.Save( m_strBindingsFile ) ? "Bindings saved" : "Cannot save the bindings";

    if( bLoad )
        m_strStatus = m_ActionMap.Load( m_strBindingsFile ) ? "Bindings loaded" : "No saved bindings to load";
}

/**
 * @brief On every connected pad, A claims the first free player slot and B releases the slot
 * the pad holds (PlayerSlotManager). A claim survives the pad being unplugged: plugged back,
 * the same pad is the same player again.
 */
void World :: HandleSlots( void )  {

    for( const SI :: stGamepadInfo &pad : m_pRenderer -> GetControllerManager().GetConnectedGamepads() )  {
        bool  bClaim   = ButtonPressedOnce( pad.nGamePadId, SI :: GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        bool  bRelease = ButtonPressedOnce( pad.nGamePadId, SI :: GAMEPAD_BUTTON_RIGHT_FACE_RIGHT );
        int   nSlot    = m_Slots.GetSlotForGamepad( pad.nGamePadId );

        if( bClaim && ( nSlot == NO_SLOT_ASSOCIATION ) )  {
            m_strStatus = "No free slot for pad " + std :: to_string( pad.nGamePadId );

            for( int nFree = __FIRST_SLOT; nFree < __FIRST_SLOT + __SLOT_COUNT; nFree++ )  {
                if( ( m_Slots.GetGamepadForSlot( nFree ) == NO_SLOT_ASSOCIATION ) && m_Slots.ClaimSlot( nFree, pad.nGamePadId ) )  {
                    m_SlotPlayers[nFree] = { __SLOT_START_X, nFree * __SLOT_START_Y_STEP };
                    m_strStatus = "Pad " + std :: to_string( pad.nGamePadId ) + " is player " + std :: to_string( nFree );
                    break;
                }
            }
        }

        if( bRelease && ( nSlot != NO_SLOT_ASSOCIATION ) )  {
            m_Slots.ReleaseSlot( nSlot );
            m_SlotPlayers.erase( nSlot );
            m_strStatus = "Pad " + std :: to_string( pad.nGamePadId ) + " left slot " + std :: to_string( nSlot );
        }
    }
}

/**
 * @brief [ and ] lower and raise the left stick's dead zone on every connected pad.
 */
void World :: HandleDeadzone( void )  {

    bool  bLower = KeyPressedOnce( SI :: KEY_LEFT_BRACKET );
    bool  bRaise = KeyPressedOnce( SI :: KEY_RIGHT_BRACKET );

    if( !bLower && !bRaise )
        return;

    m_fDeadzone += ( bRaise ? __DEADZONE_STEP : 0.0f ) - ( bLower ? __DEADZONE_STEP : 0.0f );
    Clamp( m_fDeadzone, __DEADZONE_MIN, __DEADZONE_MAX );

    for( const SI :: stGamepadInfo &pad : m_pRenderer -> GetControllerManager().GetConnectedGamepads() )
        ApplyDeadzone( pad.nGamePadId );

    char  szStatus[64];

    snprintf( szStatus, sizeof( szStatus ), "Dead zone: %.2f", m_fDeadzone );
    m_strStatus = szStatus;
}

/**
 * @brief Sets the left stick's dead zone (both axes) of one pad to the sample's current value.
 * @param nGamePadId The pad;
 */
void World :: ApplyDeadzone( int nGamePadId )  {

    SI :: IControllerManager  &manager = m_pRenderer -> GetControllerManager();

    manager.SetAxisDeadzone( nGamePadId, SI :: GAMEPAD_AXIS_LEFT_X, m_fDeadzone );
    manager.SetAxisDeadzone( nGamePadId, SI :: GAMEPAD_AXIS_LEFT_Y, m_fDeadzone );
}

/**
 * @brief Moves every player by polling its actions (ActionMap::IsActionActive). A pad player
 * reads its own pad, with that pad's dead zone as the threshold a stick must pass.
 */
void World :: MovePlayers( void )  {

    SunLight :: Input :: IInputHandler   &input   = m_pRenderer -> GetInputHandler();
    SI :: IControllerManager             &manager = m_pRenderer -> GetControllerManager();
    const float                          fMaxX    = ( float ) ( __AREA_W - __PLAYER_SIZE );
    const float                          fMaxY    = ( float ) ( __AREA_H - __PLAYER_SIZE );

    // A key binding ignores the pad id and the threshold.
    m_KeyboardPlayer.fX += ( m_ActionMap.IsActionActive( "Right", input, 0, 0.0f ) ? __PLAYER_SPEED : 0.0f ) -
                           ( m_ActionMap.IsActionActive( "Left", input, 0, 0.0f ) ? __PLAYER_SPEED : 0.0f );
    m_KeyboardPlayer.fY += ( m_ActionMap.IsActionActive( "Down", input, 0, 0.0f ) ? __PLAYER_SPEED : 0.0f ) -
                           ( m_ActionMap.IsActionActive( "Up", input, 0, 0.0f ) ? __PLAYER_SPEED : 0.0f );
    Clamp( m_KeyboardPlayer.fX, 0.0f, fMaxX );
    Clamp( m_KeyboardPlayer.fY, 0.0f, fMaxY );

    for( std :: pair<const int, stPlayer> &slotPlayer : m_SlotPlayers )  {
        int    nPad    = m_Slots.GetGamepadForSlot( slotPlayer.first );
        float  fDeadX  = manager.GetAxisDeadzone( nPad, SI :: GAMEPAD_AXIS_LEFT_X );
        float  fDeadY  = manager.GetAxisDeadzone( nPad, SI :: GAMEPAD_AXIS_LEFT_Y );

        slotPlayer.second.fX += ( m_ActionMap.IsActionActive( __PAD_RIGHT, input, nPad, fDeadX ) ? __PLAYER_SPEED : 0.0f ) -
                                ( m_ActionMap.IsActionActive( __PAD_LEFT, input, nPad, fDeadX ) ? __PLAYER_SPEED : 0.0f );
        slotPlayer.second.fY += ( m_ActionMap.IsActionActive( __PAD_DOWN, input, nPad, fDeadY ) ? __PLAYER_SPEED : 0.0f ) -
                                ( m_ActionMap.IsActionActive( __PAD_UP, input, nPad, fDeadY ) ? __PLAYER_SPEED : 0.0f );
        Clamp( slotPlayer.second.fX, 0.0f, fMaxX );
        Clamp( slotPlayer.second.fY, 0.0f, fMaxY );
    }
}

/**
 * @brief Keeps the last __EVENT_LOG_LINES connect/disconnect events for the panel, and prints
 * each one to the console too.
 * @param strEvent The event, as shown;
 */
void World :: LogEvent( const std :: string &strEvent )  {

    printf( "%s\n", strEvent.c_str() );
    m_EventLog.push_back( strEvent );

    if( m_EventLog.size() > __EVENT_LOG_LINES )
        m_EventLog.pop_front();
}

/**
 * @brief The play area and the players, through IDrawSurface (raw screen pixels).
 */
void World :: DrawPlayers( void )  {

    SunLight :: DrawSurface :: IDrawSurface  &surface = *m_pRenderer;
    std :: set<int>                           connected;

    for( const SI :: stGamepadInfo &pad : m_pRenderer -> GetControllerManager().GetConnectedGamepads() )
        connected.insert( pad.nGamePadId );

    surface.DrawFilledRectangle( __AREA_X, __AREA_Y, __AREA_W, __AREA_H, __AREA_COLOR.nRed, __AREA_COLOR.nGreen, __AREA_COLOR.nBlue, __OPAQUE );

    auto  drawPlayer = [&]( const stPlayer &player, const stRgb &color, const char *szLabel )  {
        int  nX = __AREA_X + ( int ) player.fX;
        int  nY = __AREA_Y + ( int ) player.fY;

        surface.DrawFilledRectangle( nX, nY, __PLAYER_SIZE, __PLAYER_SIZE, color.nRed, color.nGreen, color.nBlue, __OPAQUE );
        surface.DrawText( szLabel, nX + __LABEL_INSET, nY + __LABEL_INSET, __FONT_SIZE, __LABEL_COLOR.nRed, __LABEL_COLOR.nGreen, __LABEL_COLOR.nBlue, __OPAQUE );
    };

    drawPlayer( m_KeyboardPlayer, __KEYBOARD_COLOR, "K" );

    for( const std :: pair<const int, stPlayer> &slotPlayer : m_SlotPlayers )  {
        int          nPad    = m_Slots.GetGamepadForSlot( slotPlayer.first );
        const stRgb  &color  = connected.count( nPad ) ? __SLOT_COLORS[slotPlayer.first - __FIRST_SLOT] : __OFFLINE_COLOR;
        std :: string strLabel = "P" + std :: to_string( slotPlayer.first );

        drawPlayer( slotPlayer.second, color, strLabel.c_str() );
    }
}

/**
 * @brief The information panel: bindings, pads, slots, dead zone, status and events.
 */
void World :: DrawPanel( void )  {

    SunLight :: DrawSurface :: IDrawSurface  &surface = *m_pRenderer;
    SunLight :: Input :: IInputHandler       &input   = m_pRenderer -> GetInputHandler();
    int                                      nY       = __PANEL_Y;
    char                                     szLine[128];

    auto  line = [&]( const std :: string &strText, const stRgb &color )  {
        surface.DrawText( strText.c_str(), __PANEL_X, nY, __FONT_SIZE, color.nRed, color.nGreen, color.nBlue, __OPAQUE );
        nY += __LINE_HEIGHT;
    };

    surface.DrawText( "Input configuration - arrows move K, Esc quits", __AREA_X, __TITLE_Y, __TITLE_FONT_SIZE,
                      __TITLE_COLOR.nRed, __TITLE_COLOR.nGreen, __TITLE_COLOR.nBlue, __OPAQUE );

    line( "Keys (F1-F4 rebind):", __TITLE_COLOR );

    for( int nAction = 0; nAction < __KEY_ACTION_COUNT; nAction++ )  {
        SI :: stActionBinding  binding = m_ActionMap.GetBinding( __KEY_ACTIONS[nAction] );
        std :: string          strKey  = ( binding.type == SI :: ACTION_BINDING_KEY ) ? KeyName( binding.nCode ) : "(none)";

        if( nAction == m_nRebindAction )
            line( "  F" + std :: to_string( nAction + 1 ) + " " + __KEY_ACTIONS[nAction] + ": press a key...", __HIGHLIGHT_COLOR );
        else
            line( "  F" + std :: to_string( nAction + 1 ) + " " + __KEY_ACTIONS[nAction] + ": " + strKey, __TEXT_COLOR );
    }

    line( "  F5 save, F9 load", __TEXT_COLOR );
    nY += __SECTION_GAP;

    line( "Pads (A join, B leave):", __TITLE_COLOR );

    std :: vector<SI :: stGamepadInfo>  pads = m_pRenderer -> GetControllerManager().GetConnectedGamepads();

    if( pads.empty() )
        line( "  none connected", __TEXT_COLOR );

    for( const SI :: stGamepadInfo &pad : pads )  {
        std :: string  strName = pad.szName ? pad.szName : "?";

        if( strName.size() > __MAX_PAD_NAME )
            strName.resize( __MAX_PAD_NAME );

        snprintf( szLine, sizeof( szLine ), "  #%d %s %+.2f %+.2f", pad.nGamePadId, strName.c_str(),
                  input.GetGamepadAxisMovement( pad.nGamePadId, SI :: GAMEPAD_AXIS_LEFT_X ),
                  input.GetGamepadAxisMovement( pad.nGamePadId, SI :: GAMEPAD_AXIS_LEFT_Y ) );
        line( szLine, __TEXT_COLOR );
    }

    nY += __SECTION_GAP;
    line( "Player slots:", __TITLE_COLOR );

    for( int nSlot = __FIRST_SLOT; nSlot < __FIRST_SLOT + __SLOT_COUNT; nSlot++ )  {
        int  nPad = m_Slots.GetGamepadForSlot( nSlot );

        if( nPad == NO_SLOT_ASSOCIATION )
            snprintf( szLine, sizeof( szLine ), "  P%d: free", nSlot );
        else  {
            bool  bConnected = std :: any_of( pads.begin(), pads.end(), [nPad]( const SI :: stGamepadInfo &pad ) { return pad.nGamePadId == nPad; } );

            snprintf( szLine, sizeof( szLine ), "  P%d: pad %d%s", nSlot, nPad, bConnected ? "" : " (unplugged)" );
        }

        line( szLine, __SLOT_COLORS[nSlot - __FIRST_SLOT] );
    }

    nY += __SECTION_GAP;
    snprintf( szLine, sizeof( szLine ), "Dead zone ([ ]): %.2f", m_fDeadzone );
    line( szLine, __TITLE_COLOR );

    nY += __SECTION_GAP;
    line( "Status:", __TITLE_COLOR );
    line( "  " + m_strStatus, __HIGHLIGHT_COLOR );

    nY += __SECTION_GAP;
    line( "Pad events:", __TITLE_COLOR );

    for( const std :: string &strEvent : m_EventLog )
        line( "  " + strEvent, __TEXT_COLOR );
}

/**
 * @brief Constructor. Initializes class data.
 */
World :: World( void )  {

    std :: error_code  errorCode;

    m_pRenderer      = std :: make_unique<SunLight :: Renderer :: TileMapRenderer>( __DISPLAY_W,
                                                                                    __DISPLAY_H,
                                                                                    __GAME_NAME,
                                                                                    __FRAMES_PER_SECOND );
    m_KeyboardPlayer = { __KEYBOARD_START_X, __KEYBOARD_START_Y };
    m_nRebindAction  = __NO_REBIND;
    m_fDeadzone      = DEFAULT_AXIS_DEADZONE;
    m_strStatus      = "Ready";
    m_strBindingsFile = ( std :: filesystem :: temp_directory_path( errorCode ) / __BINDINGS_FILE_NAME ).string();
}

/**
 * @brief Called by Run() once per frame, inside the frame's render target: poll, move, draw.
 */
void World :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

    HandleRebinding();
    HandleSaveLoad();
    HandleSlots();
    HandleDeadzone();
    MovePlayers();

    DrawPlayers();
    DrawPanel();
}

void World :: OnStop( void )  {
}

/**
 * @brief Run World configuration. LoadMap is never called.
 */
bool World :: Run( void )  {

    BindDefaults();

    // Fired by the controller manager once per connect/disconnect, from inside Run()'s frame -
    // pads already plugged in when the sample starts are reported on the first frame.
    m_pRenderer -> GetControllerManager().SetGamepadConnectionHandler( [this]( int nGamePadId, bool bConnected )  {
        const char  *szName = bConnected ? m_pRenderer -> GetInputHandler().GetGamepadName( nGamePadId ) : nullptr;

        if( bConnected )
            ApplyDeadzone( nGamePadId );

        std :: string  strName = szName ? szName : "?";

        if( strName.size() > __MAX_PAD_NAME )
            strName.resize( __MAX_PAD_NAME );

        LogEvent( "Pad " + std :: to_string( nGamePadId ) + ( bConnected ? " in: " + strName : " out" ) );
    } );

    if( !m_pRenderer -> Start() )  {
        fprintf( stderr, "Cannot start the renderer\n" );
        return false;
    }

    m_pRenderer -> AddTileMapListener( this );

    printf( "Arrows move K; F1-F4 rebind its actions, F5/F9 save/load them; A on a pad claims a player slot, B leaves it;\n" );
    printf( "[ and ] change the stick dead zone. Esc quits. Bindings file: %s\n", m_strBindingsFile.c_str() );

    m_pRenderer -> Run();
    m_pRenderer -> Stop();

    return true;
}
