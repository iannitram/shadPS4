// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <optional>

int RunPkgExtract(const std::filesystem::path& pkg_path,
                  const std::optional<std::filesystem::path>& dir, unsigned threads);
