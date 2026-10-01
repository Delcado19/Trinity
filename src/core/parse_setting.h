#pragma once
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace trinity
{
    // Hand-edited INI values must not send NaN/Inf or partial numbers into
    // physics and damage calculations. Invalid input retains the prior default.
    inline float ParseFloatSetting(const char* text, float fallback)
    {
        char* end = nullptr;
        errno = 0;
        const float value = std::strtof(text, &end);
        if (end == text || errno == ERANGE || !std::isfinite(value)) return fallback;
        while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
        return *end ? fallback : value;
    }
}
