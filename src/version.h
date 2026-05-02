// SPDX-License-Identifier: LGPL-3.0-or-later
//
// cpp65 is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

#pragma once

#include "cpp65/config.h"

#include <cstdint>
#include <string_view>

namespace cpp65 {

inline constexpr std::uint8_t version_major = CPP65_VERSION_MAJOR;
inline constexpr std::uint8_t version_minor = CPP65_VERSION_MINOR;
inline constexpr std::uint8_t version_patch = CPP65_VERSION_PATCH;
inline constexpr std::string_view version = CPP65_VERSION;

} // namespace cpp65
