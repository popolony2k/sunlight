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

/*
 * ReadImageSize: pixel size straight from PNG/JPEG header bytes (what the
 * null engine's LoadTexture relies on). Headers are built by hand so every
 * edge - truncation, fill bytes, non-SOF markers - is exercised exactly.
 */

#include <doctest/doctest.h>
#include <vector>
#include <cstdint>
#include "backends/null/imageheader.h"

using SunLight :: Engines :: Null :: ReadImageSize;

namespace  {

    typedef std :: vector<unsigned char>  Bytes;

    void Append32( Bytes &b, unsigned n )  {
        b.push_back( ( n >> 24 ) & 0xFF );
        b.push_back( ( n >> 16 ) & 0xFF );
        b.push_back( ( n >> 8 ) & 0xFF );
        b.push_back( n & 0xFF );
    }

    void Append16( Bytes &b, unsigned n )  {
        b.push_back( ( n >> 8 ) & 0xFF );
        b.push_back( n & 0xFF );
    }

    // Signature + IHDR (length 13, type, width, height, 5 detail bytes).
    Bytes MakePng( unsigned nWidth, unsigned nHeight )  {

        Bytes  b = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

        Append32( b, 13 );
        b.push_back( 'I' ); b.push_back( 'H' ); b.push_back( 'D' ); b.push_back( 'R' );
        Append32( b, nWidth );
        Append32( b, nHeight );
        for( int n = 0; n < 5; n++ )
            b.push_back( 0 );

        return b;
    }

    // SOI, an APP0 segment (to be skipped), then the given SOF marker.
    Bytes MakeJpeg( unsigned nWidth, unsigned nHeight, unsigned char nSofMarker = 0xC0 )  {

        Bytes  b = { 0xFF, 0xD8 };

        // APP0: length 16 = 2 + 14 payload bytes
        b.push_back( 0xFF ); b.push_back( 0xE0 );
        Append16( b, 16 );
        for( int n = 0; n < 14; n++ )
            b.push_back( 0 );

        b.push_back( 0xFF ); b.push_back( nSofMarker );
        Append16( b, 17 );          // 8 + 3 components x 3
        b.push_back( 8 );           // precision
        Append16( b, nHeight );
        Append16( b, nWidth );
        for( int n = 0; n < 9; n++ )
            b.push_back( 0 );

        return b;
    }

    bool Read( const Bytes &b, int &w, int &h )  {
        return ReadImageSize( b.data(), b.size(), w, h );
    }
}

TEST_SUITE( "backends/null/ReadImageSize (PNG)" )  {

    TEST_CASE( "Reads width and height from the IHDR chunk" )  {

        int  w = 0, h = 0;

        CHECK( Read( MakePng( 96, 64 ), w, h ) == true );
        CHECK( w == 96 );
        CHECK( h == 64 );

        CHECK( Read( MakePng( 1, 1 ), w, h ) == true );
        CHECK( w == 1 );
        CHECK( h == 1 );

        // Width and height are different fields, not mixed up, and big
        // values (> 16 bits) survive.
        CHECK( Read( MakePng( 70000, 3 ), w, h ) == true );
        CHECK( w == 70000 );
        CHECK( h == 3 );
    }

    TEST_CASE( "Rejects a truncated header, a wrong signature, a missing IHDR, zero and oversized dimensions" )  {

        int    w = 7, h = 7;
        Bytes  full = MakePng( 32, 32 );

        Bytes  truncated( full.begin(), full.begin() + 23 );
        CHECK( Read( truncated, w, h ) == false );

        Bytes  wrongSig = full;
        wrongSig[1] = 'X';
        CHECK( Read( wrongSig, w, h ) == false );

        Bytes  wrongChunk = full;
        wrongChunk[12] = 'X';
        CHECK( Read( wrongChunk, w, h ) == false );

        CHECK( Read( MakePng( 0, 32 ), w, h ) == false );
        CHECK( Read( MakePng( 32, 0 ), w, h ) == false );
        CHECK( Read( MakePng( 0x80000000u, 32 ), w, h ) == false );

        // A failed read leaves the outputs alone.
        CHECK( w == 7 );
        CHECK( h == 7 );
    }
}

TEST_SUITE( "backends/null/ReadImageSize (JPEG)" )  {

    TEST_CASE( "Reads height then width from the first SOF marker, skipping earlier segments" )  {

        int  w = 0, h = 0;

        CHECK( Read( MakeJpeg( 640, 480 ), w, h ) == true );
        CHECK( w == 640 );
        CHECK( h == 480 );
    }

    TEST_CASE( "Progressive and other SOF variants count; DHT, JPG and DAC markers do not" )  {

        int  w = 0, h = 0;

        CHECK( Read( MakeJpeg( 320, 200, 0xC2 ), w, h ) == true );   // progressive
        CHECK( w == 320 );
        CHECK( h == 200 );

        CHECK( Read( MakeJpeg( 320, 200, 0xC1 ), w, h ) == true );   // extended sequential

        // 0xC4 (DHT), 0xC8 (JPG), 0xCC (DAC) carry no frame size: skipped
        // by length, and with nothing after them the search runs dry.
        CHECK( Read( MakeJpeg( 320, 200, 0xC4 ), w, h ) == false );
        CHECK( Read( MakeJpeg( 320, 200, 0xC8 ), w, h ) == false );
        CHECK( Read( MakeJpeg( 320, 200, 0xCC ), w, h ) == false );
    }

    TEST_CASE( "Tolerates fill bytes, standalone markers and a DHT before the frame header" )  {

        int    w = 0, h = 0;
        Bytes  b = { 0xFF, 0xD8, 0xFF, 0xFF, 0xD0 };   // SOI, fill byte, RST0 (no length)

        // A DHT segment (length 4 = 2 + 2 payload) to skip.
        b.push_back( 0xFF ); b.push_back( 0xC4 ); Append16( b, 4 ); b.push_back( 0 ); b.push_back( 0 );

        // Then the frame header.
        Bytes  sof = MakeJpeg( 1024, 768 );
        b.insert( b.end(), sof.begin() + 20, sof.end() );   // skip SOI(2) + APP0(2+16)

        CHECK( Read( b, w, h ) == true );
        CHECK( w == 1024 );
        CHECK( h == 768 );
    }

    TEST_CASE( "Rejects truncation, corruption, an early EOI and zero dimensions" )  {

        int    w = 0, h = 0;
        Bytes  full = MakeJpeg( 640, 480 );

        CHECK( Read( Bytes( full.begin(), full.begin() + 2 ), w, h ) == false );    // SOI only
        CHECK( Read( Bytes( full.begin(), full.begin() + 23 ), w, h ) == false );   // cut inside SOF
        CHECK( Read( MakeJpeg( 0, 480 ), w, h ) == false );
        CHECK( Read( MakeJpeg( 640, 0 ), w, h ) == false );

        Bytes  corrupt = full;
        corrupt[2] = 0x12;   // where the first marker's 0xFF should be
        CHECK( Read( corrupt, w, h ) == false );

        Bytes  earlyEoi = { 0xFF, 0xD8, 0xFF, 0xD9 };
        CHECK( Read( earlyEoi, w, h ) == false );

        // A segment whose declared length runs off the end.
        Bytes  overrun = { 0xFF, 0xD8, 0xFF, 0xE0, 0xFF, 0xF0 };
        CHECK( Read( overrun, w, h ) == false );
    }
}

TEST_SUITE( "backends/null/ReadImageSize (other)" )  {

    TEST_CASE( "Unrecognised formats, empty and null input are rejected" )  {

        int    w = 0, h = 0;
        Bytes  gif = { 'G', 'I', 'F', '8', '9', 'a', 1, 0, 1, 0 };

        CHECK( Read( gif, w, h ) == false );
        CHECK( Read( Bytes(), w, h ) == false );
        CHECK( Read( Bytes( 1, 0x89 ), w, h ) == false );
        CHECK( ReadImageSize( nullptr, 100, w, h ) == false );
    }
}
