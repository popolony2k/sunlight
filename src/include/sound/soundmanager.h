/*
 * soundmanager.h
 *
 *  Created on: Jan 2, 2024
 *      Author: popolony2k
 */

#ifndef __SOUNDMANAGER_H__
#define __SOUNDMANAGER_H__

#include <string>
#include <map>
#include "isound.h"


namespace SunLight {
    namespace Sound  {

        /**
         * @brief Sound manager class implementation.
         * 
         */
        class SoundManager  {

            std :: map<int, ISound*>   m_SoundMap;

            public:

            SoundManager( void );
            virtual ~SoundManager( void );

            bool Load( int nSoundId, std :: string strFileName );
            bool Unload( int nSoundId );

            bool Play( int nSoundId );
            bool Stop( int nSoudId );
            bool Pause( int nSoundId );
            bool Resume( int nSoundId );
            bool IsPlaying( int nSoundId );
        };
    }
}

#endif /* __SOUNDMANAGER_H__ */