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

#include "input/gamepadidentitymatcher.h"

namespace SunLight  {
    namespace Input  {

        std :: vector<int> MatchGamepadsByName( const std :: vector<std :: string>& targetNames,
                                                 const std :: vector<std :: string>& candidateNames )  {

            // Exactly one candidate on each side: the only thing it could be, regardless of
            // whether the two names agree (see this function's own doc comment for why they
            // can legitimately disagree even for a single, correctly-detected device).
            if( ( targetNames.size() == 1 ) && ( candidateNames.size() == 1 ) )
                return { 0 };

            std :: vector<int>   matches( targetNames.size(), -1 );
            std :: vector<bool>  consumed( candidateNames.size(), false );

            for( size_t nTarget = 0; nTarget < targetNames.size(); nTarget++ )  {
                for( size_t nCandidate = 0; nCandidate < candidateNames.size(); nCandidate++ )  {
                    if( !consumed[ nCandidate ] && ( candidateNames[ nCandidate ] == targetNames[ nTarget ] ) )  {
                        matches[ nTarget ]    = ( int ) nCandidate;
                        consumed[ nCandidate ] = true;
                        break;
                    }
                }
            }

            return matches;
        }
    }
}
