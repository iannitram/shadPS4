// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The key file the package extractor reads at run time (our delta D6). The keys are always the
// user's own file: none ship in this repo or its releases. Format and rules:
// docs/code-map.md, pkg-extract, "Key file".

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace PkgKeyFile {

using Rsa2048 = std::array<std::uint8_t, 256>;

struct Keys {
    Rsa2048 dk3_modulus{};
    Rsa2048 dk3_private_exponent{};
    Rsa2048 fake_modulus{};
    Rsa2048 fake_private_exponent{};
};

struct ParseResult {
    std::optional<Keys> keys; // set when the text is a complete, valid key file
    std::string error;        // set (and keys empty) when it is not
};

ParseResult Parse(std::string_view text);

} // namespace PkgKeyFile
