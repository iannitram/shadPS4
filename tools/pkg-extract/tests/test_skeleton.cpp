// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The harness itself: the command-line library links and its usage text names every option.

#include <gtest/gtest.h>
#include "cli.h"

TEST(PkgExtractSkeleton, UsageNamesEveryOption) {
    const std::string usage = PkgExtract::Usage();
    EXPECT_NE(usage.find("shadps4-pkg"), std::string::npos);
    EXPECT_NE(usage.find("--keys"), std::string::npos);
    EXPECT_NE(usage.find("--out"), std::string::npos);
    EXPECT_NE(usage.find("--threads"), std::string::npos);
}
