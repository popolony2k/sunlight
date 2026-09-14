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

#include "colliderregistry.h"


namespace SunLight {
    namespace Collision  {

        /**
         * Access the single process-wide registry instance.
         */
        ColliderRegistry& ColliderRegistry :: Instance( void )  {

            static ColliderRegistry  instance;

            return instance;
        }

        /**
         * Register a newly constructed Collider and obtain a handle for it.
         * @param pCollider The collider being registered;
         */
        ColliderHandle ColliderRegistry :: Register( Collider *pCollider )  {

            std :: uint32_t  nIndex;

            if( !m_FreeIndices.empty() )  {
                nIndex = m_FreeIndices.back();
                m_FreeIndices.pop_back();
            }
            else  {
                nIndex = ( std :: uint32_t ) m_Slots.size();
                m_Slots.emplace_back();
            }

            Slot&  slot = m_Slots[nIndex];

            // m_nNextGeneration is a monotonically increasing counter, never
            // reused across slots; only a wrap of a 32-bit counter (billions
            // of collider registrations over a process lifetime) could ever
            // make two live generations collide, which is not a practical
            // concern here.
            slot.pPtr        = pCollider;
            slot.nGeneration = m_nNextGeneration++;
            slot.bAlive      = true;

            return ColliderHandle{ nIndex, slot.nGeneration };
        }

        /**
         * Retire a handle: the Collider it referred to is no longer alive.
         * @param handle The handle to retire;
         */
        void ColliderRegistry :: Retire( const ColliderHandle& handle )  {

            if( !handle.IsValid() || ( handle.nIndex >= m_Slots.size() ) )
                return;

            Slot&  slot = m_Slots[handle.nIndex];

            if( slot.bAlive && ( slot.nGeneration == handle.nGeneration ) )  {
                slot.bAlive = false;
                slot.pPtr   = nullptr;

                m_FreeIndices.push_back( handle.nIndex );
            }
        }

        /**
         * Resolve a handle back to its Collider pointer.
         * @param handle The handle to resolve;
         */
        Collider* ColliderRegistry :: Resolve( const ColliderHandle& handle ) const  {

            if( !handle.IsValid() || ( handle.nIndex >= m_Slots.size() ) )
                return nullptr;

            const Slot&  slot = m_Slots[handle.nIndex];

            if( slot.bAlive && ( slot.nGeneration == handle.nGeneration ) )
                return slot.pPtr;

            return nullptr;
        }
    }
}
