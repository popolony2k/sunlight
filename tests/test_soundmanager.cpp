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
#include "sound/soundmanager.h"
#include "mock_sound.h"

using namespace SunLight :: Sound;

TEST_SUITE( "sound/SoundManager" )  {

    TEST_CASE( "Load creates a sound via SoundFactory and stores it under the given id" )  {

        MockSoundFixture  fixture;
        SoundManager      manager;

        CHECK( manager.Load( 1, "test.wav" ) == true );
        REQUIRE( fixture.pLastCreated != nullptr );
        CHECK( fixture.pLastCreated -> nLoadCalls == 1 );
        CHECK( fixture.pLastCreated -> strLastFileName == "test.wav" );
    }

    TEST_CASE( "Load returns false and does not store the sound when the backend fails" )  {

        MockSoundFixture  fixture;
        SoundManager      manager;

        fixture.bNextLoadResult = false;

        // A failed Load() is never stored in m_SoundMap, so the mock created
        // for it is destroyed before this call returns - pLastCreated would
        // dangle if dereferenced here (see MockSoundFixture's doc comment).
        // Id 1 was never inserted, so every later operation on it must
        // report failure.
        CHECK( manager.Load( 1, "bad.wav" ) == false );
        CHECK( manager.Play( 1 ) == false );
    }

    TEST_CASE( "Play/Stop/Pause/Resume/IsPlaying forward to the loaded sound" )  {

        MockSoundFixture  fixture;
        SoundManager      manager;

        manager.Load( 5, "a.wav" );
        REQUIRE( fixture.pLastCreated != nullptr );

        fixture.pLastCreated -> bIsPlayingResult = true;

        CHECK( manager.Play( 5 ) == true );
        CHECK( fixture.pLastCreated -> nPlayCalls == 1 );

        CHECK( manager.Stop( 5 ) == true );
        CHECK( fixture.pLastCreated -> nStopCalls == 1 );

        CHECK( manager.Pause( 5 ) == true );
        CHECK( fixture.pLastCreated -> nPauseCalls == 1 );

        CHECK( manager.Resume( 5 ) == true );
        CHECK( fixture.pLastCreated -> nResumeCalls == 1 );

        CHECK( manager.IsPlaying( 5 ) == true );
        CHECK( fixture.pLastCreated -> nIsPlayingCalls == 1 );
    }

    TEST_CASE( "Operations on an unknown sound id return false without creating any sound" )  {

        MockSoundFixture  fixture;
        SoundManager      manager;

        CHECK( manager.Play( 999 ) == false );
        CHECK( manager.Stop( 999 ) == false );
        CHECK( manager.Pause( 999 ) == false );
        CHECK( manager.Resume( 999 ) == false );
        CHECK( manager.IsPlaying( 999 ) == false );
        CHECK( manager.Unload( 999 ) == false );

        CHECK( fixture.pLastCreated == nullptr );
    }

    TEST_CASE( "Unload only erases the sound once the backend unload succeeds" )  {

        MockSoundFixture  fixture;
        SoundManager      manager;

        manager.Load( 3, "a.wav" );
        REQUIRE( fixture.pLastCreated != nullptr );

        fixture.pLastCreated -> bUnloadResult = false;

        CHECK( manager.Unload( 3 ) == false );
        CHECK( fixture.pLastCreated -> nUnloadCalls == 1 );

        // Still present in the map - operations must still reach the mock.
        CHECK( manager.Play( 3 ) == true );

        fixture.pLastCreated -> bUnloadResult = true;

        // A successful Unload() erases the entry from m_SoundMap, which
        // destroys the mock - pLastCreated would dangle if dereferenced
        // after this call (see MockSoundFixture's doc comment).
        CHECK( manager.Unload( 3 ) == true );

        // Erased now - further operations report failure.
        CHECK( manager.Play( 3 ) == false );
    }
}
