// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Locked tests: the run-time key file (docs/code-map.md, pkg-extract, "Key file").
// Every value here is made up by the test; no real key is or may ever be in this file.

#include <string>
#include <gtest/gtest.h>
#include "pkg_keyfile.h"

using PkgKeyFile::Parse;

namespace {

// 512 hex digits: bytes seed, seed+1, ... (mod 256).
std::string Hex(unsigned seed, bool upper = false) {
    static const char* lo = "0123456789abcdef";
    static const char* up = "0123456789ABCDEF";
    const char* d = upper ? up : lo;
    std::string s;
    for (unsigned i = 0; i < 256; ++i) {
        const unsigned b = (seed + i) & 0xFF;
        s += d[b >> 4];
        s += d[b & 0xF];
    }
    return s;
}

std::string Full() {
    return "dk3.modulus = " + Hex(1) + "\n" + "dk3.private_exponent = " + Hex(2) + "\n" +
           "fake.modulus = " + Hex(3) + "\n" + "fake.private_exponent = " + Hex(4) + "\n";
}

void ExpectError(const std::string& text, const std::string& message) {
    const auto r = Parse(text);
    EXPECT_FALSE(r.keys.has_value()) << "expected: " << message;
    EXPECT_EQ(r.error, message);
}

} // namespace

TEST(PkgKeyFile, FullFileParsesFirstByteFirst) {
    const auto r = Parse(Full());
    ASSERT_TRUE(r.keys.has_value()) << r.error;
    EXPECT_TRUE(r.error.empty());
    EXPECT_EQ(r.keys->dk3_modulus[0], 1);
    EXPECT_EQ(r.keys->dk3_modulus[255], 0);       // (1 + 255) & 0xFF
    EXPECT_EQ(r.keys->dk3_private_exponent[0], 2);
    EXPECT_EQ(r.keys->fake_modulus[10], 13);
    EXPECT_EQ(r.keys->fake_private_exponent[255], 3);
}

TEST(PkgKeyFile, UpperCaseHexAndAnyOrder) {
    const std::string t = "fake.private_exponent=" + Hex(4, true) + "\n" +
                          "fake.modulus=" + Hex(3) + "\n" + "dk3.private_exponent=" + Hex(2) +
                          "\n" + "dk3.modulus=" + Hex(0xAB, true);
    const auto r = Parse(t);
    ASSERT_TRUE(r.keys.has_value()) << r.error;
    EXPECT_EQ(r.keys->dk3_modulus[0], 0xAB);
    EXPECT_EQ(r.keys->fake_private_exponent[0], 4);
}

TEST(PkgKeyFile, CommentsBlankLinesCrlfAndWhitespace) {
    const std::string t = "# my keys\r\n\r\n   \t\r\n\t dk3.modulus \t=\t " + Hex(1) + " \r\n" +
                          "  # indented comment\n" + "dk3.private_exponent = " + Hex(2) + "\r\n" +
                          "fake.modulus = " + Hex(3) + "\n" + "fake.private_exponent = " + Hex(4);
    const auto r = Parse(t);
    ASSERT_TRUE(r.keys.has_value()) << r.error;
    EXPECT_EQ(r.keys->dk3_modulus[0], 1);
}

TEST(PkgKeyFile, NoEqualsOrTwoEquals) {
    ExpectError("# c\ndk3.modulus " + Hex(1) + "\n", "line 2: expected name = hex");
    ExpectError("dk3.modulus = a = b\n", "line 1: expected name = hex");
}

TEST(PkgKeyFile, UnknownName) {
    ExpectError("\n\nretail.key = " + Hex(1), "line 3: unknown key name: retail.key");
    ExpectError("DK3.modulus = " + Hex(1), "line 1: unknown key name: DK3.modulus");
}

TEST(PkgKeyFile, GivenTwiceNamesTheSecondLine) {
    ExpectError(Full() + "# again\nfake.modulus = " + Hex(9),
                "line 6: fake.modulus given twice");
}

TEST(PkgKeyFile, ValueMustBe512HexDigits) {
    ExpectError("dk3.modulus = " + Hex(1).substr(1), "line 1: dk3.modulus must be 512 hex digits");
    ExpectError("dk3.modulus = " + Hex(1) + "0", "line 1: dk3.modulus must be 512 hex digits");
    ExpectError("dk3.modulus = ", "line 1: dk3.modulus must be 512 hex digits");
    std::string bad = Hex(1);
    bad[100] = 'g';
    ExpectError("dk3.modulus = " + bad, "line 1: dk3.modulus must be 512 hex digits");
    std::string spaced = Hex(1);
    spaced[200] = ' ';
    ExpectError("dk3.modulus = " + spaced, "line 1: dk3.modulus must be 512 hex digits");
}

TEST(PkgKeyFile, FirstMissingInRuleOrder) {
    ExpectError("", "missing key: dk3.modulus");
    ExpectError("# only a comment\n", "missing key: dk3.modulus");
    ExpectError("fake.modulus = " + Hex(3) + "\ndk3.modulus = " + Hex(1),
                "missing key: dk3.private_exponent");
    ExpectError("dk3.modulus = " + Hex(1) + "\ndk3.private_exponent = " + Hex(2) +
                    "\nfake.modulus = " + Hex(3),
                "missing key: fake.private_exponent");
}

TEST(PkgKeyFile, FirstBadLineWinsOverMissingKeys) {
    ExpectError("dk3.modulus = " + Hex(1) + "\nnonsense\n", "line 2: expected name = hex");
}
