/*
 * color.h
 *
 *  Created on: Jul 12, 2021
 *      Author: popolony2k
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
