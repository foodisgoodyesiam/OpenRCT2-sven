/*****************************************************************************
 * Copyright (c) 2014-2023 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../common.h"
#include "../core/String.hpp"

#include <cstdio>
#include <ctime>
#include <optional>
#include <type_traits>
#include <vector>

__attribute__ ((always_inline, artificial)) inline int32_t SquaredMetresToSquaredFeet(int32_t squaredMetres)
{
    // 1 metre squared = 10.7639104 feet squared
    // RCT2 approximates as 11
    return squaredMetres * 11;
}

__attribute__ ((always_inline, artificial)) inline int32_t MetresToFeet(int32_t metres)
{
    // 1 metre = 3.2808399 feet
    // RCT2 approximates as 3.28125
    return (metres * 840) / 256;
}

__attribute__ ((always_inline, artificial)) inline int32_t MphToKmph(int32_t mph)
{
    // 1 mph = 1.60934 kmph
    // RCT2 approximates as 1.609375
    return (mph * 1648) >> 10;
}

__attribute__ ((always_inline, artificial)) inline int32_t MphToDmps(int32_t mph)
{
    // 1 mph = 4.4704 decimeters/s
    return (mph * 73243) >> 14;
}

bool SSE41Available();
bool AVX2Available();

__attribute__ ((always_inline, artificial, hot)) inline int32_t UtilBitScanForward(int32_t source)
{   int32_t success = __builtin_ffs(source);
    return success - 1;
}

__attribute__ ((always_inline, artificial, hot)) inline int32_t UtilBitScanForward(int64_t source)
{
    int32_t success = __builtin_ffsll(source);
    return success - 1;
}

void BitCountInit();

__attribute__ ((always_inline, artificial, hot)) inline int32_t BitCount(uint32_t source) {
	return __builtin_popcount(source);
}
int32_t StrLogicalCmp(char const* a, char const* b);
char* SafeStrCpy(char* destination, const char* source, size_t num);
char* SafeStrCat(char* destination, const char* source, size_t size);

uint32_t UtilRand();
float UtilRandNormalDistributed();

bool UtilGzipCompress(FILE* source, FILE* dest);
std::vector<uint8_t> Gzip(const void* data, const size_t dataLen);
std::vector<uint8_t> Ungzip(const void* data, const size_t dataLen);

// TODO: Make these specialized template functions, or when possible Concepts in C++20
int8_t AddClamp_int8_t(int8_t value, int8_t value_to_add) __attribute__ ((hot));
int16_t AddClamp_int16_t(int16_t value, int16_t value_to_add) __attribute__ ((hot));
int32_t AddClamp_int32_t(int32_t value, int32_t value_to_add) __attribute__ ((hot));
int64_t AddClamp_int64_t(int64_t value, int64_t value_to_add) __attribute__ ((hot));
money64 AddClamp_money64(money64 value, money64 value_to_add) __attribute__ ((hot));

__attribute__ ((hot)) inline uint8_t Lerp(uint8_t a, uint8_t b, float t)
{
    if (t <= 0)
        return a;
    if (t >= 1)
        return b;

    int32_t range = b - a;
    int32_t amount = static_cast<int32_t>(range * t);
    return static_cast<uint8_t>(a + amount);
}
__attribute__ ((always_inline, hot)) inline float FLerp(float a, float b, float f)
{
    return (a * (1.0f - f)) + (b * f);
}

__attribute__ ((hot)) uint8_t SoftLight(uint8_t a, uint8_t b);

size_t StrCatFTime(char* buffer, size_t bufferSize, const char* format, const struct tm* tp);

template<typename T> __attribute__ ((hot)) [[nodiscard]] constexpr uint64_t EnumToFlag(T v)
{
    static_assert(std::is_enum_v<T>);
    return 1uLL << static_cast<std::underlying_type_t<T>>(v);
}

template<typename... T> [[nodiscard]] constexpr uint64_t EnumsToFlags(T... types)
{
    return (EnumToFlag(types) | ...);
}

template<typename TEnum> constexpr auto EnumValue(TEnum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<TEnum>>(enumerator);
}
