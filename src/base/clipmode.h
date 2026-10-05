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

#ifndef __CLIPMODE_H__
#define __CLIPMODE_H__

/*
 * How tiles and canvases are cut at the viewport's edges.
 *
 *   1 (the default): the cut is computed in software, from the viewport's
 *     rectangle (Viewport::GetClippedRect), and only the visible part is drawn.
 *   0 (the scissor-clip study only): the whole tile or canvas is drawn at its
 *     zoomed size, and the engine's clip - the one each view pass sets - cuts it.
 *
 * Culling is the same in both: a tile or canvas entirely outside the viewport is
 * not drawn at all. Only the cut differs. Only the study branch builds with 0.
 */
#ifndef SUNLIGHT_SOFTWARE_CLIP
#define SUNLIGHT_SOFTWARE_CLIP 1
#endif

#endif /* __CLIPMODE_H__ */
