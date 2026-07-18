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
 * TileMapRenderer's window lifecycle (Start()/Run()/Stop()) calls raylib's
 * InitWindow()/WindowShouldClose()/BeginDrawing()/EndDrawing() directly, so
 * it needs a real display and isn't unit-testable here (see
 * doc/MISSING_FEATURES.md). What IS safely testable without ever calling
 * Start(): every entry point that gates on m_bIsStarted or a loaded
 * m_pTmxMap (LoadMap, AddSprite, RemoveSprite, GetMapInfo, GetLayer,
 * SetLayer, TileMapToTileMatrix) is documented to fail gracefully rather
 * than crash when called too early - this file locks that contract in,
 * plus the handful of accessors (GetInputHandler, GetCollisionManager)
 * that work standalone.
 */

#include <doctest/doctest.h>
#include "renderer/tilemaprenderer.h"
#include "sprite/sprite.h"

using namespace SunLight :: Renderer;
using namespace SunLight :: TileMap;

TEST_SUITE( "renderer/TileMapRenderer" )  {

    TEST_CASE( "Construction does not crash and does not start the renderer" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, true );
        stMapInfo        mapInfo;

        CHECK( renderer.GetMapInfo( mapInfo ) == false );
    }

    TEST_CASE( "GetInputHandler returns the same handler instance on every call" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( &renderer.GetInputHandler() == &renderer.GetInputHandler() );
    }

    TEST_CASE( "GetCollisionManager returns a stable, usable reference before any map is loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( &renderer.GetCollisionManager() == &renderer.GetCollisionManager() );

        // AddColliderToColliderRule only touches the collider-layer lists,
        // not the parent tile map, so it works even with nothing loaded.
        CHECK( renderer.GetCollisionManager().AddColliderToColliderRule( 0, 1 ) == true );
    }

    TEST_CASE( "LoadMap fails without crashing when the renderer hasn't been started" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );

        CHECK( renderer.LoadMap( "does-not-matter.tmx" ) == false );
    }

    TEST_CASE( "UnloadMap/GetMapInfo report failure when no map has been loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );
        stMapInfo        mapInfo;

        CHECK( renderer.UnloadMap() == false );
        CHECK( renderer.GetMapInfo( mapInfo ) == false );
    }

    TEST_CASE( "GetLayer/SetLayer (by id and by name) report failure when no map has been loaded" )  {

        TileMapRenderer  renderer( 800, 600, "test", -1, false );
        stLayer          layer {};

        CHECK( renderer.GetLayer( 1, layer ) == false );
        CHECK( renderer.GetLayer( "layer1", layer ) == false );
        CHECK( renderer.SetLayer( 1, layer ) == false );
        CHECK( renderer.SetLayer( "layer1", layer ) == false );
    }

    TEST_CASE( "TileMapToTileMatrix reports failure when no map has been loaded" )  {

        TileMapRenderer   renderer( 800, 600, "test", -1, false );
        stCoordinate2D    coord { 0, 0 };
        stMatrixPosition  pos   { 0, 0 };

        CHECK( renderer.TileMapToTileMatrix( coord, pos ) == false );
    }

    TEST_CASE( "AddSprite/RemoveSprite report failure when the renderer hasn't been started" )  {

        TileMapRenderer                    renderer( 800, 600, "test", -1, false );
        SunLight :: Sprite :: Sprite       sprite;

        CHECK( renderer.AddSprite( 1, sprite ) == false );
        CHECK( renderer.RemoveSprite( 1, sprite ) == false );
    }
}
