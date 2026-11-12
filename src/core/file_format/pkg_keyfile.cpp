// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <string>

#include "pkg_keyfile.h"

namespace PkgKeyFile {

namespace {

constexpr std::array<const char*, 4> KeyNames = {"dk3.modulus", "dk3.private_exponent",
                                                 "fake.modulus", "fake.private_exponent"};

std::string_view Trim(std::string_view s) {
    constexpr std::string_view ws = " \t";
    const auto first = s.find_first_not_of(ws);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

// A name is echoed in an error only if it is a short run of name characters that contains at least
// one character outside 0-9a-fA-F (code-map, Key file, rule 3).
bool IsEchoableName(std::string_view name) {
    if (name.empty() || name.size() > 32) {
        return false;
    }
    bool has_non_hex = false;
    for (const char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '.';
        if (!ok) {
            return false;
        }
        const bool is_hex =
            (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!is_hex) {
            has_non_hex = true;
        }
    }
    return has_non_hex;
}

int HexValue(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

bool DecodeKey(std::string_view value, Rsa2048& out) {
    if (value.size() != 512) {
        return false;
    }
    for (std::size_t i = 0; i < 256; ++i) {
        const int hi = HexValue(value[2 * i]);
        const int lo = HexValue(value[2 * i + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        out[i] = static_cast<std::uint8_t>(hi * 16 + lo);
    }
    return true;
}

ParseResult Fail(std::string message) {
    return {std::nullopt, std::move(message)};
}

} // namespace

ParseResult Parse(std::string_view text) {
    Keys keys;
    std::array<Rsa2048*, 4> targets = {&keys.dk3_modulus, &keys.dk3_private_exponent,
                                       &keys.fake_modulus, &keys.fake_private_exponent};
    std::array<bool, 4> seen = {false, false, false, false};

    std::size_t line_number = 0;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const auto end = text.find('\n', pos);
        std::string_view line =
            text.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
        pos = end == std::string_view::npos ? text.size() + 1 : end + 1;
        ++line_number;

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        line = Trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::string prefix = "line " + std::to_string(line_number) + ": ";
        const auto eq = line.find('=');
        if (eq == std::string_view::npos || line.find('=', eq + 1) != std::string_view::npos) {
            return Fail(prefix + "expected name = hex");
        }
        const std::string_view name = Trim(line.substr(0, eq));
        const std::string_view value = Trim(line.substr(eq + 1));

        std::size_t index = KeyNames.size();
        for (std::size_t i = 0; i < KeyNames.size(); ++i) {
            if (name == KeyNames[i]) {
                index = i;
                break;
            }
        }
        if (index == KeyNames.size()) {
            const std::string shown =
                IsEchoableName(name) ? std::string(name) : std::string("<unprintable key name>");
            return Fail(prefix + "unknown key name: " + shown);
        }
        if (seen[index]) {
            return Fail(prefix + KeyNames[index] + " given twice");
        }
        seen[index] = true;
        if (!DecodeKey(value, *targets[index])) {
            return Fail(prefix + KeyNames[index] + " must be 512 hex digits");
        }
    }

    for (std::size_t i = 0; i < KeyNames.size(); ++i) {
        if (!seen[i]) {
            return Fail(std::string("missing key: ") + KeyNames[i]);
        }
    }
    return {keys, std::string()};
}

} // namespace PkgKeyFile