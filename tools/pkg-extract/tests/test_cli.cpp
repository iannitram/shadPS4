// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Locked tests: shadps4-pkg's command line (docs/code-map.md, pkg-extract, "Command line").

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "cli.h"

using PkgExtract::ParseArgs;
using Args = std::vector<std::string>;

namespace {

void ExpectError(const Args& args, const std::string& message) {
    const auto r = ParseArgs(args);
    EXPECT_FALSE(r.options.has_value()) << "args gave options; expected: " << message;
    EXPECT_EQ(r.error, message);
}

} // namespace

TEST(PkgCli, PackageAndKeysIsOkWithDefaults) {
    const auto r = ParseArgs({"game.pkg", "--keys", "my.keys"});
    ASSERT_TRUE(r.options.has_value()) << r.error;
    EXPECT_TRUE(r.error.empty());
    EXPECT_EQ(r.options->package, "game.pkg");
    EXPECT_EQ(r.options->keys, "my.keys");
    EXPECT_FALSE(r.options->out_dir.has_value());
    EXPECT_EQ(r.options->threads, 4u);
    EXPECT_FALSE(r.options->help);
}

TEST(PkgCli, OptionsInAnyOrder) {
    const auto r = ParseArgs({"--threads", "8", "--out", "dir", "--keys", "k", "p.pkg"});
    ASSERT_TRUE(r.options.has_value()) << r.error;
    EXPECT_EQ(r.options->package, "p.pkg");
    EXPECT_EQ(r.options->keys, "k");
    ASSERT_TRUE(r.options->out_dir.has_value());
    EXPECT_EQ(*r.options->out_dir, "dir");
    EXPECT_EQ(r.options->threads, 8u);
}

TEST(PkgCli, HelpWinsOverEverything) {
    for (const Args& a : {Args{"-h"}, Args{"--help"}, Args{"--bogus", "--help"},
                          Args{"a", "b", "c", "-h"}, Args{"--threads", "99", "--help"}}) {
        const auto r = ParseArgs(a);
        ASSERT_TRUE(r.options.has_value());
        EXPECT_TRUE(r.error.empty());
        EXPECT_TRUE(r.options->help);
        EXPECT_TRUE(r.options->package.empty());
        EXPECT_TRUE(r.options->keys.empty());
        EXPECT_FALSE(r.options->out_dir.has_value());
        EXPECT_EQ(r.options->threads, 4u);
    }
}

TEST(PkgCli, HelpAsAValueStillMeansHelp) {
    const auto r = ParseArgs({"p.pkg", "--keys", "--help"});
    ASSERT_TRUE(r.options.has_value());
    EXPECT_TRUE(r.options->help);
}

TEST(PkgCli, ValueOptionTakesTheNextArgumentWhateverItIs) {
    const auto r = ParseArgs({"p.pkg", "--keys", "--out"});
    ASSERT_TRUE(r.options.has_value()) << r.error;
    EXPECT_EQ(r.options->keys, "--out");
    EXPECT_FALSE(r.options->out_dir.has_value());
}

TEST(PkgCli, MissingValue) {
    ExpectError({"p.pkg", "--keys"}, "missing value for --keys");
    ExpectError({"p.pkg", "--keys", "k", "--out"}, "missing value for --out");
    ExpectError({"p.pkg", "--keys", "k", "--threads"}, "missing value for --threads");
}

TEST(PkgCli, LastRepeatWins) {
    const auto r = ParseArgs({"p.pkg", "--keys", "a", "--keys", "b", "--threads", "2",
                              "--threads", "3", "--out", "x", "--out", "y"});
    ASSERT_TRUE(r.options.has_value()) << r.error;
    EXPECT_EQ(r.options->keys, "b");
    EXPECT_EQ(r.options->threads, 3u);
    EXPECT_EQ(*r.options->out_dir, "y");
}

TEST(PkgCli, ThreadsOneToThirtyTwo) {
    for (const char* good : {"1", "32", "07", "001", "16"}) {
        const auto r = ParseArgs({"p.pkg", "--keys", "k", "--threads", good});
        ASSERT_TRUE(r.options.has_value()) << good << ": " << r.error;
    }
    EXPECT_EQ(ParseArgs({"p.pkg", "--keys", "k", "--threads", "07"}).options->threads, 7u);
    EXPECT_EQ(ParseArgs({"p.pkg", "--keys", "k", "--threads", "032"}).options->threads, 32u);
    for (const char* bad : {"0", "33", "000", "100", "0001", "-1", "+4", "4x", "", " 4", "4.0",
                            "abc", "99999999999999999999"}) {
        ExpectError({"p.pkg", "--keys", "k", "--threads", bad},
                    "--threads must be a number from 1 to 32");
    }
}

TEST(PkgCli, UnknownOption) {
    ExpectError({"p.pkg", "--keys", "k", "--verbose"}, "unknown option: --verbose");
    ExpectError({"-x", "p.pkg", "--keys", "k"}, "unknown option: -x");
}

TEST(PkgCli, LoneDashIsAPackagePath) {
    const auto r = ParseArgs({"-", "--keys", "k"});
    ASSERT_TRUE(r.options.has_value()) << r.error;
    EXPECT_EQ(r.options->package, "-");
}

TEST(PkgCli, SecondPackageIsUnexpected) {
    ExpectError({"a.pkg", "b.pkg", "--keys", "k"}, "unexpected argument: b.pkg");
}

TEST(PkgCli, FirstErrorInScanOrderWins) {
    ExpectError({"a.pkg", "--bogus", "b.pkg"}, "unknown option: --bogus");
    ExpectError({"a.pkg", "b.pkg", "--bogus"}, "unexpected argument: b.pkg");
}

TEST(PkgCli, MissingPackageThenMissingKeys) {
    ExpectError({}, "missing package path");
    ExpectError({"--keys", "k"}, "missing package path");
    ExpectError({"p.pkg"}, "missing --keys <file>");
    ExpectError({"p.pkg", "--out", "o"}, "missing --keys <file>");
}

TEST(PkgCli, UsageNamesEveryOption) {
    const std::string u = PkgExtract::Usage();
    for (const char* opt : {"--keys", "--out", "--threads", "--help"}) {
        EXPECT_NE(u.find(opt), std::string::npos) << opt;
    }
}
