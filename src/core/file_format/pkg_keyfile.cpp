// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pkg_keyfile.h"

namespace PkgKeyFile {

ParseResult Parse(std::string_view text) {
    (void)text;
    return {std::nullopt, "not built yet"}; // stub: the "key file parser" issue
}

} // namespace PkgKeyFile
