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

#ifndef __RENDERERCONFIG_H__
#define __RENDERERCONFIG_H__

#include <optional>
#include <string>
#include "input/iinputhandler.h"
#include "window/iwindow.h"
#include "tilemap/tilemapdefs.h"


namespace SunLight {
    namespace Renderer  {

        /**
         * View port control mode (active and reactive)
         * Active, the view port reacts to a single key pressing continuously;
         * Reactive, the view port reacts only for each key pressing;
         */
        enum ViewControlMode  {
            VIEW_CONTROL_MODE_ACTIVE,
            VIEW_CONTROL_MODE_REACTIVE
        };

        /**
         * @brief Which backend set (window + engine + ...) a renderer
         * runs on. An enum, not a string: the value is the stable
         * contract, and names of concrete backends are fine here - it is
         * only information about which one to use, nothing about how it
         * works. New backends (e.g. SDL) are added before
         * RENDERER_BACKEND_LAST, which is a sentinel and never a valid
         * choice itself.
         */
        enum RendererBackend  {
            RENDERER_BACKEND_DEFAULT = 0,   // whatever this build of sunlight was built with
            RENDERER_BACKEND_RAYLIB,
            RENDERER_BACKEND_NULL,          // windowless/headless: nothing is drawn or displayed, time is virtual
            RENDERER_BACKEND_LAST           // sentinel
        };

        /**
         * @brief How a backend that has no display to pace it holds the
         * frame loop to its target FPS. Only the null backend consults it -
         * the raylib backend always paces itself (its own frame present
         * sleeps to the target), whatever this says.
         */
        enum FramePacing  {
            FRAME_PACING_REAL_TIME = 0,     // wall-clock time passes at the game's own speed (default)
            FRAME_PACING_UNLIMITED          // frames as fast as the game logic runs: a long virtual timeline in a fraction of the time
        };

        /**
         * @brief Everything needed to create a TileMapRenderer, as one
         * value - replaces having to pass a growing list of constructor
         * arguments and then call a handful of setters before Start().
         * Defaults match what the classic
         * TileMapRenderer( width, height, title, targetFps, useDefaultKeyHandler )
         * constructor produced, so a config only names what it changes.
         *
         * Only state that must be fixed before the window is created (or
         * that callers universally set right after construction) lives
         * here; everything is still adjustable afterward through
         * TileMapRenderer's/IDrawSurface's own setters.
         */
        struct RendererConfig  {

            RendererBackend                           backend               = RENDERER_BACKEND_DEFAULT;

            // Window / render resolution, in pixels (the fixed internal
            // resolution the game renders at - see IDrawSurface::GetWindowWidth).
            float                                     fWidth                = 800.0f;
            float                                     fHeight               = 600.0f;
            std :: string                             strTitle              = "SunLight";

            // Target frame rate; -1 = renderer default.
            int                                       nTargetFps            = -1;
            bool                                      bResizeable           = false;
            bool                                      bDrawFPS              = false;
            bool                                      bStretchToFill        = false;

            // Null backend only: real-time paced (default) or as fast as possible.
            FramePacing                               framePacing           = FRAME_PACING_REAL_TIME;

            // Run() stops on its own, cleanly (returns true, like an exit
            // request but without setting one), after this many frames
            // since Start(); 0 = unlimited. A safety net / smoke-test
            // budget for any backend.
            unsigned                                  nMaxFrames            = 0;

            // Register the renderer's built-in camera/zoom key handlers.
            bool                                      bUseDefaultKeyHandler = true;

            // Key that makes the window ask to close; KEY_NULL disables it.
            // Applied at every Start(), so it sticks across restarts.
            SunLight :: Input :: KeyboardKey          exitKey               = SunLight :: Input :: KEY_ESCAPE;

            // Whether the window comes up ALREADY fullscreen (no windowed phase first) and with which
            // strategy - applied at every Start(), exactly like IDrawSurface::SetFullscreen( true,
            // strategy ) on the created window would. The null backend accepts and ignores both.
            bool                                      bFullscreen           = false;
            SunLight :: Window :: FullscreenStrategy  fullscreenStrategy    = SunLight :: Window :: FULLSCREEN_STRATEGY_REAL;

            ViewControlMode                           viewControlMode       = VIEW_CONTROL_MODE_ACTIVE;
            int                                       nScrollStepWidth      = -1;   // -1 = map tile width
            int                                       nScrollStepHeight     = -1;   // -1 = map tile height

            // Viewport rectangle (screen-space clip/scroll window) and zoom
            // position. Left unset, the viewport covers the whole render
            // area and the zoom keeps the renderer's own default.
            std :: optional<SunLight :: TileMap :: stDimension2D>  viewport;
            std :: optional<unsigned>                              nZoomPos;

            /**
             * @brief Whether a backend can actually be selected in this
             * build. RENDERER_BACKEND_DEFAULT always can (it means "the
             * build's own"), as can the null backend (part of every build);
             * RENDERER_BACKEND_LAST and out-of-range values never can.
             */
            static bool IsBackendAvailable( RendererBackend backend );

            /**
             * @brief Human-readable name of a backend, for messages.
             */
            static const char* BackendName( RendererBackend backend );

            /**
             * @brief Check the config can produce a renderer.
             * @param pError Receives a message describing the first
             * problem found, if any (may be null);
             * @return true if valid, false otherwise;
             */
            bool Validate( std :: string *pError = nullptr ) const;
        };
    }
}

#endif  /* __RENDERERCONFIG_H__ */
