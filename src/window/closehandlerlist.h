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

#ifndef __CLOSEHANDLERLIST_H__
#define __CLOSEHANDLERLIST_H__

#include <functional>
#include <vector>


namespace SunLight {
    namespace Window  {

        /**
         * @brief Ordered list of "window is about to close" handlers - the
         * shared implementation behind IWindow::AddCloseHandler /
         * RemoveCloseHandler / firing, so every IWindow backend (raylib,
         * the mock, a future null window) gets identical semantics
         * instead of each re-implementing them.
         *
         * Handlers fire in registration order. Fire() works on a copy of
         * the list, so a handler may itself Add() or Remove() handlers
         * (including removing itself) without invalidating the iteration;
         * a handler removed by an earlier handler in the same Fire() call
         * is skipped rather than called after it was removed. Handlers
         * must not throw.
         */
        class CloseHandlerList  {

            public:

            typedef std :: function<void( void )>  Handler;

            int  Add( const Handler &handler );
            void Remove( int nId );
            void Fire( void );

            private:

            struct Entry  {
                int      nId;
                Handler  handler;
            };

            std :: vector<Entry>  m_Entries;
            int                   m_nNextId = 1;
        };
    }
}

#endif  /* __CLOSEHANDLERLIST_H__ */
