// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Test-only builder of fake packages: the header, the entry table and param.sfo, from the
// payloads the test gives it. Nothing is encrypted yet (the PFS image and encryption come later).
// param.sfo's layout is written out here because psf.h needs the emulator's logging and io.

#pragma once

#include <string>
#include <vector>
#include "common/endian.h"
#include "common/types.h"

namespace PkgBuilder {

struct Entry {
    u32 id;
    std::vector<u8> data;
};

struct Spec {
    std::string content_id;     // 36 characters; the title ID is the 9 characters at offset 7
    std::string title;          // param.sfo's TITLE
    std::vector<Entry> entries; // written after param.sfo, which Build always writes first
};

struct PsfHeader {
    u32_be magic;
    u32_le version;
    u32_le key_table_offset;
    u32_le data_table_offset;
    u32_le index_table_entries;
};
static_assert(sizeof(PsfHeader) == 0x14);

struct PsfRawEntry {
    u16_le key_offset;
    u16_be param_fmt;
    u32_le param_len;
    u32_le param_max_len;
    u32_le data_offset;
};
static_assert(sizeof(PsfRawEntry) == 0x10);

constexpr u32 PsfMagic = 0x00505346;
constexpr u32 PsfVersion11 = 0x00000101;
constexpr u16 PsfTextFormat = 0x0204;

std::vector<u8> Build(const Spec& spec);

} // namespace PkgBuilder
