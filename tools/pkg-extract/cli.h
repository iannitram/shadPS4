// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Command line of the standalone package extractor, shadps4-pkg (our delta D6).
//   shadps4-pkg <package> --keys <key file> [--out <dir>] [--threads N]
// Rules for ParseArgs: docs/code-map.md, pkg-extract, "Command line".

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace PkgExtract {

struct Options {
    std::filesystem::path package;               // the .pkg to unpack
    std::filesystem::path keys;                  // my key file, read at run time
    std::optional<std::filesystem::path> out_dir; // empty: ./<title id>
    unsigned threads = 4;                        // files unpacked at once, 1..32
    bool help = false;                           // -h or --help was given
};

struct ParseResult {
    std::optional<Options> options; // set when the arguments are usable
    std::string error;              // set (and options empty) when they are not
};

// args: the arguments after the program name, as given.
ParseResult ParseArgs(const std::vector<std::string>& args);

// The usage text printed for --help and after an error.
std::string Usage();

} // namespace PkgExtract
