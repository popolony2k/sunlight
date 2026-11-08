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

/*
 * Worlds sample (I19c of the master plan): two WORLDS - two different maps, each with its own Sunny -
 * shown at once.
 *
 *   - the LEFT half is the renderer's default view: it always shows the DEFAULT world (the cloud map,
 *     loaded with the renderer's own LoadMap);
 *   - the RIGHT half is an extra view showing world B (the isometric map), made with CreateWorld and
 *     filled with IWorld::LoadMap / IWorld::AddSprite;
 *   - an OVERLAY over the left half shows world B too, at opacity 0: key 2 cross-fades the left half
 *     from one world to the other (IView::SetOpacity, I19a).
 *
 * Each Sunny is a world-space sprite of its own world: WASD walks the cloud one, the arrows the
 * isometric one; neither ever appears in the other world. Key 1 makes the right view show the default
 * world or world B (IView::SetWorld); key 3 removes world B (RemoveWorld: its views go back to the
 * default world, its Sunny is detached) and creates it again.
 *
 * Like the other samples it takes its own directory as argv[1] and reads its resources relative to it:
 *
 *     ./build/samples/worlds/worlds_test samples/worlds/
 */

#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <cstdio>

#define __DISPLAY_W                 1260
#define __DISPLAY_H                 920
#define __FRAMES_PER_SECOND         60
#define __CLOUD_MAP                 "resources/clouds/test.tmx"
#define __ISO_MAP                   "resources/iso/isometric.tmx"
#define __SUNNY_SPRITE              "resources/sprites/sunny_idle_down.png"
#define __SUNNY_SIZE                32
#define __SUNNY_STEP                4
#define __SUNNY_ANIMATION_DELAY     100
#define __CLOUD_SUNNY_LAYER_ID      8       // test.tmx's "birb": its last layer, so Sunny is on top
#define __ISO_SUNNY_LAYER_ID        3       // isometric.tmx's "sprites" layer
#define __CLOUD_SUNNY_START         128
#define __ISO_SUNNY_START_X         208     // inside the isometric diamond
#define __ISO_SUNNY_START_Y         96
#define __LEFT_X                    10      // the left half: the default view
#define __HALF_Y                    10
#define __LEFT_W                    610
#define __HALF_H                    900
#define __RIGHT_X                   640     // the right half: an extra view
#define __RIGHT_W                   610
#define __RIGHT_H                   620
#define __FADE_FRAMES               60      // a cross-fade: one second at __FRAMES_PER_SECOND
#define __OPACITY_SHOWN             1.0f
#define __OPACITY_HIDDEN            0.0f
#define __HUD_X                     642     // the key list: under the right view, clear of every view
#define __HUD_Y                     650
#define __HUD_LINE_HEIGHT           21
#define __HUD_FONT_SIZE             20      // twice raylib's built-in 10 px font: a whole multiple stays crisp
#define __HUD_TEXT_SIZE             64
#define __HUD_CHANNEL               255     // the labels: opaque white
#define __NO_FRAME                  -2      // Once: no press seen yet

namespace  {

    typedef std :: shared_ptr<SunLight :: TileMap :: IWorld>  WorldHandle;
    typedef std :: shared_ptr<SunLight :: TileMap :: IView>   ViewHandle;

    SunLight :: Base :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SunLight :: Base :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // One Sunny: a world-space sprite with one animated canvas. (The canvas is declared first so it is
    // destroyed after the sprite that uses it.)
    struct Sunny  {

        SunLight :: Canvas :: TextureCanvas  canvas;
        SunLight :: Sprite :: Sprite         sprite;

        bool Load( int nX, int nY )  {
            if( !canvas.Load( __SUNNY_SPRITE ) )
                return false;

            canvas.SetTileSize( __SUNNY_SIZE );
            canvas.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { __SUNNY_SIZE, __SUNNY_SIZE } } );
            sprite.AddTextureSequence( 0, &canvas, __SUNNY_ANIMATION_DELAY );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
            sprite.SetWorldSpace( true );
            sprite.GetDimension2D().pos.x = nX;
            sprite.GetDimension2D().pos.y = nY;

            return true;
        }

        void Move( int nDX, int nDY )  {
            sprite.GetDimension2D().pos.x += nDX;
            sprite.GetDimension2D().pos.y += nDY;
        }
    };

    /*
     * Every frame: the cross-fade (the overlay's opacity one step towards its target) and the key list,
     * drawn in the free area under the right view, which no view covers. Counts frames too, which Once
     * (below) tells a held key from a new press by.
     */
    struct Frame : public SunLight :: TileMap :: ITileMapListener  {

        SunLight :: Renderer :: TileMapRenderer  &m_Renderer;
        ViewHandle                               m_pRight;
        ViewHandle                               m_pOverlay;
        float                                    m_fFadeTarget = __OPACITY_HIDDEN;
        long                                     m_nFrame      = 0;

        explicit Frame( SunLight :: Renderer :: TileMapRenderer& renderer ) : m_Renderer( renderer )  {
        }

        void Line( int nLine, const char* szText )  {
            m_Renderer.DrawText( szText, __HUD_X, __HUD_Y + ( nLine * __HUD_LINE_HEIGHT ), __HUD_FONT_SIZE,
                                 __HUD_CHANNEL, __HUD_CHANNEL, __HUD_CHANNEL, __HUD_CHANNEL );
        }

        void OnUpdate( SunLight :: TileMap :: ITileMap& )  override  {
            const float  fStep    = ( __OPACITY_SHOWN - __OPACITY_HIDDEN ) / ( float ) __FADE_FRAMES;
            float        fOpacity = m_pOverlay -> GetOpacity();

            if( fOpacity < m_fFadeTarget )
                m_pOverlay -> SetOpacity( std :: min( fOpacity + fStep, m_fFadeTarget ) );
            else if( fOpacity > m_fFadeTarget )
                m_pOverlay -> SetOpacity( std :: max( fOpacity - fStep, m_fFadeTarget ) );

            static const char  *s_Keys[] = {
                "KEYS",
                "WASD    walk cloud Sunny",
                "Arrows  walk iso Sunny",
                "1  right view: world A/B",
                "2  cross-fade left A/B",
                "3  remove / recreate B",
            };
            const int          nKeys = ( int ) ( sizeof( s_Keys ) / sizeof( s_Keys[0] ) );
            char               szText[__HUD_TEXT_SIZE];

            for( int nLine = 0; nLine < nKeys; nLine++ )
                Line( nLine, s_Keys[nLine] );

            // A blank line, then the state: how many worlds, which one the right view shows, the fade.
            snprintf( szText, sizeof( szText ), "worlds %d   right: world %d", m_Renderer.GetWorldCount(),
                      m_pRight -> GetWorld() -> GetId() );
            Line( nKeys + 1, szText );
            snprintf( szText, sizeof( szText ), "left fade to B  %.2f", m_pOverlay -> GetOpacity() );
            Line( nKeys + 2, szText );

            m_nFrame++;
        }

        void OnStop( void )  override  {
        }
    };
}

int main( int argc, char** argv )  {

    if( argc < 2 )  {
        fprintf( stderr, "usage: %s <this sample's directory> (e.g. samples/worlds/)\n", argv[0] );
        return EXIT_FAILURE;
    }

    std :: error_code  errorCode;

    std :: filesystem :: current_path( argv[1], errorCode );

    if( errorCode )  {
        fprintf( stderr, "Cannot enter the sample directory [%s]: %s\n", argv[1], errorCode.message().c_str() );
        return EXIT_FAILURE;
    }

    SunLight :: Renderer :: TileMapRenderer  renderer( __DISPLAY_W, __DISPLAY_H, "Worlds test", __FRAMES_PER_SECOND );

    renderer.SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );
    renderer.SetDrawFPS( true );
    renderer.GetViewport().SetDimension2D( Rect( __LEFT_X, __HALF_Y, __LEFT_W, __HALF_H ) );

    // The DEFAULT world: the renderer's own single-map calls fill it, and the default view (left) shows it.
    if( !renderer.Start() || !renderer.LoadMap( __CLOUD_MAP, SunLight :: TileMap :: ITileMap :: MapAlignment :: MAP_ALIGNMENT_TOP_LEFT ) )  {
        fprintf( stderr, "Cannot start / load [%s] - is [%s] this sample's own directory (samples/worlds/)?\n", __CLOUD_MAP, argv[1] );
        return EXIT_FAILURE;
    }

    renderer.GetDefaultView().FitToMap();

    Sunny  cloudSunny, isoSunny;

    if( !cloudSunny.Load( __CLOUD_SUNNY_START, __CLOUD_SUNNY_START ) || !isoSunny.Load( __ISO_SUNNY_START_X, __ISO_SUNNY_START_Y ) )  {
        fprintf( stderr, "Cannot load the sprite [%s]\n", __SUNNY_SPRITE );
        return EXIT_FAILURE;
    }

    if( !renderer.AddSprite( __CLOUD_SUNNY_LAYER_ID, cloudSunny.sprite ) )  {
        fprintf( stderr, "Cannot add Sunny to the cloud map's layer %d\n", __CLOUD_SUNNY_LAYER_ID );
        return EXIT_FAILURE;
    }

    // The right half, and the overlay over the left half: views that will show world B.
    Frame  frame( renderer );

    frame.m_pRight   = renderer.CreateView( Rect( __RIGHT_X, __HALF_Y, __RIGHT_W, __RIGHT_H ) );
    frame.m_pOverlay = renderer.CreateView( Rect( __LEFT_X, __HALF_Y, __LEFT_W, __HALF_H ) );
    frame.m_pOverlay -> SetOpacity( __OPACITY_HIDDEN );
    renderer.AddTileMapListener( &frame );

    // World B: a world of its own - its map, its Sunny - shown by the right view and the overlay.
    WorldHandle  pWorldB;

    auto  createWorldB = [&]()  {
        pWorldB = renderer.CreateWorld();

        if( !pWorldB -> LoadMap( __ISO_MAP ) || !pWorldB -> AddSprite( __ISO_SUNNY_LAYER_ID, isoSunny.sprite ) )  {
            fprintf( stderr, "Cannot fill world B from [%s]\n", __ISO_MAP );
            return false;
        }

        for( ViewHandle pView : { frame.m_pRight, frame.m_pOverlay } )  {
            pView -> SetWorld( pWorldB );
            pView -> FitToMap();
        }

        return true;
    };

    if( !createWorldB() )
        return EXIT_FAILURE;

    typedef SunLight :: Input :: ControllerType  Controller;

    auto  bind = [&renderer]( SunLight :: Input :: KeyboardKey key, std :: function<void( void )> action )  {
        renderer.SetUserKeyEventHandler( key, [action]( Controller, int ) { action(); } );
    };

    // Keys are dispatched in ACTIVE mode: a handler runs on EVERY frame its key is down, which walking wants
    // and a toggle must not. Once() runs its action only when the key was not down on the frame before.
    auto  Once = [&frame]( std :: function<void( void )> action )  {
        std :: shared_ptr<long>  pLastFrame = std :: make_shared<long>( __NO_FRAME );

        return [&frame, action, pLastFrame]()  {
            if( frame.m_nFrame != *pLastFrame + 1 )
                action();

            *pLastFrame = frame.m_nFrame;
        };
    };

    // WASD: the cloud world's Sunny. Arrows: the isometric world's. Positions are each world's own map pixels.
    bind( SunLight :: Input :: KEY_W,     [&]() { cloudSunny.Move( 0, -__SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_S,     [&]() { cloudSunny.Move( 0,  __SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_A,     [&]() { cloudSunny.Move( -__SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_D,     [&]() { cloudSunny.Move(  __SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_UP,    [&]() { isoSunny.Move( 0, -__SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_DOWN,  [&]() { isoSunny.Move( 0,  __SUNNY_STEP ); } );
    bind( SunLight :: Input :: KEY_LEFT,  [&]() { isoSunny.Move( -__SUNNY_STEP, 0 ); } );
    bind( SunLight :: Input :: KEY_RIGHT, [&]() { isoSunny.Move(  __SUNNY_STEP, 0 ); } );

    // 1: the right view shows the default world or world B (only an extra view can switch worlds).
    bind( SunLight :: Input :: KEY_ONE,   Once( [&]() {
        bool  bShowsB = ( frame.m_pRight -> GetWorld() == pWorldB ) && !pWorldB -> IsRemoved();

        frame.m_pRight -> SetWorld( bShowsB ? renderer.GetDefaultWorld() : pWorldB );
        frame.m_pRight -> FitToMap();
    } ) );

    // 2: cross-fade the left half from the default world to world B (the overlay fading in), and back.
    bind( SunLight :: Input :: KEY_TWO,   Once( [&]() {
        frame.m_fFadeTarget = ( frame.m_fFadeTarget == __OPACITY_HIDDEN ) ? __OPACITY_SHOWN : __OPACITY_HIDDEN;
    } ) );

    // 3: remove world B - the right view and the overlay go back to the default world, its Sunny is
    // detached (still the sample's, not drawn anywhere) - or create it again and show it.
    bind( SunLight :: Input :: KEY_THREE, Once( [&]() {
        if( !pWorldB -> IsRemoved() )  {
            renderer.RemoveWorld( pWorldB );
            frame.m_pRight -> FitToMap();
            frame.m_pOverlay -> FitToMap();
        }
        else
            createWorldB();
    } ) );

    renderer.Run();
    renderer.RemoveTileMapListener( &frame );
    renderer.Stop();

    return EXIT_SUCCESS;
}
