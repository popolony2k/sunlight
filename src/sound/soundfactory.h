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

#ifndef __SOUNDFACTORY_H__
#define __SOUNDFACTORY_H__

#include "isound.h"
#include <memory>
#include <functional>


namespace SunLight {
    namespace Sound  {

        /**
         * @brief Sound backend access point used to create @see ISound
         * implementations of the type selected at build time (DEFAULT_ENGINE).
         * This is the only place in the sound module that needs to know
         * which concrete backend is compiled in.
         *
         * Unlike @see SunLight :: Engines :: EngineFactory (which hands back
         * a single shared engine instance), each call here returns a brand
         * new @see ISound - SoundManager keeps one per loaded sound id, so
         * a shared singleton wouldn't make sense.
         */
        class SoundFactory  {

            public:

            typedef std :: function<std :: unique_ptr<ISound>( void )>  CreatorFunction;

            static std :: unique_ptr<ISound> CreateSound( void );

            /**
             * @brief Override the backend used by @see CreateSound() - for
             * tests only, to substitute a mock @see ISound without a real
             * audio device. Pass an empty @see CreatorFunction to restore
             * the default, build-time backend.
             *
             * @param creator The replacement creator function, or an empty
             * std::function to reset back to the default backend;
             */
            static void SetCreator( CreatorFunction creator );
        };
    }
}

#endif  /* __SOUNDFACTORY_H__ */
