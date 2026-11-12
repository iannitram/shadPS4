// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
#include "pkg_builder.h"
#include "core/file_format/pkg.h"

namespace PkgBuilder {

namespace {

constexpr u32 PkgMagic = 0x7F434E54;
constexpr u32 ParamSfoId = 0x1000;
constexpr u64 BodyAlign = 0x10;

u64 AlignUp(u64 value) {
    return (value + BodyAlign - 1) / BodyAlign * BodyAlign;
}

void Append(std::vector<u8>& out, const void* data, size_t size) {
    const auto* bytes = static_cast<const u8*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

std::vector<u8> MakeParamSfo(const std::string& content_id, const std::string& title) {
    const std::array<std::pair<std::string, std::string>, 2> texts{
        {{"CONTENT_ID", content_id}, {"TITLE", title}}};
    std::vector<u8> keys;
    std::vector<u8> values;
    std::vector<PsfRawEntry> index;
    for (const auto& [key, value] : texts) {
        PsfRawEntry entry{};
        entry.key_offset = static_cast<u16>(keys.size());
        entry.param_fmt = PsfTextFormat;
        entry.param_len = static_cast<u32>(value.size() + 1);
        entry.param_max_len = entry.param_len;
        entry.data_offset = static_cast<u32>(values.size());
        index.push_back(entry);
        Append(keys, key.c_str(), key.size() + 1);
        Append(values, value.c_str(), value.size() + 1);
    }

    PsfHeader header{};
    header.magic = PsfMagic;
    header.version = PsfVersion11;
    header.index_table_entries = static_cast<u32>(index.size());
    header.key_table_offset =
        static_cast<u32>(sizeof(PsfHeader) + index.size() * sizeof(PsfRawEntry));
    header.data_table_offset = header.key_table_offset + static_cast<u32>(keys.size());

    std::vector<u8> sfo;
    Append(sfo, &header, sizeof(header));
    Append(sfo, index.data(), index.size() * sizeof(PsfRawEntry));
    Append(sfo, keys.data(), keys.size());
    Append(sfo, values.data(), values.size());
    return sfo;
}

} // namespace

std::vector<u8> Build(const Spec& spec) {
    std::vector<Entry> entries{{ParamSfoId, MakeParamSfo(spec.content_id, spec.title)}};
    entries.insert(entries.end(), spec.entries.begin(), spec.entries.end());

    const u64 table_size = entries.size() * sizeof(PKGEntry);
    const u64 body_offset = AlignUp(sizeof(PKGHeader) + table_size);
    std::vector<u8> pkg(body_offset);

    std::vector<PKGEntry> table;
    for (const Entry& entry : entries) {
        PKGEntry pkg_entry{};
        pkg_entry.id = entry.id;
        pkg_entry.offset = static_cast<u32>(pkg.size());
        pkg_entry.size = static_cast<u32>(entry.data.size());
        table.push_back(pkg_entry);
        Append(pkg, entry.data.data(), entry.data.size());
        pkg.resize(AlignUp(pkg.size()));
    }

    PKGHeader header{};
    header.magic = PkgMagic;
    header.pkg_type = 1;
    header.pkg_file_count = static_cast<u32>(entries.size());
    header.pkg_table_entry_count = static_cast<u32>(entries.size());
    header.pkg_table_entry_count_2 = static_cast<u16>(entries.size());
    header.pkg_table_entry_offset = static_cast<u32>(sizeof(PKGHeader));
    header.pkg_body_offset = body_offset;
    header.pkg_body_size = pkg.size() - body_offset;
    header.pkg_content_offset = body_offset;
    header.pkg_content_size = pkg.size() - body_offset;
    std::memcpy(header.pkg_content_id, spec.content_id.data(),
                std::min(spec.content_id.size(), sizeof(header.pkg_content_id)));
    header.pkg_size = pkg.size();

    std::memcpy(pkg.data(), &header, sizeof(header));
    std::memcpy(pkg.data() + sizeof(PKGHeader), table.data(), table_size);
    return pkg;
}

} // namespace PkgBuilder
