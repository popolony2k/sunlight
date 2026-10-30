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

#include "base/color.h"


namespace SunLight  {
    namespace Base  {

        namespace  {

            const unsigned  __CHANNEL_MAX = 0xFF;   // a colour channel's full value
        }

        /**
         * @brief A colour's RGB multiplied by a tint's, channel by channel, rounded to nearest:
         * c x t / 255, so white (255) leaves a channel exactly as it is, and black (0) clears it.
         * Moved here from the renderer's shape helpers (I10 of the master plan) so sprites use the
         * same one as layers.
         * @param color The colour (its alpha is kept);
         * @param tint The tint (its alpha is not used);
         * @return The tinted colour;
         */
        stColor MultiplyTint( stColor color, stColor tint )  {

            auto  Channel = []( unsigned char c, unsigned char t )  {
                return ( unsigned char ) ( ( ( unsigned ) c * t + __CHANNEL_MAX / 2 ) / __CHANNEL_MAX );
            };

            return stColor { Channel( color.nRed, tint.nRed ), Channel( color.nGreen, tint.nGreen ),
                             Channel( color.nBlue, tint.nBlue ), color.nAlpha };
        }
    }
}
