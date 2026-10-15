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
 * S1d of the master plan: SpriteRegistry on its own - the sprites per layer, the parented set and
 * the once-per-frame update across view passes - with real sprites, the mock engine and the mock
 * clock, and no TileMapRenderer. The sprites here have no parent, so each canvas draws in its
 * sprite's own viewport. test_viewpasses, test_spritezorder and test_spriteregistration still pin
 * the same behaviour through the renderer.
 */

#include <doctest/doctest.h>
#include <memory>
#include <vector>
#include "renderer/sprite/spriteregistry.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
#include "mock_engine.h"
#include "mock_clock.h"

using SunLight :: Renderer :: SpriteRegistry;
typedef MockEngine :: Event  Event;

namespace  {

    const int  FRAME_DELAY_MS = 10;     // each canvas of a two-canvas sequence shows for 10 ms
    const int  FRAME_STEP_MS  = 20;     // the clock moves 20 ms per frame: every Advance moves to the other canvas
    const int  ON_SCREEN      = 20;     // a canvas position inside the 640 x 480 viewport
    const int  OFF_SCREEN     = 5000;   // and one far outside it

    void* const  H1 = ( void* ) 0x1001;
    void* const  H2 = ( void* ) 0x1002;
    void* const  H3 = ( void* ) 0x1003;

    // A visible sprite in a 640 x 480 viewport of its own, with one sequence of one or two 16 x 16 canvases,
    // each loaded with its own texture handle.
    struct TestSprite  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  first;
        SunLight :: Canvas :: TextureCanvas  second;

        TestSprite( MockEngine &engine, void *hFirst, void *hSecond = nullptr, int nZOrder = 0 )  {
            sprite.GetViewport().SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 640, 480 } } );

            engine.nLoadTextureWidth  = 16;
            engine.nLoadTextureHeight = 16;

            Load( engine, first, hFirst );

            if( hSecond != nullptr )
                Load( engine, second, hSecond );

            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
            sprite.SetZOrder( nZOrder );
        }

        void Load( MockEngine &engine, SunLight :: Canvas :: TextureCanvas &canvas, void *hTexture )  {
            engine.hLoadTextureResult = hTexture;
            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { ON_SCREEN, ON_SCREEN }, { 16, 16 } } );
            sprite.AddTextureSequence( 0, &canvas, FRAME_DELAY_MS );
        }

        // Moves both canvases on or off screen.
        void Place( int nPos )  {
            first.GetDimension2D().pos  = { nPos, nPos };
            second.GetDimension2D().pos = { nPos, nPos };
        }
    };

    // The texture handle of every TILE event, in draw order.
    std :: vector<void*> Drawn( MockEngine &engine )  {
        std :: vector<void*>  handles;

        for( const Event &evt : engine.events )
            if( evt.kind == Event :: TILE )
                handles.push_back( evt.handle );

        return handles;
    }
}

TEST_SUITE( "renderer/SpriteRegistry" )  {

    TEST_CASE( "Add, Contains and Remove, per layer; a sprite is on a layer once" )  {

        MockEngineFixture  fixture;
        TestSprite         a( fixture.engine, H1 );
        TestSprite         b( fixture.engine, H2 );
        SpriteRegistry     registry;

        CHECK( registry.Add( 1, &a.sprite ) == true );
        CHECK( registry.Add( 1, &a.sprite ) == false );      // already there
        CHECK( registry.Add( 2, &a.sprite ) == true );       // another layer is another list
        CHECK( registry.Add( 1, &b.sprite ) == true );

        CHECK( registry.Contains( 1, &a.sprite ) );
        CHECK( registry.Contains( 2, &a.sprite ) );
        CHECK_FALSE( registry.Contains( 2, &b.sprite ) );
        CHECK_FALSE( registry.Contains( 7, &a.sprite ) );

        CHECK( registry.Remove( 1, &a.sprite ) == true );
        CHECK( registry.Remove( 1, &a.sprite ) == false );
        CHECK_FALSE( registry.Contains( 1, &a.sprite ) );
        CHECK( registry.Contains( 2, &a.sprite ) );
        CHECK( registry.Contains( 1, &b.sprite ) );
    }

    TEST_CASE( "A layer's list, once opened, stays open emptied; LayerIds are ascending" )  {

        MockEngineFixture  fixture;
        TestSprite         a( fixture.engine, H1 );
        SpriteRegistry     registry;

        CHECK( registry.Empty() );

        registry.OpenLayer( 9 );                             // opened with no sprite: not empty any more
        CHECK_FALSE( registry.Empty() );

        registry.Add( 3, &a.sprite );
        registry.Add( 1, &a.sprite );
        registry.Remove( 3, &a.sprite );                     // emptied, still open

        CHECK( registry.LayerIds() == std :: vector<int> { 1, 3, 9 } );
    }

    TEST_CASE( "Forget takes a parented sprite off every layer and stops tracking it; anything else is ignored" )  {

        MockEngineFixture  fixture;
        TestSprite         a( fixture.engine, H1 );
        TestSprite         b( fixture.engine, H2 );
        SpriteRegistry     registry;
        std :: vector<int> layerIds;

        registry.Add( 3, &a.sprite );
        registry.Add( 1, &a.sprite );
        registry.MarkParented( &a.sprite );
        registry.Add( 1, &b.sprite );                        // on a layer, but never marked parented

        CHECK( registry.ParentedSprites() == std :: set<SunLight :: Sprite :: Sprite*> { &a.sprite } );

        CHECK( registry.Forget( &a.first, layerIds ) == nullptr );     // a canvas, not a parented sprite
        CHECK( layerIds.empty() );
        CHECK( registry.Forget( &b.sprite, layerIds ) == nullptr );    // not parented: left where it is
        CHECK( registry.Contains( 1, &b.sprite ) );

        CHECK( registry.Forget( &a.sprite, layerIds ) == &a.sprite );
        CHECK( layerIds == std :: vector<int> { 1, 3 } );
        CHECK_FALSE( registry.Contains( 1, &a.sprite ) );
        CHECK_FALSE( registry.Contains( 3, &a.sprite ) );
        CHECK( registry.ParentedSprites().empty() );

        registry.MarkParented( &b.sprite );
        registry.ClearParented();
        CHECK( registry.ParentedSprites().empty() );
    }

    TEST_CASE( "A layer's sprites draw in ascending z-order; equal z-orders keep the order they were added in" )  {

        MockEngineFixture  fixture;
        TestSprite         high( fixture.engine, H1, nullptr, 5 );
        TestSprite         low( fixture.engine, H2, nullptr, 1 );
        TestSprite         lowToo( fixture.engine, H3, nullptr, 1 );
        SpriteRegistry     registry;

        registry.Add( 1, &high.sprite );
        registry.Add( 1, &low.sprite );
        registry.Add( 1, &lowToo.sprite );

        registry.UpdateLayer( 1, false );
        CHECK( Drawn( fixture.engine ) == std :: vector<void*> { H2, H3, H1 } );

        fixture.engine.events.clear();
        registry.UpdateLayer( 7, false );                    // no such layer: nothing
        CHECK( fixture.engine.events.empty() );
    }

    TEST_CASE( "Across several view passes a sprite advances once per frame: every pass draws the frame a single view draws" )  {

        // One canvas with its own tile animation (a 64 px wide texture, 16 px tiles, circular): every
        // Advance steps it to the next tile, so each extra advance in a frame would show in the source x.
        const int  nFrames = 6;
        const int  nPasses = 3;

        auto  run = [&]( bool bMultiView )  {
            MockEngineFixture                     fixture;
            SunLight :: Sprite :: Sprite          sprite;
            SunLight :: Canvas :: TextureCanvas   canvas;
            SpriteRegistry                        registry;
            std :: vector<std :: vector<float>>   frames;

            fixture.engine.hLoadTextureResult = H1;
            fixture.engine.nLoadTextureWidth  = 64;
            fixture.engine.nLoadTextureHeight = 16;

            sprite.GetViewport().SetDimension2D( SunLight :: Base :: stDimension2D { { 0, 0 }, { 640, 480 } } );
            REQUIRE( canvas.Load( "sprite.png" ) == true );
            canvas.SetTileSize( 16 );
            canvas.SetAnimationMode( SunLight :: Canvas :: AnimationMode :: TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR );
            canvas.SetDimension2D( SunLight :: Base :: stDimension2D { { ON_SCREEN, ON_SCREEN }, { 16, 16 } } );
            sprite.AddTextureSequence( 0, &canvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );
            registry.Add( 1, &sprite );

            for( int nFrame = 0; nFrame < nFrames; nFrame++ )  {
                fixture.engine.events.clear();

                if( bMultiView )  {
                    registry.BeginFrame();

                    for( int nPass = 0; nPass < nPasses; nPass++ )
                        registry.UpdateLayer( 1, true );

                    registry.EndFrame();
                }
                else
                    registry.UpdateLayer( 1, false );

                std :: vector<float>  sources;

                for( const Event &evt : fixture.engine.events )
                    if( evt.kind == Event :: TILE )
                        sources.push_back( evt.srcX );

                frames.push_back( sources );
            }

            return frames;
        };

        std :: vector<std :: vector<float>>  single = run( false );
        std :: vector<std :: vector<float>>  multi  = run( true );

        CHECK( single[0] != single[1] );                     // the animation really moves (not vacuous)

        for( int nFrame = 0; nFrame < nFrames; nFrame++ )  {
            INFO( "frame " << nFrame );
            REQUIRE( single[nFrame].size() == 1 );
            CHECK( multi[nFrame] == std :: vector<float>( nPasses, single[nFrame][0] ) );
        }
    }

    TEST_CASE( "A sprite on screen in no pass is advanced at the frame's end; one on screen in a later pass, in that pass only" )  {

        MockEngineFixture  fixture;
        MockClockFixture   clockFixture;
        TestSprite         rig( fixture.engine, H1, H2 );
        SpriteRegistry     registry;

        registry.Add( 1, &rig.sprite );
        clockFixture.clock.Advance( FRAME_STEP_MS );

        // Off screen in both passes: not drawn, not advanced until EndFrame.
        rig.Place( OFF_SCREEN );
        registry.BeginFrame();
        registry.UpdateLayer( 1, true );
        registry.UpdateLayer( 1, true );
        CHECK( Drawn( fixture.engine ).empty() );
        CHECK( rig.sprite.GetActiveTexture() == &rig.first );
        registry.EndFrame();
        CHECK( rig.sprite.GetActiveTexture() == &rig.second );

        // Off screen in the first pass, on screen in the second: advanced there, and not again at the end.
        clockFixture.clock.Advance( FRAME_STEP_MS );
        registry.BeginFrame();
        registry.UpdateLayer( 1, true );
        CHECK( rig.sprite.GetActiveTexture() == &rig.second );
        rig.Place( ON_SCREEN );
        registry.UpdateLayer( 1, true );
        CHECK( rig.sprite.GetActiveTexture() == &rig.first );
        registry.EndFrame();
        CHECK( rig.sprite.GetActiveTexture() == &rig.first );
        CHECK( Drawn( fixture.engine ) == std :: vector<void*> { H1 } );
    }

    TEST_CASE( "UnloadAll unloads every registered sprite and closes every list; the parented sprites stay tracked" )  {

        MockEngineFixture  fixture;
        TestSprite         a( fixture.engine, H1 );
        TestSprite         b( fixture.engine, H2 );
        SpriteRegistry     registry;

        registry.Add( 1, &a.sprite );
        registry.Add( 2, &b.sprite );
        registry.MarkParented( &a.sprite );

        int  nUnloadsBefore = fixture.engine.nUnloadTextureCalls;

        registry.UnloadAll();

        CHECK( fixture.engine.nUnloadTextureCalls == nUnloadsBefore + 2 );   // each sprite's one canvas
        CHECK( a.sprite.GetTextureSequenceSize( 0 ) == -1 );                  // its sequences are gone
        CHECK( registry.Empty() );
        CHECK_FALSE( registry.Contains( 1, &a.sprite ) );
        CHECK( registry.ParentedSprites() == std :: set<SunLight :: Sprite :: Sprite*> { &a.sprite } );
    }
}
