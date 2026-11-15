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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include "filesystem/filesystemfactory.h"
#include "canvas/texturecanvas.h"
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "tilemap/itilemaplistener.h"

using namespace SunLight :: Renderer;
typedef SunLight :: TileMap :: ITileMap  ITM;

namespace  {

    // The map is 31 x 23 tiles of 32 px, drawn whole at zoom 1 in a viewport of the same size.
    const int    __VIEWPORT_POS_X  = 40;
    const int    __VIEWPORT_POS_Y  = 80;
    const int    __VIEWPORT_WIDTH  = 992;
    const int    __VIEWPORT_HEIGHT = 736;

    const char * const  __MAP_FILE  = "resources/map/tint.tmx";
    const char * const  __FONT_FILE = "/shared/fonts/AnonymousPro-Bold.ttf";
    const char * const  __FONT_NAME = "Sans";   // the family the map's text objects ask for

    const int    __HUD_X           = 40;
    const int    __HUD_KEYS_Y      = 20;
    const int    __HUD_NOTE_Y      = 48;
    const int    __HUD_STATE_Y     = 836;
    const int    __HUD_LINE_HEIGHT = 26;
    const int    __HUD_SPRITE_LINE = 2;      // the sprite's line, after the layers' and the group's
    const int    __FONT_SIZE       = 20;
    const size_t __TINT_TEXT_SIZE  = 64;     // "#RRGGBB " and a tint's name
    const unsigned char  __OPAQUE  = 0xFF;

    // The layers whose tint the keys 1 to 4 cycle, by name, in key order.
    const char * const  __TINTED_LAYERS[] = { "tiles", "banner", "houses", "group" };
    const size_t        __TINTED_LAYER_COUNT = sizeof( __TINTED_LAYERS ) / sizeof( __TINTED_LAYERS[0] );
    const char * const  __GROUP_LAYER     = "group";

    struct stTint  {
        const char                   *szName;
        SunLight :: Base :: stColor   color;
    };

    // What each tint key cycles through after the layer's authored tint (choice 0).
    const stTint  __TINTS[] = { { "none (white)", { 0xFF, 0xFF, 0xFF, 0xFF } },
                                { "red",          { 0xFF, 0x50, 0x50, 0xFF } },
                                { "green",        { 0x50, 0xFF, 0x50, 0xFF } },
                                { "blue",         { 0x60, 0x80, 0xFF, 0xFF } },
                                { "gold",         { 0xFF, 0xC8, 0x40, 0xFF } } };
    const size_t  __TINT_COUNT = sizeof( __TINTS ) / sizeof( __TINTS[0] );

    // What the opacity key cycles the group through after its authored opacity (choice 0), 0..1.
    const float   __OPACITIES[] = { 1.0f, 0.75f, 0.25f, 0.0f };
    const size_t  __OPACITY_COUNT = sizeof( __OPACITIES ) / sizeof( __OPACITIES[0] );

    const SunLight :: Input :: KeyboardKey  __TINT_KEYS[] = { SunLight :: Input :: KEY_ONE,   SunLight :: Input :: KEY_TWO,
                                                               SunLight :: Input :: KEY_THREE, SunLight :: Input :: KEY_FOUR };

    // Sunny (I10 of the master plan): an animated sprite on the object layer "houses" - whose tint
    // does not reach it, a sprite takes only its own - in area 5 of the map.
    const char * const  __SUNNY_FILE      = "resources/sprites/sunny_idle_down.png";
    const int           __SUNNY_SIZE      = 32;     // one frame of the strip
    const int           __SUNNY_DELAY     = 100;    // ms per frame
    const int           __SUNNY_X         = 64;
    const int           __SUNNY_Y         = 640;
    const int           __SUNNY_LAYER_ID  = 3;      // "houses"
}

/**
 * @brief I8 of the master plan: layer tint at runtime. The keys read a layer with GetLayer,
 * change its tint (or the group's opacity) and write it back with SetLayer; the HUD shows what
 * each layer holds now.
 */
class TintSample : public SunLight :: TileMap :: ITileMapListener  {

    TileMapRenderer  &m_Renderer;

    // Per tinted layer: its authored tint, and which choice it shows (0 = authored, n = __TINTS[n - 1]).
    SunLight :: Base :: stColor  m_Authored[__TINTED_LAYER_COUNT];
    size_t                       m_nTintChoice[__TINTED_LAYER_COUNT] = {};

    // The group's authored opacity (0..1), and which choice it shows (0 = authored, n = __OPACITIES[n - 1]).
    float   m_fAuthoredOpacity = __OPACITIES[0];
    size_t  m_nOpacityChoice   = 0;

    // Sunny: its frames (declared first, so it outlives the sprite), the sprite, and its choices -
    // indexes into __OPACITIES (0 = opaque, the default) and __TINTS (0 = none, the default).
    SunLight :: Canvas :: TextureCanvas  m_SunnyFrames;
    SunLight :: Sprite :: Sprite         m_Sunny;
    size_t                               m_nSunnyOpacity = 0;
    size_t                               m_nSunnyTint    = 0;

    void ApplyTint( size_t nLayer )  {

        SunLight :: TileMap :: stLayer  layer {};

        if( !m_Renderer.GetLayer( __TINTED_LAYERS[nLayer], layer ) )
            return;

        layer.tint = ( m_nTintChoice[nLayer] == 0 ) ? m_Authored[nLayer] : __TINTS[m_nTintChoice[nLayer] - 1].color;
        m_Renderer.SetLayer( __TINTED_LAYERS[nLayer], layer );
    }

    float GroupOpacity( void ) const  {

        return ( m_nOpacityChoice == 0 ) ? m_fAuthoredOpacity : __OPACITIES[m_nOpacityChoice - 1];
    }

    void ApplyGroupOpacity( void )  {

        SunLight :: TileMap :: stLayer  layer {};

        if( !m_Renderer.GetLayer( __GROUP_LAYER, layer ) )
            return;

        layer.fOpacity = GroupOpacity();
        m_Renderer.SetLayer( __GROUP_LAYER, layer );
    }

    std :: string TintName( size_t nLayer ) const  {

        char                                buffer[__TINT_TEXT_SIZE];
        const SunLight :: Base :: stColor  &color = ( m_nTintChoice[nLayer] == 0 ) ? m_Authored[nLayer]
                                                                                   : __TINTS[m_nTintChoice[nLayer] - 1].color;
        const char                         *szName = ( m_nTintChoice[nLayer] == 0 ) ? "authored"
                                                                                    : __TINTS[m_nTintChoice[nLayer] - 1].szName;

        snprintf( buffer, sizeof( buffer ), "#%02X%02X%02X %s", color.nRed, color.nGreen, color.nBlue, szName );

        return buffer;
    }

    public :

    TintSample( TileMapRenderer& renderer ) : m_Renderer( renderer )  {

        for( size_t nLayer = 0; nLayer < __TINTED_LAYER_COUNT; nLayer++ )  {
            SunLight :: TileMap :: stLayer  layer {};

            m_Authored[nLayer] = m_Renderer.GetLayer( __TINTED_LAYERS[nLayer], layer ) ? layer.tint : __TINTS[0].color;

            if( strcmp( __TINTED_LAYERS[nLayer], __GROUP_LAYER ) == 0 )
                m_fAuthoredOpacity = layer.fOpacity;
        }
    }

    void NextTint( size_t nLayer )  {

        m_nTintChoice[nLayer] = ( m_nTintChoice[nLayer] + 1 ) % ( __TINT_COUNT + 1 );
        ApplyTint( nLayer );
    }

    // Load Sunny and register it: after Start(), which the texture needs.
    bool LoadSunny( void )  {

        if( !m_SunnyFrames.Load( __SUNNY_FILE ) )
            return false;

        m_SunnyFrames.SetTileSize( __SUNNY_SIZE );
        m_SunnyFrames.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
        m_SunnyFrames.SetDimension2D( SunLight :: Base :: stDimension2D { { __SUNNY_X, __SUNNY_Y }, { __SUNNY_SIZE, __SUNNY_SIZE } } );
        m_Sunny.AddTextureSequence( 0, &m_SunnyFrames, __SUNNY_DELAY );
        m_Sunny.SetActiveTextureSequence( 0 );
        m_Sunny.SetVisible( true );

        return m_Renderer.AddSprite( __SUNNY_LAYER_ID, m_Sunny );
    }

    void NextSunnyOpacity( void )  {

        m_nSunnyOpacity = ( m_nSunnyOpacity + 1 ) % __OPACITY_COUNT;
        m_Sunny.SetOpacity( __OPACITIES[m_nSunnyOpacity] );
    }

    void NextSunnyTint( void )  {

        m_nSunnyTint = ( m_nSunnyTint + 1 ) % __TINT_COUNT;
        m_Sunny.SetTint( __TINTS[m_nSunnyTint].color );
    }

    void NextGroupOpacity( void )  {

        m_nOpacityChoice = ( m_nOpacityChoice + 1 ) % ( __OPACITY_COUNT + 1 );
        ApplyGroupOpacity();
    }

    void Reset( void )  {

        for( size_t nLayer = 0; nLayer < __TINTED_LAYER_COUNT; nLayer++ )  {
            m_nTintChoice[nLayer] = 0;
            ApplyTint( nLayer );
        }

        m_nOpacityChoice = 0;
        ApplyGroupOpacity();

        m_nSunnyOpacity = 0;
        m_nSunnyTint    = 0;
        m_Sunny.SetOpacity( __OPACITIES[0] );
        m_Sunny.SetTint( __TINTS[0].color );
    }

    // SunLight :: TileMap :: ITileMapListener
    void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap ) override  {

        m_Renderer.DrawText( "[1] [2] [3] layer tints  [4] [5] group tint, opacity  [6] [7] sprite opacity, tint  [0] reset  [Esc] quit",
                             __HUD_X, __HUD_KEYS_Y, __FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
        m_Renderer.DrawText( "Layers through GetLayer / SetLayer, Sunny through Sprite::SetOpacity / SetTint. Labels never change.",
                             __HUD_X, __HUD_NOTE_Y, __FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );

        std :: string  strTints = "tile layer " + TintName( 0 ) + "   image layer " + TintName( 1 ) + "   object layer " + TintName( 2 );
        char           szOpacity[__TINT_TEXT_SIZE];

        snprintf( szOpacity, sizeof( szOpacity ), "%.2f", GroupOpacity() );

        std :: string  strGroup = "group " + TintName( 3 ) + ", opacity " + szOpacity +
                                  ( ( m_nOpacityChoice == 0 ) ? " authored" : "" );

        m_Renderer.DrawText( strTints.c_str(), __HUD_X, __HUD_STATE_Y, __FONT_SIZE, __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
        m_Renderer.DrawText( strGroup.c_str(), __HUD_X, __HUD_STATE_Y + __HUD_LINE_HEIGHT, __FONT_SIZE,
                             __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );

        const SunLight :: Base :: stColor  &tint = __TINTS[m_nSunnyTint].color;
        char                                szSunny[__TINT_TEXT_SIZE];

        snprintf( szSunny, sizeof( szSunny ), "sprite opacity %.2f, tint #%02X%02X%02X %s",
                  m_Sunny.GetOpacity(), tint.nRed, tint.nGreen, tint.nBlue, __TINTS[m_nSunnyTint].szName );
        m_Renderer.DrawText( szSunny, __HUD_X, __HUD_STATE_Y + __HUD_SPRITE_LINE * __HUD_LINE_HEIGHT, __FONT_SIZE,
                             __OPAQUE, __OPAQUE, __OPAQUE, __OPAQUE );
    }

    void OnStop( void ) override  {
    }
};

int main( int argc, char** argv )  {

    if( argc < 2 )  {
        fprintf( stderr, "Invalid command line arguments: tint_test <sample directory>\n" );
        return EXIT_FAILURE;
    }

    // Resources are loaded by name relative to the sample's own directory, so enter it first.
    std :: error_code  errorCode;

    std :: filesystem :: current_path( argv[1], errorCode );

    if( errorCode )  {
        fprintf( stderr, "Cannot enter the sample directory [%s]: %s\n", argv[1], errorCode.message().c_str() );
        return EXIT_FAILURE;
    }

    // The fonts every sample shares live in samples/shared/fonts. Mounting anything turns off the
    // automatic mount of the working directory, so this sample's own folder is mounted too.
    SunLight :: FileSystem :: IFileSystem  &fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

    if( !fileSystem.Mount( ".", "/", true ) || !fileSystem.Mount( "../shared", "/shared", true ) )  {
        fprintf( stderr, "Cannot mount the sample resources\n" );
        return EXIT_FAILURE;
    }

    RendererConfig  config;

    config.fWidth     = 1260.0f;
    config.fHeight    = 920.0f;
    config.strTitle   = "Layer tint (I8 of the master plan)";
    config.nTargetFps = 60;
    config.viewport.emplace();
    config.viewport -> pos.x        = __VIEWPORT_POS_X;
    config.viewport -> pos.y        = __VIEWPORT_POS_Y;
    config.viewport -> size.nWidth  = __VIEWPORT_WIDTH;
    config.viewport -> size.nHeight = __VIEWPORT_HEIGHT;

    std :: unique_ptr<TileMapRenderer>  pRenderer = TileMapRenderer :: Create( config, nullptr );

    if( pRenderer == nullptr )  {
        fprintf( stderr, "Cannot create the renderer\n" );
        return EXIT_FAILURE;
    }

    if( !pRenderer -> Start() )  {
        fprintf( stderr, "Cannot start the renderer\n" );
        return EXIT_FAILURE;
    }

    // One font for the HUD (SetFont) and the map's labels (RegisterFont, the family "Sans").
    std :: shared_ptr<SunLight :: Font :: IFont>  pFont = pRenderer -> LoadFont( __FONT_FILE );

    if( !pRenderer -> SetFont( pFont ) || !pRenderer -> RegisterFont( __FONT_NAME, pFont, false, false ) )
        fprintf( stderr, "Cannot load the font [%s]: using the built-in one\n", __FONT_FILE );

    if( !pRenderer -> LoadMap( __MAP_FILE, ITM :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot load %s\n", __MAP_FILE );
        return EXIT_FAILURE;
    }

    TintSample  sample( *pRenderer );

    if( !sample.LoadSunny() )
        fprintf( stderr, "Cannot load Sunny [%s]\n", __SUNNY_FILE );

    // Once per key press, not on every frame a key is held: each press moves to the next choice.
    pRenderer -> GetControllerManager().SetInputDispatchMode( SunLight :: Input :: INPUT_DISPATCH_MODE_REACTIVE );

    for( size_t nLayer = 0; nLayer < __TINTED_LAYER_COUNT; nLayer++ )
        pRenderer -> SetUserKeyEventHandler( __TINT_KEYS[nLayer],
                                             [&sample, nLayer]( SunLight :: Input :: ControllerType, int ) { sample.NextTint( nLayer ); } );

    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_FIVE,
                                         [&sample]( SunLight :: Input :: ControllerType, int ) { sample.NextGroupOpacity(); } );
    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_SIX,
                                         [&sample]( SunLight :: Input :: ControllerType, int ) { sample.NextSunnyOpacity(); } );
    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_SEVEN,
                                         [&sample]( SunLight :: Input :: ControllerType, int ) { sample.NextSunnyTint(); } );
    pRenderer -> SetUserKeyEventHandler( SunLight :: Input :: KEY_ZERO,
                                         [&sample]( SunLight :: Input :: ControllerType, int ) { sample.Reset(); } );

    pRenderer -> AddTileMapListener( &sample );

    printf( "Keys 1 to 4 cycle the layer tints, 5 the group's opacity, 6 and 7 Sunny's opacity and tint, 0 resets, Esc quits.\n" );

    pRenderer -> Run();
    pRenderer -> RemoveTileMapListener( &sample );
    pRenderer -> Stop();

    return EXIT_SUCCESS;
}
