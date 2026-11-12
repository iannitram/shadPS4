// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/file_format/pkg.h"
#include "core/file_format/pkg_extract.h"
#include "core/file_format/pkg_keyfile.h"

// --extract-pkg: unpack a fake .pkg with the package code restored from v.0.7.0.
int RunPkgExtract(const std::filesystem::path& pkg_path,
                  const std::optional<std::filesystem::path>& dir, unsigned threads,
                  const std::optional<std::filesystem::path>& keys_path) {
    if (!keys_path) {
        std::cerr << "missing --extract-keys <file>\n";
        return 2;
    }
    std::string key_text;
    Common::FS::IOFile keys_file(*keys_path, Common::FS::FileAccessMode::Read);
    if (keys_file.IsOpen()) {
        key_text.resize(keys_file.GetSize());
        keys_file.ReadRaw<char>(key_text.data(), key_text.size());
    }
    const auto parsed = PkgKeyFile::Parse(key_text);
    if (!parsed.keys) {
        std::cerr << parsed.error << "\n";
        return 2;
    }
    PKG pkg;
    std::string failreason;
    std::filesystem::path out = dir.value_or(std::filesystem::path{});
    if (out.empty()) {
        Common::FS::IOFile file(pkg_path, Common::FS::FileAccessMode::Read);
        PKGHeader header{};
        file.ReadRaw<u8>(&header, sizeof(header));
        out = std::filesystem::current_path() /
              std::string(reinterpret_cast<const char*>(header.pkg_content_id) + 7, 9);
    }
    std::error_code ec;
    if (std::filesystem::exists(out, ec) && !std::filesystem::is_empty(out, ec)) {
        std::cerr << "Not unpacking into " << out.string() << ": the folder is not empty.\n";
        return 1;
    }
    std::cout << "Reading " << pkg_path.string() << "\n";
    if (!pkg.Extract(pkg_path, out, *parsed.keys, failreason)) {
        LOG_ERROR(Loader, "PKG extract failed: {}", failreason);
        std::cerr << "Unpacking failed: " << failreason << "\n";
        return 1;
    }
    const u32 count = pkg.GetNumberOfFiles();
    s64 total = 0;
    for (u32 i = 0; i < count; i++) {
        total += pkg.GetFileSize(static_cast<int>(i));
    }
    std::cout << "Unpacking " << pkg.GetTitleID() << " into " << out.string() << ": " << count
              << " entries, " << total << " bytes, " << threads << " at once\n";

    std::atomic<u32> next{0};
    std::atomic<s64> done_bytes{0};
    std::atomic<bool> failed{false};
    std::mutex fail_mutex;
    std::string first_failure;
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < threads; t++) {
        workers.emplace_back([&] {
            std::string reason;
            for (u32 i = next++; i < count && !failed; i = next++) {
                if (!pkg.ExtractFiles(static_cast<int>(i), reason)) {
                    std::scoped_lock lock{fail_mutex};
                    if (!failed.exchange(true)) {
                        first_failure = reason;
                    }
                    return;
                }
                done_bytes += pkg.GetFileSize(static_cast<int>(i));
            }
        });
    }
    s64 shown = -1;
    while (next < count && !failed) {
        const s64 pct = total > 0 ? done_bytes * 100 / total : 100;
        if (pct != shown) {
            std::cout << "\r" << pct << " %" << std::flush;
            shown = pct;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    for (auto& w : workers) {
        w.join();
    }
    if (failed) {
        LOG_ERROR(Loader, "PKG extract failed: {}", first_failure);
        std::cerr << "\nUnpacking failed: " << first_failure << "\n";
        return 1;
    }
    LOG_INFO(Loader, "PKG {} unpacked into {}: {} entries, {} bytes", pkg.GetTitleID(),
             out.string(), count, total);
    std::cout << "\rDone: " << count << " entries, " << total << " bytes in " << out.string()
              << "\n";
    return 0;
}
