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
 * I2 of the master plan: blend modes. What the GPU does with each mode is measured, not tested here
 * (see RaylibEngine::SetBlendMode); these pin who asks the engine for which mode, when, and in what
 * order around the draws: a game's BeginBlendMode/EndBlendMode, the end of the frame taking a
 * forgotten mode back to normal before the screen fade, and a sprite's own mode, switched for its
 * own draw only, in every view pass. A frame with no mode makes no SetBlendMode call at all.
 */

#include <doctest/doctest.h>
#include <functional>
#include <memory>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "backends/null/nullengine.h"
#include "mock_engine.h"
#include "mock_window.h"
#include "mock_filesystem.h"

using namespace SunLight :: Renderer;
typedef MockEngine :: Event  Event;
namespace SB = SunLight :: Base;

namespace  {

    SB :: stDimension2D Rect( int nX, int nY, int nW, int nH )  {

        SB :: stDimension2D  rect {};

        rect.pos.x = nX;  rect.pos.y = nY;  rect.size.nWidth = nW;  rect.size.nHeight = nH;

        return rect;
    }

    // What the game does in its OnUpdate, during the frame.
    struct GameDraw : public SunLight :: TileMap :: ITileMapListener  {
        std :: function<void( void )>  draw;

        void OnUpdate( SunLight :: TileMap :: ITileMap& ) override  { if( draw ) draw(); }
        void OnStop( void ) override                               {}
    };

    // A started renderer with no map, and a game drawing in its OnUpdate.
    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;
        GameDraw                            game;

        Scene( void )  {

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 400, 300 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );

            pRenderer -> AddTileMapListener( &game );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        void RunFrames( int nFrames )  {
            engine().events.clear();
            engine().nSetBlendModeCalls                  = 0;
            windowFixture.window.nFramesUntilShouldClose = nFrames;
            pRenderer -> Run();
        }

        // The frame's events with only the given kinds kept, as "kind:detail" strings: B<mode> for
        // a BLEND, T<handle> for a TILE, F for a FILL.
        std :: vector<std :: string> Trace( void )  {
            std :: vector<std :: string>  trace;

            for( const Event& evt : engine().events )  {
                if( evt.kind == Event :: BLEND )
                    trace.push_back( "B" + std :: to_string( ( int ) evt.blendMode ) );
                else if( evt.kind == Event :: TILE )
                    trace.push_back( "T" + std :: to_string( ( size_t ) evt.handle ) );
                else if( evt.kind == Event :: FILL )
                    trace.push_back( "F" );
            }

            return trace;
        }
    };

    // A one-texture sprite on a layer id, recognised by its texture handle.
    struct SpriteRig  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
        Scene                                &m_Scene;
        int                                  m_nLayerId;

        SpriteRig( Scene& scene, int nLayerId, void* handle, int nX = 20, int nY = 20 )
            : m_Scene( scene ), m_nLayerId( nLayerId )  {

            scene.engine().hLoadTextureResult = handle;
            scene.engine().nLoadTextureWidth  = 16;
            scene.engine().nLoadTextureHeight = 16;

            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetDimension2D( SB :: stDimension2D { { nX, nY }, { 16, 16 } } );

            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( nLayerId, sprite ) == true );
        }

        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( m_nLayerId, sprite );
        }
    };

    void* const  H1 = ( void* ) 1;
    void* const  H2 = ( void* ) 2;

    // The mode numbers, as Trace() spells them.
    const std :: string  NORMAL   = "B" + std :: to_string( ( int ) SB :: BLEND_MODE_NORMAL );
    const std :: string  ADD      = "B" + std :: to_string( ( int ) SB :: BLEND_MODE_ADD );
    const std :: string  MULTIPLY = "B" + std :: to_string( ( int ) SB :: BLEND_MODE_MULTIPLY );
    const std :: string  SUBTRACT = "B" + std :: to_string( ( int ) SB :: BLEND_MODE_SUBTRACT );
}

TEST_SUITE( "renderer/blend_modes" )  {

    TEST_CASE( "BeginBlendMode and EndBlendMode put the mode around the game's own draws" )  {

        Scene  scene;

        scene.game.draw = [&scene]( void )  {
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_ADD );
            scene.pRenderer -> DrawFilledRectangle( 0, 0, 8, 8, 255, 255, 255, 255 );
            scene.pRenderer -> EndBlendMode();
        };

        scene.RunFrames( 1 );

        CHECK( scene.Trace() == std :: vector<std :: string> { ADD, "F", NORMAL } );
        CHECK( scene.engine().currentBlendMode == SB :: BLEND_MODE_NORMAL );
    }

    TEST_CASE( "A second BeginBlendMode replaces the first: the calls do not nest" )  {

        Scene  scene;

        scene.game.draw = [&scene]( void )  {
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_ADD );
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_SUBTRACT );
            scene.pRenderer -> DrawFilledRectangle( 0, 0, 8, 8, 0, 0, 0, 255 );
            scene.pRenderer -> EndBlendMode();
        };

        scene.RunFrames( 1 );

        CHECK( scene.Trace() == std :: vector<std :: string> { ADD, SUBTRACT, "F", NORMAL } );
    }

    TEST_CASE( "A missing EndBlendMode ends with the frame, before the screen fade, and the next frame starts in normal" )  {

        Scene  scene;
        int    nFrame = 0;

        scene.pRenderer -> SetScreenFade( 0.5f );
        scene.game.draw = [&scene, &nFrame]( void )  {
            nFrame++;
            CHECK( scene.engine().currentBlendMode == SB :: BLEND_MODE_NORMAL );   // each frame starts in normal
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_MULTIPLY );         // and no EndBlendMode
        };

        scene.RunFrames( 2 );

        // Per frame: the game's multiply, the renderer's normal, then the fade (a FILL) drawn in normal.
        CHECK( nFrame == 2 );
        CHECK( scene.Trace() == std :: vector<std :: string> { MULTIPLY, NORMAL, "F", MULTIPLY, NORMAL, "F" } );
    }

    TEST_CASE( "A frame with no mode anywhere makes no SetBlendMode call" )  {

        Scene      scene;
        SpriteRig  plain( scene, 2, H1 );

        scene.RunFrames( 3 );

        CHECK( scene.engine().nSetBlendModeCalls == 0 );
        CHECK( scene.Trace() == std :: vector<std :: string> { "T1", "T1", "T1" } );
    }

    TEST_CASE( "A sprite with a mode switches to it for its own draw, then back; a sprite with none draws as it is" )  {

        Scene      scene;
        SpriteRig  lit( scene, 2, H1 );
        SpriteRig  plain( scene, 3, H2 );

        lit.sprite.SetBlendMode( SB :: BLEND_MODE_ADD );

        scene.RunFrames( 1 );

        CHECK( scene.Trace() == std :: vector<std :: string> { ADD, "T1", NORMAL, "T2" } );
    }

    TEST_CASE( "Inside a game's BeginBlendMode a sprite's own mode goes back to the game's mode, not to normal" )  {

        Scene      scene;
        SpriteRig  shadow( scene, 2, H1 );

        shadow.sprite.SetBlendMode( SB :: BLEND_MODE_MULTIPLY );
        scene.game.draw = [&scene, &shadow]( void )  {
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_SUBTRACT );
            shadow.sprite.Draw();
            scene.pRenderer -> EndBlendMode();
        };

        scene.RunFrames( 1 );

        // The frame's own draw of the sprite, then the game's draw of it inside subtract.
        CHECK( scene.Trace() == std :: vector<std :: string> { MULTIPLY, "T1", NORMAL,
                                                               SUBTRACT, MULTIPLY, "T1", SUBTRACT, NORMAL } );
    }

    TEST_CASE( "A sprite whose mode is already in effect switches nothing" )  {

        Scene      scene;
        SpriteRig  glow( scene, 2, H1 );

        glow.sprite.SetBlendMode( SB :: BLEND_MODE_ADD );
        scene.game.draw = [&scene, &glow]( void )  {
            scene.pRenderer -> BeginBlendMode( SB :: BLEND_MODE_ADD );
            glow.sprite.Draw();
            scene.pRenderer -> EndBlendMode();
        };

        scene.RunFrames( 1 );

        CHECK( scene.Trace() == std :: vector<std :: string> { ADD, "T1", NORMAL, ADD, "T1", NORMAL } );
    }

    TEST_CASE( "A sprite with a mode switches in every view pass that draws it" )  {

        Scene      scene;
        SpriteRig  lit( scene, 2, H1 );

        lit.sprite.SetBlendMode( SB :: BLEND_MODE_ADD );
        scene.pRenderer -> CreateView( Rect( 500, 10, 400, 300 ) );

        scene.RunFrames( 1 );

        // The default view's pass, then the extra one's, which starts by clearing its rectangle (the FILL).
        CHECK( scene.Trace() == std :: vector<std :: string> { ADD, "T1", NORMAL, "F", ADD, "T1", NORMAL } );
    }

    TEST_CASE( "A sprite with a mode that is off screen switches nothing" )  {

        Scene      scene;
        SpriteRig  far( scene, 2, H1, 5000, 5000 );

        far.sprite.SetBlendMode( SB :: BLEND_MODE_ADD );

        scene.RunFrames( 1 );

        CHECK( scene.engine().nSetBlendModeCalls == 0 );
        CHECK( scene.Trace().empty() );
    }

    TEST_CASE( "Sprite: no mode by default; SetBlendMode, ClearBlendMode, and Unload keeps the mode" )  {

        MockEngineFixture       engineFixture;
        MemoryFileSystemFixture fsFixture;
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;

        CHECK( sprite.HasBlendMode() == false );
        CHECK( sprite.GetBlendMode() == SB :: BLEND_MODE_NORMAL );

        sprite.SetBlendMode( SB :: BLEND_MODE_SUBTRACT );
        CHECK( sprite.HasBlendMode() == true );
        CHECK( sprite.GetBlendMode() == SB :: BLEND_MODE_SUBTRACT );

        REQUIRE( canvas.Load( "sprite.png" ) == true );
        sprite.AddTextureSequence( 0, &canvas, 0 );
        sprite.Unload();
        CHECK( sprite.HasBlendMode() == true );
        CHECK( sprite.GetBlendMode() == SB :: BLEND_MODE_SUBTRACT );

        sprite.ClearBlendMode();
        CHECK( sprite.HasBlendMode() == false );
        CHECK( sprite.GetBlendMode() == SB :: BLEND_MODE_NORMAL );
    }

    TEST_CASE( "NullEngine keeps the mode it was given; BeginRenderTarget starts in normal" )  {

        SunLight :: Engines :: Null :: NullEngine  engine;
        SB :: TextureHandle                        hTarget = engine.LoadRenderTarget( 8, 8 );

        CHECK( engine.GetBlendMode() == SB :: BLEND_MODE_NORMAL );

        engine.SetBlendMode( SB :: BLEND_MODE_MULTIPLY );
        CHECK( engine.GetBlendMode() == SB :: BLEND_MODE_MULTIPLY );

        engine.BeginRenderTarget( hTarget );
        CHECK( engine.GetBlendMode() == SB :: BLEND_MODE_NORMAL );

        engine.SetBlendMode( SB :: BLEND_MODE_ADD );
        engine.EndRenderTarget();
        CHECK( engine.GetBlendMode() == SB :: BLEND_MODE_ADD );     // only Begin resets it

        engine.UnloadRenderTarget( hTarget );
    }
}
