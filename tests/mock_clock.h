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

#ifndef __MOCK_CLOCK_H__
#define __MOCK_CLOCK_H__

#include "general/clock.h"

/**
 * @brief Test double for SunLight::General::IClock - time only moves when a
 * test says so, so wait/animation timing is exercised deterministically
 * with no real sleeping.
 */
class MockClock : public SunLight :: General :: IClock  {

    public:

    int64_t  nNow = 0;

    int64_t NowMilliseconds( void )  {
        return nNow;
    }

    void Advance( int64_t nMilliseconds )  {
        nNow += nMilliseconds;
    }
};

/**
 * @brief RAII fixture that installs a single MockClock as the process-global
 * clock for the fixture's lifetime, restoring the real one on destruction.
 */
class MockClockFixture  {

    public:

    MockClock  clock;

    MockClockFixture( void )  {
        SunLight :: General :: Clock :: SetClock( &clock );
    }

    ~MockClockFixture( void )  {
        SunLight :: General :: Clock :: SetClock( nullptr );
    }
};

#endif /* __MOCK_CLOCK_H__ */
