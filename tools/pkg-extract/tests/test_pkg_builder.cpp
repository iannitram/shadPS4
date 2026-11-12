// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Tests of the test-package builder: what it writes is read back field for field with the
// reader's own structs (PKGHeader, PKGEntry, PSF). The payloads are random per run and never
// written to the repo or printed.

#include <algorithm>
#include <cstring>
#include <random>
#include <string>
#include <string_view>
#include <vector>
#include <gtest/gtest.h>
#include "core/file_format/pkg.h"
#include "pkg_builder.h"

namespace {

constexpr u32 PkgMagic = 0x7F434E54;
constexpr std::string_view ContentId = "EP0000-PKGBUILD0_00-0000000000000000";
constexpr std::string_view Title = "Builder test";

std::vector<u8> RandomBytes(size_t size) {
    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution<unsigned> byte(0, 255);
    std::vector<u8> bytes(size);
    for (u8& b : bytes) {
        b = static_cast<u8>(byte(engine));
    }
    return bytes;
}

PkgBuilder::Spec MakeSpec() {
    return PkgBuilder::Spec{std::string(ContentId),
                            std::string(Title),
                            {{0x10, RandomBytes(0x500)}, {0x400, RandomBytes(0x30)}}};
}

PKGHeader ReadHeader(const std::vector<u8>& pkg) {
    PKGHeader header{};
    std::memcpy(&header, pkg.data(), sizeof(header));
    return header;
}

PKGEntry ReadEntry(const std::vector<u8>& pkg, size_t index) {
    PKGEntry entry{};
    std::memcpy(&entry, pkg.data() + sizeof(PKGHeader) + index * sizeof(PKGEntry),
                sizeof(entry));
    return entry;
}

std::string_view ReadPsfText(const std::vector<u8>& sfo, std::string_view key) {
    PkgBuilder::PsfHeader header{};
    std::memcpy(&header, sfo.data(), sizeof(header));
    for (size_t i = 0; i < header.index_table_entries; ++i) {
        PkgBuilder::PsfRawEntry entry{};
        std::memcpy(&entry, sfo.data() + sizeof(PkgBuilder::PsfHeader) +
                                i * sizeof(PkgBuilder::PsfRawEntry),
                    sizeof(entry));
        const char* name = reinterpret_cast<const char*>(sfo.data() + header.key_table_offset +
                                                         entry.key_offset);
        if (key == name) {
            const char* text = reinterpret_cast<const char*>(sfo.data() +
                                                             header.data_table_offset +
                                                             entry.data_offset);
            return std::string_view(text, entry.param_len - 1);
        }
    }
    return {};
}

} // namespace

TEST(PkgBuilder, HeaderReadsBackAsTheReaderReadsIt) {
    const auto pkg = PkgBuilder::Build(MakeSpec());
    ASSERT_GE(pkg.size(), sizeof(PKGHeader));
    const PKGHeader header = ReadHeader(pkg);

    EXPECT_EQ(static_cast<u32>(header.magic), PkgMagic);
    EXPECT_EQ(static_cast<u32>(header.pkg_type), 1u);
    EXPECT_EQ(static_cast<u32>(header.pkg_file_count), 3u);
    EXPECT_EQ(static_cast<u32>(header.pkg_table_entry_count), 3u);
    EXPECT_EQ(static_cast<u32>(header.pkg_table_entry_count_2), 3u);
    EXPECT_EQ(static_cast<u32>(header.pkg_table_entry_offset), sizeof(PKGHeader));
    EXPECT_EQ(static_cast<u64>(header.pkg_size), pkg.size());
    const auto body_offset = static_cast<u64>(header.pkg_body_offset);
    EXPECT_EQ(static_cast<u64>(header.pkg_body_size), pkg.size() - body_offset);
    EXPECT_EQ(static_cast<u64>(header.pkg_content_offset), body_offset);
    EXPECT_EQ(static_cast<u64>(header.pkg_content_size), pkg.size() - body_offset);
    EXPECT_GE(body_offset, sizeof(PKGHeader) + 3 * sizeof(PKGEntry));
    EXPECT_EQ(std::memcmp(header.pkg_content_id, ContentId.data(), ContentId.size()), 0);

    const std::string_view title_id(reinterpret_cast<const char*>(header.pkg_content_id + 7), 9);
    EXPECT_EQ(title_id, "PKGBUILD0");
}

TEST(PkgBuilder, EntriesReadBackAsTheReaderReadsThem) {
    const auto spec = MakeSpec();
    const auto pkg = PkgBuilder::Build(spec);
    const PKGHeader header = ReadHeader(pkg);
    ASSERT_EQ(static_cast<u32>(header.pkg_table_entry_count), 3u);

    const std::vector<u32> ids{0x1000, 0x10, 0x400};
    const std::vector<std::vector<u8>> payloads{{}, spec.entries[0].data, spec.entries[1].data};
    for (size_t i = 0; i < ids.size(); ++i) {
        const PKGEntry entry = ReadEntry(pkg, i);
        EXPECT_EQ(static_cast<u32>(entry.id), ids[i]) << "entry " << i;
        EXPECT_EQ(static_cast<u32>(entry.filename_offset), 0u) << "entry " << i;
        EXPECT_EQ(static_cast<u32>(entry.flags1), 0u) << "entry " << i;
        EXPECT_EQ(static_cast<u32>(entry.flags2), 0u) << "entry " << i;
        EXPECT_EQ(static_cast<u64>(entry.padding), 0u) << "entry " << i;
        ASSERT_GE(static_cast<u64>(entry.offset), static_cast<u64>(header.pkg_body_offset));
        if (i == 0) {
            EXPECT_GT(static_cast<u32>(entry.size), 0u);
            continue;
        }
        const auto size = static_cast<u32>(entry.size);
        ASSERT_EQ(size, payloads[i].size()) << "entry " << i;
        ASSERT_LE(static_cast<u64>(entry.offset) + size, pkg.size());
        EXPECT_TRUE(std::equal(payloads[i].begin(), payloads[i].end(),
                               pkg.begin() + static_cast<u32>(entry.offset)))
            << "entry " << i;
    }
}

TEST(PkgBuilder, ParamSfoCarriesTheContentIdAndTitle) {
    const auto pkg = PkgBuilder::Build(MakeSpec());
    const PKGEntry entry = ReadEntry(pkg, 0);
    ASSERT_EQ(static_cast<u32>(entry.id), 0x1000u);
    const auto sfo_begin = pkg.begin() + static_cast<u32>(entry.offset);
    const std::vector<u8> sfo(sfo_begin, sfo_begin + static_cast<u32>(entry.size));

    PkgBuilder::PsfHeader psf{};
    std::memcpy(&psf, sfo.data(), sizeof(psf));
    EXPECT_EQ(static_cast<u32>(psf.magic), PkgBuilder::PsfMagic);
    EXPECT_EQ(static_cast<u32>(psf.index_table_entries), 2u);
    EXPECT_EQ(ReadPsfText(sfo, "CONTENT_ID"), ContentId);
    EXPECT_EQ(ReadPsfText(sfo, "TITLE"), Title);
}

TEST(PkgBuilder, RandomPayloadsDifferEachRun) {
    EXPECT_NE(RandomBytes(32), RandomBytes(32));
    EXPECT_NE(PkgBuilder::Build(MakeSpec()), PkgBuilder::Build(MakeSpec()));
}
