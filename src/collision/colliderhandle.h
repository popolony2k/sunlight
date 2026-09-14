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

#ifndef __COLLIDERHANDLE_H__
#define __COLLIDERHANDLE_H__

#include <cstdint>


namespace SunLight {
    namespace Collision  {

        /**
         * @brief Lightweight, safe-to-store identity for a Collider.
         *
         * A ColliderHandle never dereferences the Collider it refers to by
         * itself; it must be resolved through ColliderRegistry, which can
         * tell a still-alive collider from one whose owning object has since
         * been destroyed, without ever touching the (possibly freed) memory
         * the raw pointer used to point to. See ColliderRegistry for the
         * mechanics.
         *
         * nGeneration == 0 is reserved to mean "invalid handle" (e.g. a
         * default-constructed one that was never registered).
         */
        struct ColliderHandle  {

            std :: uint32_t  nIndex      = 0;
            std :: uint32_t  nGeneration = 0;

            bool IsValid( void ) const  {
                return nGeneration != 0;
            }

            bool operator == ( const ColliderHandle& other ) const  {
                return ( nIndex == other.nIndex ) && ( nGeneration == other.nGeneration );
            }

            bool operator != ( const ColliderHandle& other ) const  {
                return !( *this == other );
            }
        };
    }
}

#endif /* __COLLIDERHANDLE_H__ */
