/*****************************************************************************
 * Copyright (c) 2014-2024 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

namespace OpenRCT2
{
    int32_t SquaredMetresToSquaredFeet(int32_t squaredMetres) __attribute__ ((const));
    int32_t MetresToFeet(int32_t metres) __attribute__ ((const));
    int32_t FeetToMetres(int32_t feet) __attribute__ ((const));
    int32_t MphToKmph(int32_t mph) __attribute__ ((const));
    int32_t MphToDmps(int32_t mph) __attribute__ ((const));
    int32_t BaseZToMetres(int16_t baseZ) __attribute__ ((const));
    uint8_t MetresToBaseZ(int16_t metres) __attribute__ ((const));
    int32_t HeightUnitsToMetres(int32_t heightUnit) __attribute__ ((const));
    int32_t ToHumanReadableSpeed(int32_t baseSpeed) __attribute__ ((const));
    uint16_t ToHumanReadableAirTime(uint16_t airTime) __attribute__ ((const));
    int32_t ToHumanReadableRideLength(int32_t rideLength) __attribute__ ((const));
} // namespace OpenRCT2
