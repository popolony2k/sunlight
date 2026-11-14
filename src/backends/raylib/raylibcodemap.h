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

#ifndef __RAYLIBCODEMAP_H__
#define __RAYLIBCODEMAP_H__

#include <array>
#include <cstddef>


namespace SunLight  {
    namespace Input  {
        namespace RayLib  {

            // The translation tables between sunlight's key, button and axis codes and raylib's,
            // shared by RayLibInputHandler (keys) and RayLibGamepadInput (buttons and axes).

            // One translation: our code and raylib's code for the same key, button or axis.
            struct CodePair  {
                int nOurs;
                int nRaylib;
            };

            // Both directions of one translation table: toRaylib is indexed by our code, fromRaylib
            // by raylib's code. A code that is not mapped is 0.
            template<std :: size_t N>
            struct CodeMap  {
                std :: array<int, N>  toRaylib;
                std :: array<int, N>  fromRaylib;
            };

            // Builds both directions from one list of pairs, at compile time. A code out of range, or a
            // code used by two pairs, throws, which makes a constexpr table a compile error.
            template<std :: size_t N, std :: size_t M>
            constexpr CodeMap<N> BuildCodeMap( const CodePair ( &pairs )[M] )  {

                CodeMap<N>            map {};
                std :: array<bool, N> seenOurs {};
                std :: array<bool, N> seenRaylib {};

                for( const CodePair& p : pairs )  {
                    if( ( p.nOurs < 0 ) || ( p.nOurs >= ( int ) N ) || ( p.nRaylib < 0 ) || ( p.nRaylib >= ( int ) N ) )
                        throw "a code is out of the table's range";

                    if( seenOurs[p.nOurs] || seenRaylib[p.nRaylib] )
                        throw "a code appears in two pairs";

                    seenOurs[p.nOurs]         = true;
                    seenRaylib[p.nRaylib]     = true;
                    map.toRaylib[p.nOurs]     = p.nRaylib;
                    map.fromRaylib[p.nRaylib] = p.nOurs;
                }

                return map;
            }

            // Our code -> raylib's code. A code outside the table gives 0.
            template<std :: size_t N>
            constexpr int ToRaylib( const CodeMap<N>& map, int nCode )  {

                return ( ( nCode >= 0 ) && ( nCode < ( int ) N ) ) ? map.toRaylib[nCode] : 0;
            }

            // raylib's code -> ours. A raylib code outside the table gives 0.
            template<std :: size_t N>
            constexpr int FromRaylib( const CodeMap<N>& map, int nRaylibCode )  {

                return ( ( nRaylibCode >= 0 ) && ( nRaylibCode < ( int ) N ) ) ? map.fromRaylib[nRaylibCode] : 0;
            }
        }
    }
}

#endif /* __RAYLIBCODEMAP_H__ */
