// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <cstdint>
#include <string_view>

namespace kettle
{
constexpr uint64_t fnv1a64(std::string_view text)
{
    uint64_t hash = 14695981039346656037ull;

    for (char c : text)
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ull;
    }

    return hash;
}

constexpr uint64_t hash(std::string_view text)
{
    return fnv1a64(text); // hash is an alias to a chosen hash method.
}
} // namespace kettle