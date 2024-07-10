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

#include "soundmanager.h"

#ifndef DEFAULT_ENGINE
    #error "Unexpected value of DEFAULT_ENGINE"
#endif

#if DEFAULT_ENGINE == 1    /* USES RAYLIB */
    #include "sound/raylib/raylibsound.h"
    
    #define __DEFAULT_ENGINE  SunLight :: Sound ::RayLib :: RayLibSound
#else
    #error "Unknown value of DEFAULT_ENGINE"
#endif

namespace SunLight {
    namespace Sound  {

        /**
         * @brief Constructor. Initialize all class data.
         * 
         */
        SoundManager :: SoundManager( void )  {

            m_SoundMap.clear();
        }

        /**
         * @brief Destructor Finalize all class data.
         * Release all sound data previously loaded;
         * 
         */
        SoundManager :: ~SoundManager( void )  {

            for( std :: pair<int, ISound*> item : m_SoundMap )  {
                delete item.second;
            }

            m_SoundMap.clear();
        }

        /**
         * @brief Load a sound data assigning an Id to it;
         * 
         * @param nSoundId The sound id to assign to sound loaded;
         * @param strFileName The sound file name;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Load( int nSoundId, std :: string strFileName )  {

            __DEFAULT_ENGINE  *pSound = new __DEFAULT_ENGINE();
            bool              bRet;

            bRet = pSound -> Load( strFileName.c_str() );

            if( !bRet )
                delete pSound;
            else
                m_SoundMap.insert( std :: make_pair( nSoundId, pSound ) );

            return bRet;
        }

        /**
         * @brief Unload a previously loaded sound;
         * 
         * @param nSoundId The corresponding sound id loaded by @see Load function;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Unload( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool   bRet = false;

            if( itItem != m_SoundMap.end() )  {
                bRet = itItem -> second -> Unload();

                if( bRet )  {
                    delete itItem -> second;
                    m_SoundMap.erase( nSoundId );
                }
            }

            return bRet;
        }

        /**
         * @brief Proovide sound playing method on target engine;
         * 
         * @param nSoundId The corresponding sound id loaded by @see Load function;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Play( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool    bRet = false;

            if( itItem != m_SoundMap.end() )
                bRet = itItem -> second -> Play();

            return bRet;
        }

        /**
         * @brief Proovide sound stop method on target engine;
         * 
         * @param nSoundId The corresponding sound id loaded by @see Load function;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Stop( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool    bRet = false;

            if( itItem != m_SoundMap.end() )
                bRet = itItem -> second -> Stop();

            return bRet;
        }

        /**
         * @brief Proovide sound pause method on target engine;
         * 
         * @param nSoundId The corresponding sound id loaded by @see Load function;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Pause( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool    bRet = false;

            if( itItem != m_SoundMap.end() )
                bRet = itItem -> second -> Pause();

            return bRet;
        }
        /**
         * @brief Proovide sound resume method on target engine;
         * 
         * @param nSoundId The corresponding sound id loaded by @see Load function;
         * @return true If operation was succesfull;
         * @return false If operation was failed;
         */
        bool SoundManager :: Resume( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool    bRet = false;

            if( itItem != m_SoundMap.end() )
                bRet = itItem -> second -> Resume();

            return bRet;
        }

        /**
         * @brief Check if audio data previously loaded is playing;
         * 
         * @param nSoundId The previous sound id loaded by @see Load function;
         * @return true If is playing;
         * @return false If is not playing;
         */
        bool SoundManager :: IsPlaying( int nSoundId )  {

            std :: map<int, ISound*> :: iterator itItem = m_SoundMap.find( nSoundId );
            bool    bRet = false;

            if( itItem != m_SoundMap.end() )
                bRet = itItem -> second -> IsPlaying();

            return bRet;
        }
    }
}
