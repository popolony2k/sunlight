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

#include "backends/raylib/raylibsound.h"
#include "filesystem/filesystemfactory.h"

#include <filesystem>


namespace SunLight {
    namespace Sound  {
        namespace RayLib  {

            int  RayLibSound :: m_nReferenceCount  = 0;


            /**
             * @brief First call initialization.
             */
            void RayLibSound :: Initialize( void )  {

                if( m_nReferenceCount == 0 )  {
                    InitAudioDevice();
                }

                m_nReferenceCount++;
                m_Loaded = false;
            }

            /**
             * @brief Last call finalization.
             */
            void RayLibSound :: Finalize( void )  {

                Unload();

                if( m_nReferenceCount > 0 )  {
                    m_nReferenceCount--;

                    if( m_nReferenceCount == 0 )
                        CloseAudioDevice();
                }

                m_Loaded = false;
            }

            /**
             * @brief Constructor. Initialize all class data.
             */
            RayLibSound :: RayLibSound( void )  {

                Initialize();
            }

            /**
             * @brief Destructor. Finalize all class data.
             */
            RayLibSound :: ~RayLibSound( void )  {

                Finalize();
            }

            /**
             * @brief Implements audio data loading based on
             * raylib target engine;
             *
             * Reads the file via SunLight::FileSystem directly rather
             * than raylib's own path-based ::LoadSound() - confirmed by
             * reading raudio.c's own source directly that it has it's OWN
             * private, static copy of LoadFileData that always reads
             * straight from the OS filesystem, bypassing
             * SetLoadFileDataCallback entirely (unlike texture loading,
             * which honors that hook - see RaylibEngine's own
             * FileSystemLoadFileDataCallback comment). ::LoadSound()
             * itself is just ::LoadSoundFromWave(::LoadWave(path))
             * internally, so calling LoadWaveFromMemory() (which takes
             * the raw bytes plus the file's own extension, to pick the
             * right decoder) + LoadSoundFromWave() reproduces it exactly,
             * just sourced from SunLight::FileSystem instead of a raw
             * fopen().
             *
             * @param strFileName The audio data file name to load;
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool  RayLibSound :: Load( std :: string strFileName )  {

                if( m_Loaded )
                  Unload();

                std :: vector<unsigned char>  data;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strFileName, data ) )  {
                    m_Loaded = false;

                    return false;
                }

                std :: string  strExtension = std :: filesystem :: path( strFileName ).extension().string();
                Wave           wave         = ::LoadWaveFromMemory( strExtension.c_str(), data.data(), ( int ) data.size() );

                m_Sound = ::LoadSoundFromWave( wave );
                m_Loaded = ( m_Sound.stream.buffer != NULL );

                ::UnloadWave( wave );

                return m_Loaded;
            }

            /**
             * @brief Implements unload previous data loaded by @see Load() 
             * method on rayib target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: Unload( void )  {

                if( m_Loaded )  {
                    ::UnloadSound( m_Sound );
                    m_Loaded = false;

                    return true;
                }

                return false;
            }

            /**
             * @brief Implements sound playing method on chosen target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: Play( void )  {

                if( m_Loaded )  {
                    ::PlaySound( m_Sound );
                    return true;
                }

                return false;
            }

            /**
             * @brief Implements sound stop method on chosen target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: Stop( void )  {

                if( m_Loaded )  {
                    ::StopSound( m_Sound );
                    return true;
                }

                return false;
            }

            /**
             * @brief Implements sound pause method on chosen target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: Pause( void )  {

                if( m_Loaded )  {
                    ::PauseSound( m_Sound );
                    return true;
                }

                return false;
            }

            /**
             * @brief Implements sound resume method on chosen target engine;
             * 
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: Resume( void )  {

                if( m_Loaded )  {
                    ::ResumeSound( m_Sound );
                    return true;
                }

                return false;
            }

            /**
             * @brief Check if audio data previously loaded is playing;
             * 
             * @return true If is playing;
             * @return false If is not playing;
             */
            bool RayLibSound :: IsPlaying( void )  {

                if( m_Loaded )  {
                    return ::IsSoundPlaying( m_Sound );
                }

                return false;
            }

            /**
             * @brief Implements per-sound volume control on chosen target
             * engine;
             *
             * @param fVolume Volume level, 0.0 (silent) to 1.0 (max);
             * @return true If operation was succesfull;
             * @return false If operation was failed;
             */
            bool RayLibSound :: SetVolume( float fVolume )  {

                if( m_Loaded )  {
                    ::SetSoundVolume( m_Sound, fVolume );
                    return true;
                }

                return false;
            }
        }
    }
}
