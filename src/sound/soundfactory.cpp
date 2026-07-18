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

#include "soundfactory.h"

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif

#if DEFAULT_ENGINE == 1    /* USES RAYLIB */
    #include "sound/raylib/raylibsound.h"

    #define __DEFAULT_ENGINE  SunLight :: Sound :: RayLib :: RayLibSound
#else
    #error "Unknown value of DEFAULT_ENGINE"
#endif


namespace SunLight {
    namespace Sound  {

        static SoundFactory :: CreatorFunction   s_pCreator = nullptr;

        /**
         * @brief Create a new @see ISound instance - the backend selected
         * at build time, unless overridden by @see SetCreator() (tests only).
         */
        std :: unique_ptr<ISound> SoundFactory :: CreateSound( void )  {

            if( s_pCreator )
                return s_pCreator();

            return std :: make_unique<__DEFAULT_ENGINE>();
        }

        /**
         * @brief Override the backend used by @see CreateSound().
         */
        void SoundFactory :: SetCreator( CreatorFunction creator )  {

            s_pCreator = creator;
        }
    }
}
