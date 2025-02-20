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

#include "object.h"


namespace SunLight  {
    namespace Base  {

        /**
         * @brief Constructor. Initialize all class data.
         * 
         */
        Object :: Object( void )  {
            m_pPtrData = nullptr;
        }

        /**
         * @brief Destructor. Finalize all class data.
         */
        Object :: ~Object( void )  {

        }

        /**
         * @brief Set a pointer to object.
         * 
         * @param ptrData The pointer to set;
         */
        void Object :: SetPtrData( VoidPtr ptrData )  {
            
            m_pPtrData = ptrData;
        }

        /**
         * @brief Get the Ptr Data stored on object;
         * 
         * @return VoidPtr The pointer previsously set by @see SetPtrData
         * method;
         */
        Object :: VoidPtr Object :: GetPtrData( void )  {
            
            return m_pPtrData;
        }
    }
}