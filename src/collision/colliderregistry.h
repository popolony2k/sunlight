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

#ifndef __COLLIDERREGISTRY_H__
#define __COLLIDERREGISTRY_H__

#include <deque>
#include <vector>
#include "colliderhandle.h"


namespace SunLight {
    namespace Collision  {

        class Collider;

        /**
         * @brief Stable indirection table between ColliderHandle values and
         * the Collider objects they refer to.
         *
         * The problem this solves: CollisionManager::Update() may still be
         * holding a snapshotted Collider* when an ICollisionListener::
         * OnCollision() callback fired earlier in that same Update() call
         * destroys the Collider it refers to (or the object that owns it).
         * At that point the pointer is dangling, and there is no way to
         * safely ask *it* whether it is still alive without dereferencing
         * already-freed memory - a self-reported "am I alive" flag on
         * Collider itself doesn't help, because reading it requires
         * dereferencing the very pointer that may be dangling.
         *
         * This does NOT contradict this codebase's usual "raw pointer for a
         * non-owning reference" convention (see CLAUDE.md's "Pointer
         * ownership convention" - CollisionManager tracking a bare
         * `Collider*` is called out there by name as correct). A
         * ColliderHandle still doesn't own the Collider it refers to -
         * ColliderRegistry never constructs or destroys one - it's purely a
         * safe-to-hold lookup token standing in for the raw pointer, for
         * the one specific operation (dereferencing it after the fact,
         * inside Update()) that convention doesn't make safe by itself.
         *
         * ColliderRegistry sidesteps the dangling-pointer problem by never
         * handing out raw pointers as the long-lived identity. Instead:
         *   - Register() hands back a ColliderHandle {index, generation}
         *     pointing at a slot in a std::deque, whose storage is never
         *     freed while the registry exists (deque never invalidates
         *     other elements on push_back, and slots are only ever
         *     recycled, never erased).
         *   - Collider's destructor calls Retire() on its own handle as
         *     its last act. This is always safe - a destructor runs on
         *     still-valid memory - and it happens before that memory is
         *     actually freed, whether the Collider is destroyed normally,
         *     synchronously mid-Update(), or any other way.
         *   - Resolve() reads the slot (always safe: the slot itself is
         *     never freed) and only returns the Collider* if the slot is
         *     still marked alive AND its generation matches the handle's -
         *     so a stale handle whose slot has since been recycled for an
         *     unrelated Collider is correctly rejected, not confused with
         *     the new occupant.
         *
         * The registry does NOT protect against destroying a Collider
         * through means that skip its destructor (e.g. placement-new reuse
         * without calling the destructor first, or freeing the underlying
         * storage directly) - normal C++ object lifetime rules still apply.
         *
         * Single-threaded by design, matching CollisionManager::Update()'s
         * current usage; not safe to call concurrently from multiple
         * threads without external synchronization.
         */
        class ColliderRegistry  {

            struct Slot  {
                Collider          *pPtr        = nullptr;
                std :: uint32_t   nGeneration  = 0;
                bool              bAlive       = false;
            };

            std :: deque<Slot>             m_Slots;
            std :: vector<std :: uint32_t> m_FreeIndices;
            std :: uint32_t                m_nNextGeneration = 1;

            ColliderRegistry( void ) = default;

            public:

            ColliderRegistry( const ColliderRegistry& ) = delete;
            ColliderRegistry& operator = ( const ColliderRegistry& ) = delete;

            static ColliderRegistry& Instance( void );

            /**
             * Register a newly constructed Collider and obtain a handle
             * for it. Called once from Collider's constructor.
             * @param pCollider The collider being registered. Must not be
             * null;
             */
            ColliderHandle Register( Collider *pCollider );

            /**
             * Retire a handle: the Collider it referred to is no longer
             * alive. Called once from Collider's destructor. Safe to call
             * with an already-invalid/retired handle (no-op).
             * @param handle The handle to retire;
             */
            void Retire( const ColliderHandle& handle );

            /**
             * Resolve a handle back to its Collider pointer.
             * @param handle The handle to resolve;
             * @return The live Collider* if handle refers to a still-alive
             * collider, nullptr otherwise (retired, or stale/generation
             * mismatch).
             */
            Collider* Resolve( const ColliderHandle& handle ) const;
        };
    }
}

#endif /* __COLLIDERREGISTRY_H__ */
