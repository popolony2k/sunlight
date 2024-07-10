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

#ifndef __RAYLIBSOUND_H__
#define __RAYLIBSOUND_H__

#include <raylib.h>
#include "isound.h"


namespace SunLight {
    namespace Sound  {
        namespace RayLib  {

            /**
             * @brief Implement the sound data handler for RayLib target.
             * 
             */
            class RayLibSound : public SunLight :: Sound :: ISound  {

                ::Sound       m_Sound;
                bool          m_Loaded;
                static int    m_nReferenceCount;

                void Initialize( void );
                void Finalize( void );

                public:

                RayLibSound( void );
                virtual ~RayLibSound( void );

                bool Load( std :: string strFileName );
                bool Unload( void );

                bool Play( void );
                bool Stop( void );
                bool Pause( void );
                bool Resume( void );
                bool IsPlaying( void );
            };
        }
    }
}
#endif /* __RAYLIBSOUND_H__ */
