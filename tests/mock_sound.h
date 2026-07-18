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

#ifndef __MOCK_SOUND_H__
#define __MOCK_SOUND_H__

#include "sound/isound.h"
#include "sound/soundfactory.h"
#include <string>

/**
 * @brief Test double for SunLight::Sound::ISound - records call counts and
 * the last file name passed to Load(), and returns a per-instance result
 * for each method so tests can force success/failure paths.
 */
class MockSound : public SunLight :: Sound :: ISound  {

    public:

    bool          bLoadResult      = true;
    bool          bUnloadResult    = true;
    bool          bPlayResult      = true;
    bool          bStopResult      = true;
    bool          bPauseResult     = true;
    bool          bResumeResult    = true;
    bool          bIsPlayingResult = false;

    int           nLoadCalls      = 0;
    int           nUnloadCalls    = 0;
    int           nPlayCalls      = 0;
    int           nStopCalls      = 0;
    int           nPauseCalls     = 0;
    int           nResumeCalls    = 0;
    int           nIsPlayingCalls = 0;

    std :: string strLastFileName;

    bool Load( std :: string strFileName )  {
        nLoadCalls++;
        strLastFileName = strFileName;
        return bLoadResult;
    }

    bool Unload( void )  {
        nUnloadCalls++;
        return bUnloadResult;
    }

    bool Play( void )  {
        nPlayCalls++;
        return bPlayResult;
    }

    bool Stop( void )  {
        nStopCalls++;
        return bStopResult;
    }

    bool Pause( void )  {
        nPauseCalls++;
        return bPauseResult;
    }

    bool Resume( void )  {
        nResumeCalls++;
        return bResumeResult;
    }

    bool IsPlaying( void )  {
        nIsPlayingCalls++;
        return bIsPlayingResult;
    }
};

/**
 * @brief RAII fixture that points SoundFactory :: CreateSound() at freshly
 * created MockSound instances for the fixture's lifetime, restoring the
 * default (real) backend on destruction. pLastCreated tracks the most
 * recently created mock so tests can inspect/configure it.
 *
 * Gotcha: SoundManager owns the ISound it creates and destroys it the
 * moment it's no longer kept (Load() returning false never stores it;
 * a successful Unload() erases it from the map) - so pLastCreated dangles
 * right after either of those. Only dereference it before the call that
 * discards the sound, or to configure it before that call happens.
 */
class MockSoundFixture  {

    public:

    MockSound   *pLastCreated  = nullptr;
    bool         bNextLoadResult = true;

    MockSoundFixture( void )  {
        SunLight :: Sound :: SoundFactory :: SetCreator( [this]( void ) -> std :: unique_ptr<SunLight :: Sound :: ISound>  {
            std :: unique_ptr<MockSound>  pMock = std :: make_unique<MockSound>();

            pMock -> bLoadResult = bNextLoadResult;
            pLastCreated = pMock.get();

            return pMock;
        } );
    }

    ~MockSoundFixture( void )  {
        SunLight :: Sound :: SoundFactory :: SetCreator( nullptr );
    }
};

#endif /* __MOCK_SOUND_H__ */
