// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli.h"

namespace PkgExtract {

namespace {

bool ParseThreads(const std::string& text, unsigned& threads) {
    if (text.empty() || text.size() > 3) {
        return false;
    }
    unsigned value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + static_cast<unsigned>(c - '0');
    }
    if (value < 1 || value > 32) {
        return false;
    }
    threads = value;
    return true;
}

} // namespace

ParseResult ParseArgs(const std::vector<std::string>& args) {
    for (const std::string& arg : args) {
        if (arg == "-h" || arg == "--help") {
            Options help_only;
            help_only.help = true;
            return {help_only, ""};
        }
    }

    Options options;
    bool has_package = false;
    bool has_keys = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--keys" || arg == "--out" || arg == "--threads") {
            if (i + 1 == args.size()) {
                return {std::nullopt, "missing value for " + arg};
            }
            const std::string& value = args[++i];
            if (arg == "--keys") {
                options.keys = value;
                has_keys = true;
            } else if (arg == "--out") {
                options.out_dir = value;
            } else if (!ParseThreads(value, options.threads)) {
                return {std::nullopt, "--threads must be a number from 1 to 32"};
            }
        } else if (arg.size() > 1 && arg[0] == '-') {
            return {std::nullopt, "unknown option: " + arg};
        } else if (!has_package) {
            options.package = arg;
            has_package = true;
        } else {
            return {std::nullopt, "unexpected argument: " + arg};
        }
    }
    if (!has_package) {
        return {std::nullopt, "missing package path"};
    }
    if (!has_keys) {
        return {std::nullopt, "missing --keys <file>"};
    }
    return {options, ""};
}

std::string Usage() {
    return "usage: shadps4-pkg <package> --keys <key file> [--out <dir>] [--threads N]\n"
           "  --keys <file>  my key file (never shipped with this tool)\n"
           "  --out <dir>    output folder, empty or absent (default ./<title id>)\n"
           "  --threads N    files unpacked at once, 1 to 32 (default 4)\n"
           "  -h, --help     show this text\n";
}

} // namespace PkgExtract
