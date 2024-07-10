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

#ifndef __COLOR_H__
#define __COLOR_H__


namespace SunLight  {
    namespace Base  {
        /**
         * Color definition structure.
         */
        struct stColor  {
            unsigned char  nRed;
            unsigned char  nGreen;
            unsigned char  nBlue;
            unsigned char  nAlpha;
        };
    }
}

/**
 * Pre-defined colors.
 */
#define WHITE_COLOR   SunLight :: Base :: stColor  { 255, 255, 255, 255 }
#define BLACK_COLOR   SunLight :: Base :: stColor  { 0, 0, 0, 255 }

#endif /* __COLOR_H__ */
