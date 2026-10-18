// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <thread>
#include <vector>
#include <CLI/CLI.hpp>
#include <SDL3/SDL_messagebox.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "common/arch.h"
#include "common/io_file.h"
#include "common/key_manager.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"
#include "core/file_format/pkg.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
#include "core/user_settings.h"
#include "emulator.h"
#include "imgui/big_picture/big_picture.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif

// --extract-pkg: unpack a fake .pkg with the package code restored from v.0.7.0.
static int RunPkgExtract(const std::filesystem::path& pkg_path,
                         const std::optional<std::filesystem::path>& dir, unsigned threads) {
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
    if (!pkg.Extract(pkg_path, out, failreason)) {
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

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

#if defined(__APPLE__) && defined(ARCH_X86_64)
    // KosmicKrisp only supports Apple Silicon. Check that we are not running on an Intel Mac.
    int sysctl_ret = 0;
    size_t sysctl_size = sizeof(sysctl_ret);
    sysctlbyname("sysctl.proc_translated", &sysctl_ret, &sysctl_size, nullptr, 0);
    if (sysctl_ret != 1) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "shadPS4",
                                 "shadPS4 only supports Apple Silicon Macs.", nullptr);
        std::cout << "shadPS4 only supports Apple Silicon Macs." << std::endl;
        return -1;
    }
#endif

    CLI::App app{"shadPS4 Emulator CLI"};

    // ---- CLI state ----
    std::optional<std::string> gamePath;
    std::vector<std::string> gameArgs;
    std::optional<std::filesystem::path> overrideRoot;
    std::optional<int> waitPid;
    bool waitForDebugger = false;
    bool userfaultfd = false;

    std::optional<std::string> fullscreenStr;
    bool ignoreGamePatch = false;
    bool showFps = false;
    bool configClean = false;
    bool configGlobal = false;
    bool bigPicture = false;
    bool sameProcess = false;
    bool append_log{};

    std::optional<std::filesystem::path> addGameFolder;
    std::optional<std::filesystem::path> setAddonFolder;
    std::optional<std::string> patchFile;

    std::vector<std::pair<std::filesystem::path, std::string>> mounts;
    static std::vector<std::string> env_vars;

    // ---- Options ----
    app.add_option("guest_arg", gamePath, "Game path or ID"); // positional
    app.add_option("-g,--game", gamePath, "Game path or ID");
    app.add_option("-p,--patch", patchFile, "Patch file to apply");
    app.add_flag("-i,--ignore-game-patch", ignoreGamePatch,
                 "Disable automatic loading of game patches");

    app.add_flag("-b,--big-picture", bigPicture, "Start in Big Picture Mode");
    app.add_flag("--same-process", sameProcess,
                 "Launch the game in the same process when using Big Picture Mode");

    app.add_option("-f,--fullscreen", fullscreenStr, "Fullscreen mode (true|false)");

    app.add_option("--override-root", overrideRoot)->check(CLI::ExistingDirectory);

    app.add_flag("--wait-for-debugger", waitForDebugger);
    app.add_option("--wait-for-pid", waitPid);
#ifdef __linux__
    app.add_flag("--userfaultfd", userfaultfd,
                 "Enable userfaultfd for tracking memory (Linux only)");
#endif

    app.add_flag("--show-fps", showFps);
    app.add_flag("--config-clean", configClean);
    app.add_flag("--config-global", configGlobal);
    app.add_flag("--log-append", append_log);

    app.add_option("--add-game-folder", addGameFolder)->check(CLI::ExistingDirectory);
    app.add_option("--set-addon-folder", setAddonFolder)->check(CLI::ExistingDirectory);
    app.add_option("--mount", mounts, "Mount source to destination");
    app.add_option("-e,--env", env_vars, "Environment variables to pass to the guest");

    std::optional<std::filesystem::path> extractPkg;
    std::optional<std::filesystem::path> extractDir;
    unsigned extractThreads = 4;
    app.add_option("--extract-pkg", extractPkg, "Unpack a fake PS4 .pkg and exit")
        ->check(CLI::ExistingFile);
    app.add_option("--extract-dir", extractDir,
                   "Folder to unpack into (default: ./<title id>); must be empty or absent");
    app.add_option("--extract-threads", extractThreads, "Files unpacked at once (default 4)")
        ->check(CLI::Range(1u, 32u));

    // ---- Capture args after `--` verbatim ----
    app.allow_extras();

    // ---- No-args behavior ----
    if (argc == 1) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "shadPS4",
                                 "This is a CLI application. Please use the '-b' flag for Big "
                                 "Picture mode, or QTLauncher for a standalone GUI:\n"
                                 "https://github.com/shadps4-emu/shadps4-qtlauncher/releases",
                                 nullptr);
        std::cout << app.help();
        return -1;
    }

    try {
        bool double_dash_found = false;
        int double_dash_index;
        for (int i = 0; i < argc; i++) {
            if (double_dash_found) {
                gameArgs.emplace_back(argv[i]);
            }
            if (!double_dash_found && std::string(argv[i]) == "--") {
                double_dash_found = true;
                double_dash_index = i;
            }
        }

        // If the -- arg is present, only parse args before it
        if (double_dash_found) {
            app.parse(double_dash_index, argv);
        } else {
            app.parse(argc, argv);
        }
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if (waitPid)
        Core::Debugger::WaitForPid(*waitPid);

    // Initialize main log with default config
    Common::Log::Setup("shadps4.log");

    LOG_INFO(Debug, "Run: {}", fmt::join(std::span(argv, argc), " "));

    if (extractPkg) {
        return RunPkgExtract(*extractPkg, extractDir, extractThreads);
    }

    IPC::Instance().Init();

    // Initialize key manager
    KeyManager::GetInstance()->LoadFromFile();

    // Load configurations
    EmulatorSettings.Load();
    UserSettings.Load();

    if (bigPicture) {
        BigPictureMode::Launch(argv[0], sameProcess);
        return 0;
    }

    // ---- Utility commands ----
    if (addGameFolder) {
        EmulatorSettings.AddGameInstallDir(*addGameFolder);
        EmulatorSettings.Save();
        LOG_INFO(Config, "Game folder successfully saved.");
        return 0;
    }

    if (setAddonFolder) {
        EmulatorSettings.SetAddonInstallDir(*setAddonFolder);
        EmulatorSettings.Save();
        LOG_INFO(Config, "Addon folder successfully saved.");
        return 0;
    }

    if (!gamePath.has_value()) {
        if (!gameArgs.empty()) {
            gamePath = gameArgs.front();
            gameArgs.erase(gameArgs.begin());
        } else {
            LOG_ERROR(Debug, "Please provide a game path or ID.");
            return 1;
        }
    }

    // ---- Apply flags ----
    if (patchFile)
        MemoryPatcher::patch_file = *patchFile;

    if (ignoreGamePatch)
        Core::FileSys::MntPoints::ignore_game_patches = true;

    if (fullscreenStr) {
        if (*fullscreenStr == "true") {
            EmulatorSettings.SetFullScreen(true);
        } else if (*fullscreenStr == "false") {
            EmulatorSettings.SetFullScreen(false);
        } else {
            LOG_ERROR(Debug, "Invalid argument for --fullscreen (use true|false)");
            return 1;
        }
    }

    if (showFps)
        EmulatorSettings.SetShowFpsCounter(true);

    if (configClean)
        EmulatorSettings.SetConfigMode(ConfigMode::Clean);

    if (configGlobal)
        EmulatorSettings.SetConfigMode(ConfigMode::Global);

    if (userfaultfd) {
        EmulatorSettings.SetUserfaultfdTracking(true);
    }

    // ---- Resolve game path or ID ----
    std::filesystem::path ebootPath(*gamePath);
    const auto archive_component_exists = [](const std::filesystem::path& p) -> bool {
        std::filesystem::path accum;
        for (const auto& comp : p) {
            accum /= comp;
            if (comp.extension() == ".zar") {
                return std::filesystem::is_regular_file(accum);
            }
        }
        return false;
    };
    if (!std::filesystem::exists(ebootPath) && !archive_component_exists(ebootPath)) {
        bool found = false;
        for (const auto& installDir : EmulatorSettings.GetGameInstallDirs()) {
            if (auto foundPath = Common::FS::FindGameByID(installDir, *gamePath, 0)) {
                ebootPath = *foundPath;
                found = true;
                break;
            }
        }
        if (!found) {
            for (const auto& installDir : EmulatorSettings.GetGameInstallDirs()) {
                if (auto foundPath = Common::FS::FindGameByID(installDir, *gamePath, 5)) {
                    ebootPath = *foundPath;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            LOG_ERROR(Debug, "Game ID or file path not found: {}", *gamePath);
            return 1;
        }
    }

    auto* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(ebootPath, gameArgs, overrideRoot, mounts, env_vars, append_log);

    return 0;
}
