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

#ifndef __NULLIMAGEHEADER_H__
#define __NULLIMAGEHEADER_H__

#include <cstddef>


namespace SunLight  {
    namespace Engines  {
        namespace Null  {

            /**
             * @brief Read an image's pixel size straight from its header
             * bytes - no decoding, no GPU, no third-party library. What the
             * null engine's LoadTexture needs so sprite/tileset sizes (and
             * everything derived from them: bounds, collision insets,
             * centering) are real in a headless run.
             *
             * The format is recognised by its magic bytes, never by the
             * file extension - the real backend content-sniffs the same way,
             * and real assets do rely on that (e.g. a JPEG stored in a
             * ".png" file). Supported: PNG (IHDR) and JPEG (the first SOF
             * marker, baseline or progressive). Anything else, or a
             * truncated/corrupt header, is rejected.
             *
             * @param pData The file's bytes;
             * @param nSize Number of bytes at pData;
             * @param nWidth Receives the width in pixels on success;
             * @param nHeight Receives the height in pixels on success;
             * @return true if the size was read, false if the data isn't a
             * recognised, well-formed PNG/JPEG header (outputs untouched);
             */
            bool ReadImageSize( const unsigned char *pData, size_t nSize, int &nWidth, int &nHeight );
        }
    }
}

#endif  /* __NULLIMAGEHEADER_H__ */
