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

#ifndef __GAMEPADIDENTITYMATCHER_H__
#define __GAMEPADIDENTITYMATCHER_H__

#include <string>
#include <vector>


namespace SunLight {
    namespace Input  {

        /**
         * @brief Correlates two independently-enumerated gamepad name lists (F7b of the master
         * plan: raylib/GLFW's own gamepad list has no vibration API, SDL2's does, so a real
         * IGamepadHaptics has to find "the same" device in SDL's own, separately-indexed list).
         *
         * When there is exactly one name on each side, they are matched unconditionally,
         * regardless of whether the strings agree - confirmed live against a real Xbox One S
         * controller that raylib/GLFW names "Xbox Wireless Controller" and SDL names "Xbox One
         * S Controller": two mapping databases disagreeing about the identical, single
         * connected unit. A device's name is the only signal raylib exposes at all (no GUID,
         * no vendor/product id - checked against its public header), and the two libraries'
         * names for it cannot be relied on to agree, so with only one candidate on each side
         * there is nothing else it could be.
         *
         * With more than one name on either side, falls back to a greedy exact-name match in
         * order: for each name in targetNames, the first not-yet-matched name in candidateNames
         * that equals it is taken. This is a best-effort heuristic only, not a
         * guaranteed-correct identification - neither a device name nor an SDL-compatible GUID
         * identifies a specific physical unit, only its make and model (GLFW's own
         * documentation says exactly this of its joystick GUIDs), so two identical controllers
         * are indistinguishable by name or GUID alike, and the single-candidate shortcut above
         * does not apply once there is more than one to choose from. Matching by each list's
         * own relative connection order is the best available tie-break once two candidates
         * share a name.
         *
         * @param targetNames Names to find a match for, e.g. raylib's own gamepad names;
         * @param candidateNames Names to match against, e.g. SDL's own gamepad names;
         * @return One entry per targetNames entry, in the same order: the matched index into
         * candidateNames, or -1 if nothing matched;
         */
        std :: vector<int> MatchGamepadsByName( const std :: vector<std :: string>& targetNames,
                                                 const std :: vector<std :: string>& candidateNames );
    }
}

#endif  /* __GAMEPADIDENTITYMATCHER_H__ */
