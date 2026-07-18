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

#include <doctest/doctest.h>
#include "scripting/scriptprocessor.h"
#include <vector>
#include <utility>

using namespace SunLight :: Scripting;

namespace  {

    /**
     * @brief Records every OnCommand()/OnError() call it receives, so tests
     * can assert on what ScriptProcessor :: Run() actually dispatched.
     */
    class TestScriptListener : public IScriptListener  {

        public:

        std :: vector<std :: pair<Commands, uint16_t>>  commandCalls;
        std :: vector<std :: string>                    errorCalls;

        void OnCommand( Commands cmd, uint16_t nEventId )  {
            commandCalls.push_back( { cmd, nEventId } );
        }

        void OnError( std :: string strError )  {
            errorCalls.push_back( strError );
        }
    };
}

TEST_SUITE( "scripting/ScriptProcessor" )  {

    TEST_CASE( "Compile fails on an empty queue and succeeds once a command is added" )  {

        ScriptProcessor  sp;

        CHECK( sp.Compile() == false );

        sp.AddNoParmCmd( WAIT_SPRITES_QUEUE_EMPTY );

        CHECK( sp.Compile() == true );
    }

    TEST_CASE( "Run drains the queue one command per call and returns false once exhausted" )  {

        ScriptProcessor  sp;

        sp.AddOneParmCmd( WAIT_CMD, 0 );
        sp.AddOneParmCmd( WAIT_CMD, 0 );
        sp.Compile();

        CHECK( sp.Run() == true );
        CHECK( sp.Run() == true );
        CHECK( sp.Run() == false );
    }

    TEST_CASE( "Run blocks on WAIT_CMD until the requested delay elapses" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddOneParmCmd( WAIT_CMD, 60000 );  // 60s - never elapses during this test
        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );
        sp.AddScriptListener( &listener );
        sp.Compile();

        CHECK( sp.Run() == true );
        CHECK( listener.commandCalls.empty() );

        CHECK( sp.Run() == true );
        CHECK( listener.commandCalls.empty() );
    }

    TEST_CASE( "Listener-dispatched commands forward the command and parameter" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddOneParmCmd( MOVE_SPRITES_TO_SCREEN_CMD, 7 );
        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );
        sp.AddScriptListener( &listener );
        sp.Compile();

        CHECK( sp.Run() == true );
        CHECK( sp.Run() == true );

        REQUIRE( listener.commandCalls.size() == 2 );
        CHECK( listener.commandCalls[0] == std :: make_pair( ( Commands ) MOVE_SPRITES_TO_SCREEN_CMD, ( uint16_t ) 7 ) );
        CHECK( listener.commandCalls[1] == std :: make_pair( ( Commands ) PLAY_SONG_CMD, ( uint16_t ) 42 ) );
    }

    TEST_CASE( "Run does not crash on a listener-dispatched command when no listener is attached" )  {

        ScriptProcessor  sp;

        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );
        sp.Compile();

        CHECK( sp.Run() == true );
    }

    TEST_CASE( "WAIT_SPRITES_QUEUE_EMPTY pauses the queue until ResetWaitSpritesQueueEmptyCmd is called" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddNoParmCmd( WAIT_SPRITES_QUEUE_EMPTY );
        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );
        sp.AddScriptListener( &listener );
        sp.Compile();

        CHECK( sp.Run() == true );
        CHECK( listener.commandCalls.empty() );

        // Still paused - repeated Run() calls must not advance past the wait.
        CHECK( sp.Run() == true );
        CHECK( listener.commandCalls.empty() );

        sp.ResetWaitSpritesQueueEmptyCmd();

        CHECK( sp.Run() == true );
        REQUIRE( listener.commandCalls.size() == 1 );
        CHECK( listener.commandCalls[0].second == 42 );
    }

    TEST_CASE( "GOTO_LABEL_CMD jumps back to a previously registered label" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        // A label can only be jumped to once LABEL_CMD has actually run and
        // registered it - this is a single forward pass, not a pre-scan of
        // the whole script, so only backward jumps to already-visited
        // labels are possible. GOTO_LABEL_CMD has no condition of its own,
        // so this repeats indefinitely; call Run() a bounded number of times
        // instead of draining to exhaustion.
        sp.AddOneParmCmd( LABEL_CMD, 1 );          // 0
        sp.AddOneParmCmd( PLAY_SONG_CMD, 42 );     // 1
        sp.AddOneParmCmd( GOTO_LABEL_CMD, 1 );     // 2
        sp.AddScriptListener( &listener );
        sp.Compile();

        for( int nCount = 0; nCount < 5; nCount++ )
            CHECK( sp.Run() == true );

        REQUIRE( listener.commandCalls.size() >= 2 );

        for( const std :: pair<Commands, uint16_t>& call : listener.commandCalls )
            CHECK( call.second == 42 );
    }

    TEST_CASE( "GOTO_LABEL_CMD reports an error via the listener for an unknown label instead of crashing" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddOneParmCmd( GOTO_LABEL_CMD, 999 );  // no matching LABEL_CMD anywhere
        sp.AddScriptListener( &listener );
        sp.Compile();

        CHECK( sp.Run() == true );

        CHECK( listener.errorCalls.size() == 1 );
    }

    TEST_CASE( "END_LOOP_CMD reports an error via the listener for an unmatched label instead of crashing" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddOneParmCmd( END_LOOP_CMD, 999 );  // no matching LOOP_CMD anywhere
        sp.AddScriptListener( &listener );
        sp.Compile();

        CHECK( sp.Run() == true );

        CHECK( listener.errorCalls.size() == 1 );
    }

    TEST_CASE( "LOOP_CMD/END_LOOP_CMD repeats the loop body the requested number of times" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        sp.AddTwoParmsCmd( LOOP_CMD, /* label */ 1, /* count */ 2 );  // 0
        sp.AddOneParmCmd( MOVE_SPRITES_TO_SCREEN_CMD, 7 );            // 1: loop body
        sp.AddOneParmCmd( END_LOOP_CMD, /* label */ 1 );              // 2
        sp.AddOneParmCmd( PLAY_SONG_CMD, 99 );                        // 3: after the loop
        sp.AddScriptListener( &listener );
        sp.Compile();

        while( sp.Run() )
            ;

        // A requested count of 2 additional repeats means the body runs
        // 3 times in total (the initial pass plus 2 jumps back).
        REQUIRE( listener.commandCalls.size() == 4 );
        CHECK( listener.commandCalls[0] == std :: make_pair( ( Commands ) MOVE_SPRITES_TO_SCREEN_CMD, ( uint16_t ) 7 ) );
        CHECK( listener.commandCalls[1] == std :: make_pair( ( Commands ) MOVE_SPRITES_TO_SCREEN_CMD, ( uint16_t ) 7 ) );
        CHECK( listener.commandCalls[2] == std :: make_pair( ( Commands ) MOVE_SPRITES_TO_SCREEN_CMD, ( uint16_t ) 7 ) );
        CHECK( listener.commandCalls[3] == std :: make_pair( ( Commands ) PLAY_SONG_CMD, ( uint16_t ) 99 ) );
    }

    TEST_CASE( "Clear empties the queue so Compile() reports it as empty again" )  {

        ScriptProcessor  sp;

        sp.AddNoParmCmd( WAIT_SPRITES_QUEUE_EMPTY );
        sp.Compile();

        sp.Clear();

        CHECK( sp.Compile() == false );
    }

    TEST_CASE( "AddScriptListener/GetScriptListener round-trip" )  {

        TestScriptListener  listener;
        ScriptProcessor      sp;

        CHECK( sp.GetScriptListener() == nullptr );

        sp.AddScriptListener( &listener );

        CHECK( sp.GetScriptListener() == &listener );
    }
}
