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

#ifndef __BLENDMODE_H__
#define __BLENDMODE_H__


namespace SunLight  {
    namespace Base  {

        /**
         * @brief How what is drawn next combines with what is already drawn (I2 of the master plan).
         * Each mode is a formula on the colour channels, written for a source colour s with alpha a
         * drawn over a destination colour d; in every mode the destination's alpha is combined as
         * "over" (a + d.alpha x (1 - a)), so drawing over an opaque pixel leaves it opaque (see
         * IEngine::BeginRenderTarget).
         *
         * Always qualify it as SunLight::Base::BlendMode: raylib declares a global BlendMode enum of
         * its own (raylib.h), the same trap as SunLight::Input::KeyboardKey.
         */
        enum BlendMode  {
            BLEND_MODE_NORMAL,      // s x a + d x (1 - a): the default
            BLEND_MODE_ADD,         // d + s x a: lights, glows
            BLEND_MODE_MULTIPLY,    // d x (s x a + 1 - a): shadows, tints; a transparent pixel leaves d as is
            BLEND_MODE_SUBTRACT     // d - s x a: darkening by a colour
        };
    }
}

#endif /* __BLENDMODE_H__ */
