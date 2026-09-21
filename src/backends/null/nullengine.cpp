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

#include "backends/null/nullengine.h"
#include "backends/null/imageheader.h"
#include "filesystem/filesystemfactory.h"
#include <cstdint>
#include <vector>

#if defined( __APPLE__ )
    #include <mach-o/dyld.h>
    #include <limits.h>
#elif defined( _WIN32 )
    // windows.h pulls in winuser.h/wingdi.h, which #define names this class
    // also uses as method names (DrawText -> DrawTextA/W) - that would
    // silently rename NullEngine::DrawText in this file only and break the
    // link. Exclude those headers; only kernel32's GetModuleFileNameA is needed.
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOGDI
        #define NOGDI
    #endif
    #ifndef NOUSER
        #define NOUSER
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

namespace SunLight  {
    namespace Engines  {
        namespace Null  {

            /**
             * Everything a "texture" or "render target" is here: just its
             * size. One record type for both, so a render target's texture
             * handle is simply the render target's own handle.
             */
            struct NullSurface  {
                int   nWidth;
                int   nHeight;
            };

            /**
             * @brief "Load" a texture: read the file through
             * SunLight::FileSystem (same virtual-path routing the real
             * backend's loader hook applies), and take its size from the
             * image header. The pixels are never decoded.
             * @param szFileName Texture file name;
             * @param nWidth Receives the width;
             * @param nHeight Receives the height;
             * @return A handle, or nullptr (with 0x0) if the file can't be
             * read or isn't a recognised PNG/JPEG;
             */
            SunLight :: Base :: TextureHandle NullEngine :: LoadTexture( const char *szFileName,
                                                                         int& nWidth,
                                                                         int& nHeight )  {

                std :: vector<unsigned char>  data;
                std :: string                 strVirtualPath = SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( szFileName );
                int                           nReadWidth  = 0;
                int                           nReadHeight = 0;

                nWidth  = 0;
                nHeight = 0;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strVirtualPath, data ) )
                    return nullptr;

                if( !ReadImageSize( data.data(), data.size(), nReadWidth, nReadHeight ) )
                    return nullptr;

                nWidth  = nReadWidth;
                nHeight = nReadHeight;

                return new NullSurface { nReadWidth, nReadHeight };
            }

            /**
             * @brief Release a handle from LoadTexture.
             */
            void NullEngine :: UnloadTexture( SunLight :: Base :: TextureHandle hTexture )  {

                delete reinterpret_cast<NullSurface*>( hTexture );
            }

            /**
             * @brief Drawing is a no-op.
             */
            void NullEngine :: SetPixel( int, int, SunLight :: Base :: stColor )  {
            }

            void NullEngine :: DrawTexture( SunLight :: Base :: TextureHandle, int, int, SunLight :: Base :: stColor )  {
            }

            void NullEngine :: DrawTextureTiled( SunLight :: Base :: TextureHandle,
                                                 SunLight :: Base :: stRectangle,
                                                 SunLight :: Base :: stRectangle,
                                                 SunLight :: Base :: stVector2D,
                                                 float,
                                                 float,
                                                 SunLight :: Base :: stColor )  {
            }

            void NullEngine :: DrawFilledRectangle( int, int, int, int, SunLight :: Base :: stColor )  {
            }

            void NullEngine :: DrawText( const char *, int, int, int, SunLight :: Base :: stColor )  {
            }

            void NullEngine :: ClearBackground( SunLight :: Base :: stColor )  {
            }

            void NullEngine :: DrawFPS( int, int )  {
            }

            void NullEngine :: BeginRenderTarget( SunLight :: Base :: TextureHandle )  {
            }

            void NullEngine :: EndRenderTarget( void )  {
            }

            void NullEngine :: DrawTextureScaled( SunLight :: Base :: TextureHandle,
                                                  SunLight :: Base :: stRectangle,
                                                  SunLight :: Base :: stRectangle,
                                                  SunLight :: Base :: stColor )  {
            }

            /**
             * @brief Succeeds iff the font file exists (nothing is loaded -
             * MeasureText's metric doesn't depend on the font).
             */
            bool NullEngine :: SetFont( const char *szFilePath )  {

                if( ( szFilePath == nullptr ) || ( szFilePath[0] == '\0' ) )
                    return false;

                return SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Exists(
                           SunLight :: FileSystem :: IFileSystem :: ToVirtualPath( szFilePath ) );
            }

            /**
             * @brief Deterministic fixed-metric width: number of characters
             * (UTF-8 code points, not bytes) x nFontSize / 2. Not
             * pixel-accurate to any real font - it exists so layout code
             * that positions things from this value behaves identically on
             * every machine.
             */
            int NullEngine :: MeasureText( const char *szText, int nFontSize )  {

                if( szText == nullptr )
                    return 0;

                long  nCharacters = 0;

                for( const unsigned char *p = ( const unsigned char * ) szText; *p; p++ )  {
                    // Count every byte that isn't a UTF-8 continuation (10xxxxxx).
                    if( ( *p & 0xC0 ) != 0x80 )
                        nCharacters++;
                }

                return ( int ) ( ( nCharacters * ( long ) nFontSize ) / 2 );
            }

            /**
             * @brief Directory of the running executable, trailing
             * separator included (same contract as the real backend), read
             * from the OS directly - no windowing library involved.
             * @return The directory, or an empty string if it can't be
             * determined;
             */
            std :: string NullEngine :: GetApplicationDirectory( void )  {

                std :: string  strPath;

#if defined( __APPLE__ )
                char      szBuffer[PATH_MAX];
                uint32_t  nSize = sizeof( szBuffer );

                if( _NSGetExecutablePath( szBuffer, &nSize ) == 0 )
                    strPath = szBuffer;
#elif defined( _WIN32 )
                char   szBuffer[MAX_PATH];
                DWORD  nLength = GetModuleFileNameA( NULL, szBuffer, MAX_PATH );

                if( ( nLength > 0 ) && ( nLength < MAX_PATH ) )
                    strPath.assign( szBuffer, nLength );
#else
                char     szBuffer[PATH_MAX];
                ssize_t  nLength = readlink( "/proc/self/exe", szBuffer, sizeof( szBuffer ) - 1 );

                if( nLength > 0 )
                    strPath.assign( szBuffer, ( size_t ) nLength );
#endif

                size_t  nLastSeparator = strPath.find_last_of( "/\\" );

                if( nLastSeparator == std :: string :: npos )
                    return std :: string();

                return strPath.substr( 0, nLastSeparator + 1 );
            }

            /**
             * @brief A render target is just a size record.
             */
            SunLight :: Base :: TextureHandle NullEngine :: LoadRenderTarget( int nWidth, int nHeight )  {

                return new NullSurface { nWidth, nHeight };
            }

            /**
             * @brief Release a render target from LoadRenderTarget.
             */
            void NullEngine :: UnloadRenderTarget( SunLight :: Base :: TextureHandle hRenderTarget )  {

                delete reinterpret_cast<NullSurface*>( hRenderTarget );
            }

            /**
             * @brief A render target's texture is the render target's own
             * record (valid for as long as it is).
             */
            SunLight :: Base :: TextureHandle NullEngine :: GetRenderTargetTexture( SunLight :: Base :: TextureHandle hRenderTarget )  {

                return hRenderTarget;
            }
        }
    }
}
