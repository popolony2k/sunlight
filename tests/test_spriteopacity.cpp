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
 * I10 of the master plan: a sprite's opacity (0..1) and tint, applied by Sprite::Draw() to the
 * frame it draws, for that draw only - multiplied into the frame canvas's own colour, which is
 * never changed. Values worked out by hand: alpha = canvas alpha x opacity, truncated; each RGB
 * channel = (canvas x tint + 127) / 255.
 */

#include <doctest/doctest.h>
#include <cmath>
#include <memory>
#include <vector>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"
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

    // A started renderer with no map.
    struct Scene  {

        MockEngineFixture                   engineFixture;
        MockWindowFixture                   windowFixture;
        MemoryFileSystemFixture             fsFixture;
        std :: unique_ptr<TileMapRenderer>  pRenderer;

        Scene( void )  {

            RendererConfig  config;

            config.fWidth   = 1260.0f;
            config.fHeight  = 920.0f;
            config.viewport = Rect( 10, 10, 400, 300 );

            pRenderer = TileMapRenderer :: Create( config, nullptr );

            REQUIRE( pRenderer != nullptr );
            REQUIRE( pRenderer -> Start() == true );
        }

        ~Scene( void )  {
            if( pRenderer )
                pRenderer -> Stop();
        }

        MockEngine& engine( void )  { return engineFixture.engine; }

        // One frame's TILE events: what the sprites drew, with the colour each was drawn in.
        std :: vector<Event> Frame( void )  {
            engine().events.clear();
            windowFixture.window.nEndFrameCalls          = 0;
            windowFixture.window.nFramesUntilShouldClose = 1;
            pRenderer -> Run();

            std :: vector<Event>  tiles;

            for( const Event& evt : engine().events )
                if( evt.kind == Event :: TILE )
                    tiles.push_back( evt );

            return tiles;
        }
    };

    // A one-frame sprite on layer 1. Another rig's canvas can be shared instead of its own.
    struct SpriteRig  {
        SunLight :: Sprite :: Sprite         sprite;
        SunLight :: Canvas :: TextureCanvas  canvas;
        Scene                                &m_Scene;

        SpriteRig( Scene& scene, SunLight :: Canvas :: TextureCanvas* pShared = nullptr ) : m_Scene( scene )  {

            SunLight :: Canvas :: TextureCanvas  *pCanvas = pShared ? pShared : &canvas;

            if( pShared == nullptr )  {
                scene.engine().nLoadTextureWidth  = 16;
                scene.engine().nLoadTextureHeight = 16;
                REQUIRE( canvas.Load( "sprite.png" ) == true );
                canvas.SetDimension2D( SB :: stDimension2D { { 20, 20 }, { 16, 16 } } );
            }

            sprite.AddTextureSequence( 0, pCanvas, 0 );
            sprite.SetActiveTextureSequence( 0 );
            sprite.SetVisible( true );

            REQUIRE( scene.pRenderer -> AddSprite( 1, sprite ) == true );
        }

        ~SpriteRig( void )  {
            m_Scene.pRenderer -> RemoveSprite( 1, sprite );
        }
    };

    void CheckColor( const SB :: stColor& color, int nRed, int nGreen, int nBlue, int nAlpha )  {
        CHECK( color.nRed   == nRed );
        CHECK( color.nGreen == nGreen );
        CHECK( color.nBlue  == nBlue );
        CHECK( color.nAlpha == nAlpha );
    }
}

TEST_SUITE( "sprite/opacity_tint" )  {

    TEST_CASE( "A sprite is opaque and untinted by default, and draws its canvas's colour as before" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        CHECK( rig.sprite.GetOpacity() == 1.0f );
        CheckColor( rig.sprite.GetTint(), 0xFF, 0xFF, 0xFF, 0xFF );

        std :: vector<Event>  tiles = scene.Frame();

        REQUIRE( tiles.size() == 1 );
        CheckColor( tiles[0].color, 0xFF, 0xFF, 0xFF, 0xFF );
    }

    TEST_CASE( "Opacity multiplies into the drawn alpha, and into the canvas's own" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        rig.sprite.SetOpacity( 0.5f );
        CheckColor( scene.Frame()[0].color, 0xFF, 0xFF, 0xFF, 127 );   // 255 x 0.5 = 127.5, truncated

        rig.canvas.SetColor( SB :: stColor { 0xFF, 0xFF, 0xFF, 200 } );
        CheckColor( scene.Frame()[0].color, 0xFF, 0xFF, 0xFF, 100 );   // 200 x 0.5
    }

    TEST_CASE( "Tint multiplies into the drawn RGB, and into the canvas's own; its alpha is not used" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        rig.sprite.SetTint( SB :: stColor { 0xFF, 0x80, 0x00, 0x10 } );
        CheckColor( scene.Frame()[0].color, 0xFF, 0x80, 0x00, 0xFF );

        // (128 x 255 + 127) / 255 = 128; (255 x 128 + 127) / 255 = 128; 64 x 0 = 0.
        rig.canvas.SetColor( SB :: stColor { 0x80, 0xFF, 0x40, 0xFF } );
        CheckColor( scene.Frame()[0].color, 0x80, 0x80, 0x00, 0xFF );
    }

    TEST_CASE( "Opacity and tint together" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        rig.sprite.SetOpacity( 0.25f );
        rig.sprite.SetTint( SB :: stColor { 0x40, 0x80, 0xC0, 0xFF } );
        CheckColor( scene.Frame()[0].color, 0x40, 0x80, 0xC0, 63 );   // 255 x 0.25 = 63.75, truncated
    }

    TEST_CASE( "The canvas's own colour is the same after the draw, so a sprite sharing it draws it plain" )  {

        Scene      scene;
        SpriteRig  faded( scene );
        SpriteRig  plain( scene, &faded.canvas );

        faded.sprite.SetOpacity( 0.5f );
        faded.sprite.SetTint( SB :: stColor { 0x00, 0xFF, 0x00, 0xFF } );

        std :: vector<Event>  tiles = scene.Frame();

        REQUIRE( tiles.size() == 2 );
        CheckColor( tiles[0].color, 0x00, 0xFF, 0x00, 127 );   // faded, added first
        CheckColor( tiles[1].color, 0xFF, 0xFF, 0xFF, 0xFF );   // plain, the same canvas
        CheckColor( faded.canvas.GetColor(), 0xFF, 0xFF, 0xFF, 0xFF );
    }

    TEST_CASE( "Opacity and tint hold in every view pass" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        rig.sprite.SetOpacity( 0.5f );
        rig.sprite.SetTint( SB :: stColor { 0x80, 0x80, 0xFF, 0xFF } );
        scene.pRenderer -> CreateView( Rect( 500, 10, 400, 300 ) );

        std :: vector<Event>  tiles = scene.Frame();

        REQUIRE( tiles.size() == 2 );   // the default view's pass, then the extra one's

        for( const Event& tile : tiles )
            CheckColor( tile.color, 0x80, 0x80, 0xFF, 127 );
    }

    TEST_CASE( "Opacity is clamped to 0..1, and a NaN leaves it as it was" )  {

        SunLight :: Sprite :: Sprite  sprite;

        sprite.SetOpacity( 1.5f );
        CHECK( sprite.GetOpacity() == 1.0f );
        sprite.SetOpacity( -0.5f );
        CHECK( sprite.GetOpacity() == 0.0f );
        sprite.SetOpacity( 0.75f );
        sprite.SetOpacity( std :: nanf( "" ) );
        CHECK( sprite.GetOpacity() == 0.75f );
    }

    TEST_CASE( "Unload keeps the opacity and the tint: they are the sprite's, not its frames'" )  {

        Scene      scene;
        SpriteRig  rig( scene );

        rig.sprite.SetOpacity( 0.5f );
        rig.sprite.SetTint( SB :: stColor { 0x10, 0x20, 0x30, 0x40 } );
        rig.sprite.Unload();

        CHECK( rig.sprite.GetOpacity() == 0.5f );
        CheckColor( rig.sprite.GetTint(), 0x10, 0x20, 0x30, 0x40 );
    }
}
