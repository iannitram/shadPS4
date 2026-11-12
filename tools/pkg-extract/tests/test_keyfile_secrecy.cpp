// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Review addition: the key file parser must never put key material into an error message
// (docs/code-map.md, pkg-extract, "Key file", rule 9). Every value here is made up by the test.

#include <string>
#include <string_view>
#include <gtest/gtest.h>
#include "pkg_keyfile.h"

using PkgKeyFile::Parse;

namespace {

// 512 hex digits: bytes seed, seed+1, ... (mod 256).
std::string Hex(unsigned seed) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (unsigned i = 0; i < 256; ++i) {
        const unsigned b = (seed + i) & 0xFF;
        s += d[b >> 4];
        s += d[b & 0xF];
    }
    return s;
}

// Fails if any 32-character slice of a key value appears in the error text.
void ExpectNoKeyMaterial(const std::string& error, const std::string& value) {
    for (std::size_t at = 0; at + 32 <= value.size(); at += 32) {
        EXPECT_EQ(error.find(value.substr(at, 32)), std::string::npos) << error;
    }
}

} // namespace

TEST(PkgKeyFileSecrecy, BadValueErrorsDoNotEchoTheValue) {
    const std::string wrong_length = Hex(1).substr(1);
    const auto short_r = Parse("dk3.modulus = " + wrong_length + "\n");
    ASSERT_FALSE(short_r.keys.has_value());
    ExpectNoKeyMaterial(short_r.error, wrong_length);

    std::string bad = Hex(5);
    bad[100] = 'g';
    const auto bad_r = Parse("dk3.modulus = " + bad + "\n");
    ASSERT_FALSE(bad_r.keys.has_value());
    ExpectNoKeyMaterial(bad_r.error, bad);
}

TEST(PkgKeyFileSecrecy, LaterErrorsDoNotEchoEarlierValidKeys) {
    const std::string dk3_modulus = Hex(1);
    const std::string dk3_private = Hex(2);
    const std::string fake_modulus = Hex(3);
    const std::string short_exponent = Hex(4).substr(0, 510);
    const std::string text = "dk3.modulus = " + dk3_modulus + "\n" +
                             "dk3.private_exponent = " + dk3_private + "\n" +
                             "fake.modulus = " + fake_modulus + "\n" +
                             "fake.private_exponent = " + short_exponent + "\n";
    const auto r = Parse(text);
    ASSERT_FALSE(r.keys.has_value());
    ExpectNoKeyMaterial(r.error, dk3_modulus);
    ExpectNoKeyMaterial(r.error, dk3_private);
    ExpectNoKeyMaterial(r.error, fake_modulus);
    ExpectNoKeyMaterial(r.error, short_exponent);
}

TEST(PkgKeyFileSecrecy, GivenTwiceErrorDoesNotEchoTheSecondValue) {
    const std::string first = Hex(1);
    const std::string second = Hex(9);
    const auto r = Parse("dk3.modulus = " + first + "\ndk3.modulus = " + second + "\n");
    ASSERT_FALSE(r.keys.has_value());
    EXPECT_EQ(r.error, "line 2: dk3.modulus given twice");
    ExpectNoKeyMaterial(r.error, first);
    ExpectNoKeyMaterial(r.error, second);
}

TEST(PkgKeyFileSecrecy, UnknownNameWithKeyHexIsNotEchoed) {
    // 64 hex digits on the left of '=': all letters and digits, so only the length rule rejects it.
    const std::string key_hex = Hex(1).substr(0, 64);
    const auto r = Parse(key_hex + " = " + Hex(2) + "\n");
    ASSERT_FALSE(r.keys.has_value());
    EXPECT_EQ(r.error, "line 1: unknown key name: <unprintable key name>");
    ExpectNoKeyMaterial(r.error, key_hex);
}

TEST(PkgKeyFileSecrecy, ShortUnknownNameIsEchoed) {
    const auto r = Parse("foo_bar = " + Hex(1) + "\n");
    ASSERT_FALSE(r.keys.has_value());
    EXPECT_EQ(r.error, "line 1: unknown key name: foo_bar");
}

TEST(PkgKeyFileSecrecy, UnknownNameLengthLimitIs32) {
    const std::string name32 = "abcdefghijklmnopqrstuvwxyz012345";
    ASSERT_EQ(name32.size(), 32u);
    const auto echoed = Parse(name32 + " = " + Hex(1) + "\n");
    ASSERT_FALSE(echoed.keys.has_value());
    EXPECT_EQ(echoed.error, "line 1: unknown key name: " + name32);

    const std::string name33 = name32 + "6";
    const auto hidden = Parse(name33 + " = " + Hex(1) + "\n");
    ASSERT_FALSE(hidden.keys.has_value());
    EXPECT_EQ(hidden.error, "line 1: unknown key name: <unprintable key name>");
    ExpectNoKeyMaterial(hidden.error, name33);
}
