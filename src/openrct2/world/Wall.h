/*****************************************************************************
 * Copyright (c) 2014-2024 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "TileElement.h"

enum
{
    WALL_ANIMATION_FLAG_ACROSS_TRACK = (1 << 2),
    // 3 - 6 animation frame number
    WALL_ANIMATION_FLAG_DIRECTION_BACKWARD = (1 << 7),
    WALL_ANIMATION_FLAG_ALL_FLAGS = WALL_ANIMATION_FLAG_ACROSS_TRACK | WALL_ANIMATION_FLAG_DIRECTION_BACKWARD
};

enum EDGE_SLOPE
{
    EDGE_SLOPE_UPWARDS = (1 << 0),
    EDGE_SLOPE_DOWNWARDS = (1 << 1),
    EDGE_SLOPE_ELEVATED = (1 << 2),

    EDGE_SLOPE_UPWARDS_ELEVATED = EDGE_SLOPE_UPWARDS | EDGE_SLOPE_ELEVATED,
    EDGE_SLOPE_DOWNWARDS_ELEVATED = EDGE_SLOPE_DOWNWARDS | EDGE_SLOPE_ELEVATED,
};

uint8_t GetWallSlopeFromEdgeSlope(uint8_t Slope, uint8_t Edge);
