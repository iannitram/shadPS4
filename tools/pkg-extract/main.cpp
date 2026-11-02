// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// shadps4-pkg: unpacks a self-made (fake) PS4 package, on Linux, Windows and macOS.
// Skeleton: parses its arguments; the unpacking is wired in by the "standalone extractor" issue.

#include <iostream>
#include <string>
#include <vector>
#include "cli.h"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    const PkgExtract::ParseResult parsed = PkgExtract::ParseArgs(args);
    if (!parsed.options) {
        std::cerr << "shadps4-pkg: " << parsed.error << "\n" << PkgExtract::Usage();
        return 2;
    }
    if (parsed.options->help) {
        std::cout << PkgExtract::Usage();
        return 0;
    }
    std::cerr << "shadps4-pkg: unpacking is not built yet\n";
    return 3;
}
