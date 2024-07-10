/*
 * raylibsound.h
 *
 *  Created on: Jan 3, 2024
 *      Author: popolony2k
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
