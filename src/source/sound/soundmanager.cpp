/*
 * soundmanager.cpp
 *
 *  Created on: Jan 2, 2024
 *      Author: popolony2k
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
