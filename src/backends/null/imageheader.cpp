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

#include "backends/null/imageheader.h"


namespace SunLight  {
    namespace Engines  {
        namespace Null  {

            static unsigned ReadBigEndian32( const unsigned char *p )  {

                return ( ( unsigned ) p[0] << 24 ) | ( ( unsigned ) p[1] << 16 ) | ( ( unsigned ) p[2] << 8 ) | ( unsigned ) p[3];
            }

            static unsigned ReadBigEndian16( const unsigned char *p )  {

                return ( ( unsigned ) p[0] << 8 ) | ( unsigned ) p[1];
            }

            /**
             * PNG: 8-byte signature, then the first chunk must be IHDR
             * (4-byte length, "IHDR", width u32 BE, height u32 BE) - so
             * width/height sit at offsets 16 and 20.
             */
            static bool ReadPngSize( const unsigned char *pData, size_t nSize, int &nWidth, int &nHeight )  {

                static const unsigned char  szSignature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

                if( nSize < 24 )
                    return false;

                for( int nCount = 0; nCount < 8; nCount++ )  {
                    if( pData[nCount] != szSignature[nCount] )
                        return false;
                }

                if( ( pData[12] != 'I' ) || ( pData[13] != 'H' ) || ( pData[14] != 'D' ) || ( pData[15] != 'R' ) )
                    return false;

                unsigned  nW = ReadBigEndian32( pData + 16 );
                unsigned  nH = ReadBigEndian32( pData + 20 );

                // Zero is illegal in PNG; anything past INT_MAX can't be
                // represented in the int outputs.
                if( ( nW == 0 ) || ( nH == 0 ) || ( nW > 0x7FFFFFFFu ) || ( nH > 0x7FFFFFFFu ) )
                    return false;

                nWidth  = ( int ) nW;
                nHeight = ( int ) nH;

                return true;
            }

            /**
             * JPEG: SOI (FF D8), then a sequence of marker segments. The
             * first Start-Of-Frame marker (FF C0..CF except C4 = DHT, C8 =
             * JPG extension and CC = DAC) carries: length u16, precision u8,
             * height u16, width u16. Every other marker with a payload is
             * skipped by its own length; standalone markers (SOI, EOI,
             * RSTn, TEM) have none.
             */
            static bool ReadJpegSize( const unsigned char *pData, size_t nSize, int &nWidth, int &nHeight )  {

                if( ( nSize < 4 ) || ( pData[0] != 0xFF ) || ( pData[1] != 0xD8 ) )
                    return false;

                size_t  nPos = 2;

                while( nPos + 1 < nSize )  {
                    if( pData[nPos] != 0xFF )
                        return false;   // not on a marker boundary: corrupt

                    // Skip any 0xFF fill bytes before the marker code.
                    while( ( nPos < nSize ) && ( pData[nPos] == 0xFF ) )
                        nPos++;

                    if( nPos >= nSize )
                        return false;

                    unsigned char  nMarker = pData[nPos++];

                    // Standalone markers: no length/payload.
                    if( ( nMarker == 0x01 ) || ( ( nMarker >= 0xD0 ) && ( nMarker <= 0xD9 ) ) )  {
                        if( nMarker == 0xD9 )
                            return false;   // EOI before any SOF

                        continue;
                    }

                    if( nPos + 2 > nSize )
                        return false;

                    unsigned  nLength = ReadBigEndian16( pData + nPos );

                    if( nLength < 2 )
                        return false;

                    bool  bIsSof = ( nMarker >= 0xC0 ) && ( nMarker <= 0xCF ) &&
                                   ( nMarker != 0xC4 ) && ( nMarker != 0xC8 ) && ( nMarker != 0xCC );

                    if( bIsSof )  {
                        // length(2) + precision(1) + height(2) + width(2)
                        if( ( nLength < 7 ) || ( nPos + 7 > nSize ) )
                            return false;

                        unsigned  nH = ReadBigEndian16( pData + nPos + 3 );
                        unsigned  nW = ReadBigEndian16( pData + nPos + 5 );

                        if( ( nW == 0 ) || ( nH == 0 ) )
                            return false;

                        nWidth  = ( int ) nW;
                        nHeight = ( int ) nH;

                        return true;
                    }

                    nPos += nLength;
                }

                return false;
            }

            /**
             * Recognise the format by magic bytes, then read its size.
             */
            bool ReadImageSize( const unsigned char *pData, size_t nSize, int &nWidth, int &nHeight )  {

                if( ( pData == nullptr ) || ( nSize < 2 ) )
                    return false;

                if( pData[0] == 0x89 )
                    return ReadPngSize( pData, nSize, nWidth, nHeight );

                if( ( pData[0] == 0xFF ) && ( pData[1] == 0xD8 ) )
                    return ReadJpegSize( pData, nSize, nWidth, nHeight );

                return false;
            }
        }
    }
}
