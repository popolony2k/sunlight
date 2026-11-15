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

#include "general/helper.h"
#include <cmath>
#include <cstdlib>


namespace SunLight  {
    namespace General  {
        /**
         * @brief Generate a random number between specified range;
         *
         * @param nBegin Initial range value;
         * @param nEnd Final range value;
         * @return the generated number;
         */
        int Helper :: Random( int nBegin, int nEnd )  {

            return ( nBegin + ( std :: rand() % ( std :: abs( nEnd ) - std :: abs( nBegin ) + 1 ) ) );
        }

        /**
         * @brief Round to the nearest whole number, ties rounding up (toward +infinity),
         * never std::round's "away from zero" - which rounds a negative tie DOWN (more
         * negative) while a positive tie rounds UP, flipping direction exactly at zero. Two
         * screen positions straddling zero that are both exactly N.5 (a real case: an
         * integer map position times a zoom whose fraction, applied to that position, lands
         * on an exact half-pixel - found live at zoom 3.8125, 61/16, against small camera
         * offsets) would then round AWAY from each other, opening a 1px gap between two tiles
         * that are otherwise exactly, mathematically contiguous. std::floor(x + 0.5) has no
         * such flip - already this project's own convention for exactly this reason, see
         * Shape::ScreenLineWidth's own "half-up rounding" comment.
         */
        float Helper :: RoundHalfUp( float fValue )  {

            return std :: floor( fValue + 0.5f );
        }
    }
}