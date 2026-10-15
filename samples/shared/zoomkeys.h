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

#ifndef __ZOOMKEYS_H__
#define __ZOOMKEYS_H__

#include <functional>
#include "renderer/tilemaprenderer.h"


namespace Samples  {

    typedef std :: function<void( void )>  ZOOM_ACTION;

    /**
     * @brief The zoom keys every sample that zooms shares: + zooms in, - zooms out and = resets the
     * zoom, on the main keyboard and on the keypad. A sample's own zoom keys (Page Up, Page Down,
     * Home, ...) stay as they are; these are added next to them.
     *
     * + and = are one physical key (+ is Shift and =), and raylib reports physical keys: KEY_EQUAL is
     * both, told apart by whether a Shift key is held. The keypad has its own +, - and = keys.
     * @param renderer The renderer the keys are registered with (its input handler reads Shift);
     * @param zoomIn What zooming in does (the renderer's own ZoomIn, or a view's);
     * @param zoomOut What zooming out does;
     * @param resetZoom What resetting the zoom does;
     */
    inline void InstallZoomKeys( SunLight :: Renderer :: TileMapRenderer& renderer,
                                 ZOOM_ACTION zoomIn,
                                 ZOOM_ACTION zoomOut,
                                 ZOOM_ACTION resetZoom )  {

        renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_EQUAL,
                                         [&renderer, zoomIn, resetZoom]( SunLight :: Input :: ControllerType, int )  {
                                             SunLight :: Input :: IInputHandler&  input = renderer.GetInputHandler();

                                             if( input.IsKeyDown( SunLight :: Input :: KEY_LEFT_SHIFT ) ||
                                                 input.IsKeyDown( SunLight :: Input :: KEY_RIGHT_SHIFT ) )
                                                 zoomIn();
                                             else
                                                 resetZoom();
                                         } );
        renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_MINUS,
                                         [zoomOut]( SunLight :: Input :: ControllerType, int ) { zoomOut(); } );
        renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_KP_ADD,
                                         [zoomIn]( SunLight :: Input :: ControllerType, int ) { zoomIn(); } );
        renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_KP_SUBTRACT,
                                         [zoomOut]( SunLight :: Input :: ControllerType, int ) { zoomOut(); } );
        renderer.SetUserKeyEventHandler( SunLight :: Input :: KEY_KP_EQUAL,
                                         [resetZoom]( SunLight :: Input :: ControllerType, int ) { resetZoom(); } );
    }
}

#endif /* __ZOOMKEYS_H__ */
