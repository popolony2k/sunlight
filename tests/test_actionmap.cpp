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
 * F3 of the master plan: ActionMap, named actions bound to a key, gamepad button or gamepad
 * axis direction, rebindable at any time, with save/load to a plain text file.
 */

#include <doctest/doctest.h>
#include "input/actionmap.h"
#include "backends/physfs/physfsfilesystem.h"
#include "mock_filesystem.h"
#include "mock_input.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace SunLight :: Input;

namespace  {

    // A path under the system temp directory, never the project tree (test files are not
    // scratch tools, but there's no reason to litter the repo either).
    std :: filesystem :: path TempBindingsPath( const char* szName )  {

        return std :: filesystem :: temp_directory_path() / szName;
    }

    // An OS file, as ActionMap::Save/Load take it since I12: a host path (a plain path is virtual).
    std :: string HostPath( const std :: filesystem :: path& path )  {

        return "hpath://" + path.string();
    }
}

TEST_SUITE( "input/ActionMap" )  {

    TEST_CASE( "An unbound action has no binding and is never active" )  {

        ActionMap          map;
        MockInputHandler    input;

        CHECK( map.GetBinding( "Jump" ).type == ACTION_BINDING_NONE );
        CHECK( map.IsActionActive( "Jump", input, 0, 0.5f ) == false );
    }

    TEST_CASE( "BindKey binds and IsActionActive reflects IsKeyDown" )  {

        ActionMap          map;
        MockInputHandler    input;

        CHECK( map.BindKey( "Jump", KEY_SPACE ) == true );
        CHECK( map.GetBinding( "Jump" ).type == ACTION_BINDING_KEY );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );

        CHECK( map.IsActionActive( "Jump", input, 0, 0.5f ) == false );
        input.downKeys.insert( KEY_SPACE );
        CHECK( map.IsActionActive( "Jump", input, 0, 0.5f ) == true );
    }

    TEST_CASE( "BindGamepadButton binds and IsActionActive reflects IsGamepadButtonDown" )  {

        ActionMap          map;
        MockInputHandler    input;

        CHECK( map.BindGamepadButton( "Fire", GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == true );

        CHECK( map.IsActionActive( "Fire", input, 0, 0.5f ) == false );
        input.downButtons.insert( GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        CHECK( map.IsActionActive( "Fire", input, 0, 0.5f ) == true );
    }

    TEST_CASE( "BindGamepadAxis binds and IsActionActive only fires past the threshold, in its own direction" )  {

        ActionMap          map;
        MockInputHandler    input;

        CHECK( map.BindGamepadAxis( "MoveRight", GAMEPAD_AXIS_LEFT_X, 1 ) == true );

        input.axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.0f;
        CHECK( map.IsActionActive( "MoveRight", input, 0, 0.5f ) == false );

        input.axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.4f;   // short of the threshold
        CHECK( map.IsActionActive( "MoveRight", input, 0, 0.5f ) == false );

        input.axisValues[ GAMEPAD_AXIS_LEFT_X ] = 0.6f;
        CHECK( map.IsActionActive( "MoveRight", input, 0, 0.5f ) == true );

        input.axisValues[ GAMEPAD_AXIS_LEFT_X ] = -0.6f;   // wrong direction
        CHECK( map.IsActionActive( "MoveRight", input, 0, 0.5f ) == false );
    }

    TEST_CASE( "Opposite directions on the same axis are separate bindings and never conflict" )  {

        ActionMap  map;

        CHECK( map.BindGamepadAxis( "MoveLeft", GAMEPAD_AXIS_LEFT_X, -1 ) == true );
        CHECK( map.BindGamepadAxis( "MoveRight", GAMEPAD_AXIS_LEFT_X, 1 ) == true );

        CHECK( map.GetBinding( "MoveLeft" ).nDirection == -1 );
        CHECK( map.GetBinding( "MoveRight" ).nDirection == 1 );
    }

    TEST_CASE( "A rebind round trip: rebinding an action frees its old input for another action" )  {

        ActionMap  map;

        CHECK( map.BindKey( "Jump", KEY_W ) == true );
        CHECK( map.BindKey( "Jump", KEY_SPACE ) == true );   // rebind

        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );

        // KEY_W is free again - a different action can take it.
        CHECK( map.BindKey( "Crouch", KEY_W ) == true );
        CHECK( map.GetBinding( "Crouch" ).nCode == KEY_W );
    }

    TEST_CASE( "Rebinding an action to the input it already holds is an idempotent success" )  {

        ActionMap  map;

        map.BindKey( "Jump", KEY_SPACE );

        CHECK( map.BindKey( "Jump", KEY_SPACE ) == true );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );
    }

    TEST_CASE( "A conflicting binding is reported: a key already bound to a different action is refused" )  {

        ActionMap  map;

        map.BindKey( "Jump", KEY_SPACE );

        CHECK( map.BindKey( "Fire", KEY_SPACE ) == false );
        CHECK( map.GetBinding( "Fire" ).type == ACTION_BINDING_NONE );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );   // unchanged
    }

    TEST_CASE( "A conflicting gamepad button binding is refused the same way" )  {

        ActionMap  map;

        map.BindGamepadButton( "Fire", GAMEPAD_BUTTON_RIGHT_FACE_DOWN );

        CHECK( map.BindGamepadButton( "Jump", GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == false );
    }

    TEST_CASE( "A conflicting gamepad axis binding (same axis, same direction) is refused" )  {

        ActionMap  map;

        map.BindGamepadAxis( "MoveRight", GAMEPAD_AXIS_LEFT_X, 1 );

        CHECK( map.BindGamepadAxis( "Dash", GAMEPAD_AXIS_LEFT_X, 1 ) == false );
    }

    TEST_CASE( "Unbind clears an action's binding, a no-op if it had none" )  {

        ActionMap  map;

        map.BindKey( "Jump", KEY_SPACE );
        map.Unbind( "Jump" );
        CHECK( map.GetBinding( "Jump" ).type == ACTION_BINDING_NONE );

        map.Unbind( "NeverBound" );
        CHECK( map.GetBinding( "NeverBound" ).type == ACTION_BINDING_NONE );
    }

    TEST_CASE( "A saved file reloads to the same bindings" )  {

        std :: filesystem :: path  path = TempBindingsPath( "sunlight_test_actionmap_roundtrip.bindings" );
        ActionMap                   saved;

        saved.BindKey( "Jump", KEY_SPACE );
        saved.BindGamepadButton( "Fire", GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        saved.BindGamepadAxis( "MoveLeft", GAMEPAD_AXIS_LEFT_X, -1 );
        saved.BindGamepadAxis( "MoveRight", GAMEPAD_AXIS_LEFT_X, 1 );

        REQUIRE( saved.Save( HostPath( path ) ) == true );

        ActionMap  loaded;

        REQUIRE( loaded.Load( HostPath( path ) ) == true );

        CHECK( loaded.GetBinding( "Jump" ).type == ACTION_BINDING_KEY );
        CHECK( loaded.GetBinding( "Jump" ).nCode == KEY_SPACE );
        CHECK( loaded.GetBinding( "Fire" ).type == ACTION_BINDING_GAMEPAD_BUTTON );
        CHECK( loaded.GetBinding( "Fire" ).nCode == GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        CHECK( loaded.GetBinding( "MoveLeft" ).type == ACTION_BINDING_GAMEPAD_AXIS );
        CHECK( loaded.GetBinding( "MoveLeft" ).nCode == GAMEPAD_AXIS_LEFT_X );
        CHECK( loaded.GetBinding( "MoveLeft" ).nDirection == -1 );
        CHECK( loaded.GetBinding( "MoveRight" ).nDirection == 1 );

        std :: filesystem :: remove( path );
    }

    TEST_CASE( "Load replaces every binding the map previously held" )  {

        std :: filesystem :: path  path = TempBindingsPath( "sunlight_test_actionmap_replace.bindings" );
        ActionMap                   saved;

        saved.BindKey( "Jump", KEY_SPACE );
        saved.Save( HostPath( path ) );

        ActionMap  loaded;

        loaded.BindKey( "Stale", KEY_A );
        REQUIRE( loaded.Load( HostPath( path ) ) == true );

        CHECK( loaded.GetBinding( "Jump" ).nCode == KEY_SPACE );
        CHECK( loaded.GetBinding( "Stale" ).type == ACTION_BINDING_NONE );

        std :: filesystem :: remove( path );
    }

    TEST_CASE( "Load fails for a nonexistent file and leaves existing bindings untouched" )  {

        ActionMap  map;

        map.BindKey( "Jump", KEY_SPACE );

        CHECK( map.Load( HostPath( TempBindingsPath( "sunlight_test_actionmap_missing.bindings" ) ) ) == false );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );
    }

    TEST_CASE( "Load fails for a malformed line and leaves existing bindings untouched" )  {

        std :: filesystem :: path  path = TempBindingsPath( "sunlight_test_actionmap_malformed.bindings" );

        {
            std :: ofstream  file( path );
            file << "Jump=NOT_A_REAL_TAG:1\n";
        }

        ActionMap  map;

        map.BindKey( "Fire", KEY_SPACE );
        CHECK( map.Load( HostPath( path ) ) == false );
        CHECK( map.GetBinding( "Fire" ).nCode == KEY_SPACE );
        CHECK( map.GetBinding( "Jump" ).type == ACTION_BINDING_NONE );

        std :: filesystem :: remove( path );
    }

    // I12 of the master plan: Load and Save go through SunLight::FileSystem, so a bindings file inside
    // a mounted archive or an encrypted pack loads (found by Scarab: Load used a raw std::ifstream).

    TEST_CASE( "Load reads a virtual path through the FileSystem: a file that exists nowhere on disk" )  {

        MemoryFileSystemFixture  fsFixture;
        ActionMap                map;
        std :: string            strShipped = "# defaults\nJump=KEY:32\nFire=BUTTON:7\n";

        fsFixture.fs.files["config/default.bindings"] = Bytes( strShipped.begin(), strShipped.end() );

        REQUIRE( map.Load( "config/default.bindings" ) == true );
        CHECK( map.GetBinding( "Jump" ).type == ACTION_BINDING_KEY );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );
        CHECK( map.GetBinding( "Fire" ).type == ACTION_BINDING_GAMEPAD_BUTTON );
        CHECK( map.GetBinding( "Fire" ).nCode == 7 );
        CHECK( map.Load( "vpath://config/default.bindings" ) == true );
    }

    TEST_CASE( "Load reads through the read filter: an encrypted bindings file" )  {

        MemoryFileSystemFixture  fsFixture;
        ActionMap                map;
        std :: string            strShipped = "Jump=KEY:32\n";

        for( char& c : strShipped )
            c = ( char ) ( c ^ 0x5A );

        fsFixture.fs.files["config/default.bindings"] = Bytes( strShipped.begin(), strShipped.end() );
        fsFixture.fs.SetReadFilter( []( const Bytes& in, Bytes& out )  {
            out = in;

            for( unsigned char& c : out )
                c = ( unsigned char ) ( c ^ 0x5A );

            return true;
        } );

        REQUIRE( map.Load( "config/default.bindings" ) == true );
        CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );
    }

    TEST_CASE( "Save writes a virtual path into the write directory, and Load reads it back" )  {

        MemoryFileSystemFixture  fsFixture;
        ActionMap                saved;
        ActionMap                loaded;

        saved.BindKey( "Jump", KEY_SPACE );
        CHECK( saved.Save( "user/my.bindings" ) == false );          // no write directory yet

        fsFixture.fs.SetWriteDirectory( "user" );
        REQUIRE( saved.Save( "user/my.bindings" ) == true );
        REQUIRE( loaded.Load( "user/my.bindings" ) == true );
        CHECK( loaded.GetBinding( "Jump" ).nCode == KEY_SPACE );
    }

    TEST_CASE( "Load reads a bindings file packed in a mounted .zip, with no copy on disk" )  {

        namespace fs = std :: filesystem;

        fs :: path  scratch = fs :: temp_directory_path() / "sunlight_actionmap_zip";
        fs :: path  zipPath = fs :: temp_directory_path() / "sunlight_actionmap_zip.zip";

        fs :: remove_all( scratch );
        fs :: remove( zipPath );
        fs :: create_directories( scratch / "config" );
        std :: ofstream( scratch / "config" / "default.bindings", std :: ios :: binary ) << "Jump=KEY:32\n";

        std :: string  strZipCmd = "cmake -E chdir \"" + scratch.string() + "\" cmake -E tar cf \"" + zipPath.string() + "\" --format=zip .";

        REQUIRE( std :: system( strZipCmd.c_str() ) == 0 );
        fs :: remove_all( scratch );                                  // only the archive is left

        {
            SunLight :: FileSystem :: PhysFs :: PhysFsFileSystem  packFs;
            ActionMap                                             map;

            REQUIRE( packFs.Init( "sunlight_tests" ) );
            REQUIRE( packFs.Mount( zipPath.string(), "/", true ) );
            SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( &packFs );

            CHECK( map.Load( "config/default.bindings" ) == true );
            CHECK( map.GetBinding( "Jump" ).nCode == KEY_SPACE );

            SunLight :: FileSystem :: FileSystemFactory :: SetFileSystem( nullptr );
            packFs.Shutdown();
        }

        fs :: remove( zipPath );
    }
}
