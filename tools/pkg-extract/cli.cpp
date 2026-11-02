// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli.h"

namespace PkgExtract {

ParseResult ParseArgs(const std::vector<std::string>& args) {
    (void)args;
    return {std::nullopt, "not built yet"}; // stub: the "command line" issue
}

std::string Usage() {
    return "usage: shadps4-pkg <package> --keys <key file> [--out <dir>] [--threads N]\n"
           "  --keys <file>  my key file (never shipped with this tool)\n"
           "  --out <dir>    output folder, empty or absent (default ./<title id>)\n"
           "  --threads N    files unpacked at once, 1 to 32 (default 4)\n";
}

} // namespace PkgExtract
