// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Restored from shadPS4 v.0.7.0 (src/core/file_format/pkg.cpp and the parts of
// src/core/crypto/crypto.cpp it used). Changes from 0.7.0:
// - CryptoPP replaced by OpenSSL/LibreSSL (EVP AES-128-CBC and AES-128-XTS, HMAC-SHA256,
//   SHA-256, BIGNUM for the two RSA-2048 PKCS#1 v1.5 decryptions);
// - file sizes held in 64 bits (0.7.0 used int, which breaks files of 2 GiB and more);
// - a key that does not open, or a block that does not decompress, is an error instead of
//   silently writing garbage;
// - the root folder is the folder asked for, not <its parent>/<title id>.

#include <algorithm>
#include <cstring>
#include <memory>
#include <fmt/format.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <zlib.h>

#include "common/io_file.h"
#include "common/logging/log.h"
#include "core/file_format/pkg.h"
// The key sets are not kept in the repository. Generate the file locally from shadPS4
// v.0.7.0's src/core/crypto/keys.h; without it, --extract-pkg reports that it is missing.
#if __has_include("core/file_format/pkg_keys.h")
#include "core/file_format/pkg_keys.h"
#define PKG_HAVE_KEYS 1
#else
#define PKG_HAVE_KEYS 0
namespace PkgKeys {
struct PkgDerivedKey3Keyset {
    static constexpr u8 Modulus[1] = {};
    static constexpr u8 PrivateExponent[1] = {};
};
using FakeKeyset = PkgDerivedKey3Keyset;
} // namespace PkgKeys
#endif
#include "core/file_format/pkg_type.h"

namespace {

constexpr u32 PkgMagic = 0x7F434E54;
constexpr u32 PfscMagic = 0x43534650;
constexpr u64 PfscBlock = 0x10000;
constexpr u64 XtsSector = 0x1000;

bool DecompressPFSC(std::span<const u8> compressed, std::span<u8> decompressed) {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }
    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_in = const_cast<Bytef*>(compressed.data());
    stream.avail_out = static_cast<uInt>(decompressed.size());
    stream.next_out = decompressed.data();
    const int ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    return ret == Z_STREAM_END || (ret == Z_OK && stream.avail_out == 0) ||
           (ret == Z_BUF_ERROR && stream.avail_out == 0);
}

/// One PFSC block: stored as is when it fills a whole block, deflated otherwise.
bool ReadBlock(std::span<const u8> stored, std::span<u8> out) {
    if (stored.size() == PfscBlock) {
        std::memcpy(out.data(), stored.data(), PfscBlock);
        return true;
    }
    if (stored.size() > PfscBlock) {
        return false;
    }
    return DecompressPFSC(stored, out);
}

u64 GetPFSCOffset(std::span<const u8> pfs_image) {
    for (u64 i = 0x20000; i + 4 <= pfs_image.size(); i += PfscBlock) {
        u32 value;
        std::memcpy(&value, &pfs_image[i], sizeof(u32));
        if (value == PfscMagic) {
            return i;
        }
    }
    return ~0ULL;
}

/// RSA-2048 decryption with a private key, then PKCS#1 v1.5 (type 2) unpadding. The result
/// must be exactly `out.size()` bytes, as every key the package format stores is.
template <typename Keyset>
bool RSA2048Decrypt(std::span<u8> out, std::span<const u8, 256> cipher) {
    std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx(BN_CTX_new(), BN_CTX_free);
    std::unique_ptr<BIGNUM, decltype(&BN_free)> c(BN_bin2bn(cipher.data(), 256, nullptr),
                                                  BN_free);
    std::unique_ptr<BIGNUM, decltype(&BN_free)> n(
        BN_bin2bn(Keyset::Modulus, sizeof(Keyset::Modulus), nullptr), BN_free);
    std::unique_ptr<BIGNUM, decltype(&BN_free)> d(
        BN_bin2bn(Keyset::PrivateExponent, sizeof(Keyset::PrivateExponent), nullptr), BN_free);
    std::unique_ptr<BIGNUM, decltype(&BN_free)> m(BN_new(), BN_free);
    if (!ctx || !c || !n || !d || !m || BN_mod_exp(m.get(), c.get(), d.get(), n.get(), ctx.get()) != 1) {
        return false;
    }
    std::array<u8, 256> em{};
    if (BN_bn2binpad(m.get(), em.data(), 256) != 256 || em[0] != 0 || em[1] != 2) {
        return false;
    }
    const auto sep = std::find(em.begin() + 2, em.end(), u8{0});
    if (sep == em.end() || sep - em.begin() < 10) {
        return false;
    }
    const auto len = static_cast<size_t>(em.end() - (sep + 1));
    if (len != out.size()) {
        return false;
    }
    std::memcpy(out.data(), &*(sep + 1), len);
    return true;
}

/// AES-128-CBC without padding; key = ivkey[16..32), iv = ivkey[0..16) (as 0.7.0).
bool AesCbcDecrypt(std::span<const u8, 32> ivkey, std::span<const u8> in, std::span<u8> out) {
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                       EVP_CIPHER_CTX_free);
    int len = 0;
    int final_len = 0;
    return ctx && in.size() % 16 == 0 && out.size() >= in.size() &&
           EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_cbc(), nullptr, ivkey.data() + 16,
                              ivkey.data()) == 1 &&
           EVP_CIPHER_CTX_set_padding(ctx.get(), 0) == 1 &&
           EVP_DecryptUpdate(ctx.get(), out.data(), &len, in.data(), static_cast<int>(in.size())) ==
               1 &&
           EVP_DecryptFinal_ex(ctx.get(), out.data() + len, &final_len) == 1;
}

/// sha256(entry || dk3): the iv and key of an encrypted entry.
std::array<u8, 32> EntryIvKey(const PKGEntry& entry, std::span<const u8, 32> dk3) {
    std::array<u8, 64> input{};
    std::memcpy(input.data(), &entry, sizeof(entry));
    std::memcpy(input.data() + sizeof(entry), dk3.data(), dk3.size());
    std::array<u8, 32> digest{};
    SHA256(input.data(), input.size(), digest.data());
    return digest;
}

/// The PFS data and tweak keys: HMAC-SHA256(ekpfs, le32(1) || seed), tweak first.
bool PfsGenCryptoKey(std::span<const u8, 32> ekpfs, std::span<const u8, 16> seed,
                     std::span<u8, 16> dataKey, std::span<u8, 16> tweakKey) {
    std::array<u8, 20> d{};
    const u32 index = 1;
    std::memcpy(d.data(), &index, sizeof(index));
    std::memcpy(d.data() + sizeof(index), seed.data(), seed.size());
    std::array<u8, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    if (!HMAC(EVP_sha256(), ekpfs.data(), static_cast<int>(ekpfs.size()), d.data(), d.size(),
              digest.data(), &digest_len) ||
        digest_len < 32) {
        return false;
    }
    std::memcpy(tweakKey.data(), digest.data(), 16);
    std::memcpy(dataKey.data(), digest.data() + 16, 16);
    return true;
}

/// AES-128-XTS over whole 0x1000-byte sectors, the first one numbered `sector` (0.7.0's
/// hand-rolled XTS is the standard one: tweak = sector number, little endian).
bool DecryptPFS(std::span<const u8, 16> dataKey, std::span<const u8, 16> tweakKey,
                std::span<const u8> src, std::span<u8> dst, u64 sector) {
    std::array<u8, 32> key{};
    std::memcpy(key.data(), dataKey.data(), 16);
    std::memcpy(key.data() + 16, tweakKey.data(), 16);
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                       EVP_CIPHER_CTX_free);
    if (!ctx || src.size() % XtsSector != 0 || dst.size() < src.size() ||
        EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_xts(), nullptr, key.data(), nullptr) != 1) {
        return false;
    }
    for (u64 i = 0; i < src.size(); i += XtsSector) {
        std::array<u8, 16> iv{};
        const u64 current = sector + i / XtsSector;
        std::memcpy(iv.data(), &current, sizeof(current));
        int len = 0;
        if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, nullptr, iv.data()) != 1 ||
            EVP_DecryptUpdate(ctx.get(), dst.data() + i, &len, src.data() + i,
                              static_cast<int>(XtsSector)) != 1 ||
            len != static_cast<int>(XtsSector)) {
            return false;
        }
    }
    return true;
}

} // namespace

PKG::PKG() = default;

PKG::~PKG() = default;

const std::filesystem::path& PKG::GetPath(int index) const {
    return extractPaths.at(fsTable[index].inode);
}

bool PKG::IsFile(int index) const {
    return fsTable[index].type == PFS_FILE;
}

s64 PKG::GetFileSize(int index) const {
    return IsFile(index) ? iNodeBuf[fsTable[index].inode].Size : 0;
}

bool PKG::Extract(const std::filesystem::path& filepath, const std::filesystem::path& extract,
                  std::string& failreason) {
    if (!PKG_HAVE_KEYS) {
        failreason = "This build has no src/core/file_format/pkg_keys.h (the key sets are not "
                     "kept in the repository); generate it from shadPS4 v.0.7.0's "
                     "src/core/crypto/keys.h and rebuild";
        return false;
    }
    extract_path = extract;
    pkgpath = filepath;
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        failreason = "Cannot open the PKG file";
        return false;
    }
    pkgSize = file.GetSize();
    file.ReadRaw<u8>(&pkgheader, sizeof(PKGHeader));

    if (pkgheader.magic != PkgMagic) {
        failreason = "Not a PS4 PKG file (bad magic)";
        return false;
    }
    if (pkgheader.pkg_size > pkgSize) {
        failreason = "PKG file size is different";
        return false;
    }
    if ((pkgheader.pkg_content_size + pkgheader.pkg_content_offset) > pkgheader.pkg_size) {
        failreason = "Content size is bigger than pkg size";
        return false;
    }
    std::memcpy(pkgTitleID, pkgheader.pkg_content_id + 7, sizeof(pkgTitleID));

    const u32 offset = pkgheader.pkg_table_entry_offset;
    const u32 n_files = pkgheader.pkg_table_entry_count;
    std::array<u8, 256> imgkeydata{};
    bool have_dk3 = false;
    bool have_ekpfs = false;

    std::filesystem::create_directories(extract_path / "sce_sys");

    for (u32 i = 0; i < n_files; i++) {
        PKGEntry entry{};
        if (!file.Seek(offset + i * sizeof(PKGEntry))) {
            failreason = "Failed to seek to PKG table entry offset";
            return false;
        }
        file.ReadRaw<u8>(&entry, sizeof(entry));

        const auto name = GetEntryNameByType(entry.id);
        const auto out_path =
            extract_path / "sce_sys" / (name.empty() ? std::to_string(entry.id) : std::string(name));
        std::filesystem::create_directories(out_path.parent_path());

        std::vector<u8> data(entry.size);
        if (!file.Seek(entry.offset)) {
            failreason = "Failed to seek to PKG entry offset";
            return false;
        }
        file.ReadRaw<u8>(data.data(), data.size());

        if (entry.id == 0x10) { // ENTRY_KEYS: seed digest, 7 digests, 7 keys; key 3 is DK3
            constexpr size_t key3 = 32 + 7 * 32 + 3 * 256;
            if (data.size() < key3 + 256 ||
                !RSA2048Decrypt<PkgKeys::PkgDerivedKey3Keyset>(
                    dk3_, std::span<const u8, 256>(data.data() + key3, 256))) {
                failreason = "The PKG's entry key does not open with the fake-package key set "
                             "(not a fake package?)";
                return false;
            }
            have_dk3 = true;
        } else if (entry.id == 0x20) { // IMAGE_KEY: AES-CBC with sha256(entry || dk3), then RSA
            if (!have_dk3 || data.size() < imgkeydata.size()) {
                failreason = "IMAGE_KEY came before ENTRY_KEYS";
                return false;
            }
            std::memcpy(imgkeydata.data(), data.data(), imgkeydata.size());
            const auto ivkey = EntryIvKey(entry, dk3_);
            std::array<u8, 256> imgKey{};
            if (!AesCbcDecrypt(ivkey, imgkeydata, imgKey) ||
                !RSA2048Decrypt<PkgKeys::FakeKeyset>(ekpfsKey,
                                                     std::span<const u8, 256>(imgKey))) {
                failreason = "The PKG's image key does not open with the fake-package key set";
                return false;
            }
            have_ekpfs = true;
        } else if (entry.id >= 0x400 && entry.id <= 0x403 && have_dk3) {
            // license.dat, license.info, nptitle.dat, npbind.dat: AES-CBC encrypted
            std::vector<u8> plain(data.size());
            const auto ivkey = EntryIvKey(entry, dk3_);
            const size_t whole = data.size() / 16 * 16;
            if (AesCbcDecrypt(ivkey, std::span<const u8>(data.data(), whole), plain)) {
                std::copy(data.begin() + whole, data.end(), plain.begin() + whole);
                data = std::move(plain);
            }
        }

        // Create, not Write: Write opens an existing file only (r+b) since 0.7.0.
        Common::FS::IOFile out(out_path, Common::FS::FileAccessMode::Create);
        if (!out.IsOpen()) {
            failreason = "Failed to create " + out_path.string();
            return false;
        }
        out.WriteRaw<u8>(data.data(), data.size());
    }
    if (!have_ekpfs) {
        failreason = "The PKG has no IMAGE_KEY entry";
        return false;
    }

    // The seed, then the data and tweak keys.
    std::array<u8, 16> seed{};
    if (!file.Seek(pkgheader.pfs_image_offset + 0x370)) {
        failreason = "Failed to seek to PFS image offset";
        return false;
    }
    file.Read(seed);
    if (!PfsGenCryptoKey(ekpfsKey, seed, dataKey, tweakKey)) {
        failreason = "Cannot derive the PFS keys";
        return false;
    }

    const u64 length = static_cast<u64>(pkgheader.pfs_cache_size) * 2; // as 0.7.0
    if (length == 0 || length % XtsSector != 0) {
        failreason = "Unexpected PFS cache size";
        return false;
    }
    std::vector<u8> pfs_encrypted(length);
    std::vector<u8> pfs_decrypted(length);
    file.Seek(pkgheader.pfs_image_offset);
    file.ReadRaw<u8>(pfs_encrypted.data(), length);
    file.Close();
    if (!DecryptPFS(dataKey, tweakKey, pfs_encrypted, pfs_decrypted, 0)) {
        failreason = "PFS image decryption failed";
        return false;
    }

    pfsc_offset = GetPFSCOffset(pfs_decrypted);
    if (pfsc_offset == ~0ULL) {
        failreason = "No PFSC image found (wrong keys?)";
        return false;
    }
    const std::span<const u8> pfsc(pfs_decrypted.data() + pfsc_offset, length - pfsc_offset);

    PFSCHdr pfsChdr;
    std::memcpy(&pfsChdr, pfsc.data(), sizeof(pfsChdr));
    if (pfsChdr.block_sz2 != static_cast<s64>(PfscBlock)) {
        failreason = "Unexpected PFSC block size";
        return false;
    }
    const u64 num_blocks = pfsChdr.data_length / pfsChdr.block_sz2;
    if (pfsChdr.block_offsets + (num_blocks + 1) * 8 > pfsc.size()) {
        failreason = "PFSC block map does not fit in the PFS cache";
        return false;
    }
    sectorMap.resize(num_blocks + 1); // one more for the end of the last block
    std::memcpy(sectorMap.data(), pfsc.data() + pfsChdr.block_offsets, (num_blocks + 1) * 8);

    u32 ent_size = 0;
    u32 ndinode = 0;
    int ndinode_counter = 0;
    bool dinode_reached = false;
    bool uroot_reached = false;
    std::vector<u8> block(PfscBlock);

    // The inodes and the directory entries.
    for (u64 i = 0; i < num_blocks; i++) {
        const u64 sectorOffset = sectorMap[i];
        const u64 sectorSize = sectorMap[i + 1] - sectorOffset;
        if (sectorOffset + sectorSize > pfsc.size()) {
            failreason = "Directory data lies beyond the PFS cache";
            return false;
        }
        std::fill(block.begin(), block.end(), u8{0});
        if (!ReadBlock(pfsc.subspan(sectorOffset, sectorSize), block)) {
            failreason = "A directory block does not decompress";
            return false;
        }

        if (i == 0) {
            std::memcpy(&ndinode, block.data() + 0x30, 4); // number of folders and files
        }

        u64 occupied_blocks = (static_cast<u64>(ndinode) * 0xA8) / PfscBlock; // inode blocks
        if (((static_cast<u64>(ndinode) * 0xA8) % PfscBlock) != 0) {
            occupied_blocks += 1;
        }

        if (i >= 1 && i <= occupied_blocks) { // inodes: type, size and place of each file
            for (u64 p = 0; p + sizeof(Inode) <= PfscBlock; p += 0xA8) {
                Inode node;
                std::memcpy(&node, &block[p], sizeof(node));
                if (node.Mode == 0) {
                    break;
                }
                iNodeBuf.push_back(node);
            }
        }

        // The root/uroot entries (sometimes more than two).
        const std::string_view flat_path_table(reinterpret_cast<const char*>(&block[0x10]), 15);
        if (flat_path_table == "flat_path_table") {
            uroot_reached = true;
        }

        if (uroot_reached) {
            for (u64 j = 0; j + 16 <= PfscBlock; j += ent_size) {
                Dirent dirent;
                std::memcpy(&dirent, &block[j], std::min<u64>(sizeof(dirent), PfscBlock - j));
                ent_size = dirent.entsize;
                if (dirent.ino != 0) {
                    ndinode_counter++;
                } else {
                    extractPaths[ndinode_counter] = extract_path;
                    uroot_reached = false;
                    break;
                }
                if (ent_size == 0) {
                    break;
                }
            }
        }

        const char dot = static_cast<char>(block[0x10]);
        const std::string_view dotdot(reinterpret_cast<const char*>(&block[0x28]), 2);
        if (dot == '.' && dotdot == "..") {
            dinode_reached = true;
        }

        // Folder and file names.
        bool end_reached = false;
        if (dinode_reached) {
            for (u64 j = 0; j + 16 <= PfscBlock; j += ent_size) {
                Dirent dirent;
                std::memcpy(&dirent, &block[j], std::min<u64>(sizeof(dirent), PfscBlock - j));
                if (dirent.ino == 0) {
                    break;
                }
                ent_size = dirent.entsize;
                if (ent_size == 0 || dirent.namelen < 0 || dirent.namelen > 512) {
                    failreason = "A directory entry is malformed";
                    return false;
                }
                auto& table = fsTable.emplace_back();
                table.name = std::string(dirent.name, dirent.namelen);
                table.inode = dirent.ino;
                table.type = dirent.type;

                if (table.type == PFS_CURRENT_DIR) {
                    current_dir = extractPaths[table.inode];
                }
                extractPaths[table.inode] = current_dir / std::filesystem::path(table.name);

                if (table.type == PFS_FILE || table.type == PFS_DIR) {
                    if (table.type == PFS_DIR) {
                        std::filesystem::create_directories(extractPaths[table.inode]);
                    }
                    ndinode_counter++;
                    if ((ndinode_counter + 1) == static_cast<int>(ndinode)) { // +1: the root
                        end_reached = true;
                    }
                }
            }
            if (end_reached) {
                break;
            }
        }
    }
    LOG_INFO(Loader, "PKG {}: {} directory entries, {} inodes, {} PFSC blocks",
             GetTitleID(), fsTable.size(), iNodeBuf.size(), num_blocks);
    return true;
}

bool PKG::ExtractFiles(const int index, std::string& failreason) {
    const auto& entry = fsTable[index];
    if (entry.type != PFS_FILE) {
        return true;
    }
    if (entry.inode >= iNodeBuf.size()) {
        failreason = fmt::format("{}: inode {} out of range", entry.name, entry.inode);
        return false;
    }
    const Inode& node = iNodeBuf[entry.inode];
    const u64 sector_loc = node.loc;
    const u64 nblocks = node.Blocks;
    const s64 bsize = node.Size;
    if (sector_loc + nblocks >= sectorMap.size()) {
        failreason = fmt::format("{}: blocks lie beyond the block map", entry.name);
        return false;
    }

    // .at(): several threads run this at once, so the map must not be changed here
    Common::FS::IOFile inflated(extractPaths.at(entry.inode), Common::FS::FileAccessMode::Create);
    Common::FS::IOFile pkgFile(pkgpath, Common::FS::FileAccessMode::Read); // one per call
    if (!inflated.IsOpen() || !pkgFile.IsOpen()) {
        failreason = fmt::format("{}: cannot open the output or the PKG", entry.name);
        return false;
    }

    std::vector<u8> encrypted(PfscBlock + 2 * XtsSector);
    std::vector<u8> decrypted(PfscBlock + 2 * XtsSector);
    std::vector<u8> block(PfscBlock);
    s64 written = 0;

    for (u64 j = 0; j < nblocks; j++) {
        const u64 sectorOffset = sectorMap[sector_loc + j]; // into the PFSC image
        const u64 sectorSize = sectorMap[sector_loc + j + 1] - sectorOffset;
        if (sectorSize > PfscBlock) {
            failreason = fmt::format("{}: block {} is {} bytes", entry.name, j, sectorSize);
            return false;
        }
        const u64 start = pfsc_offset + sectorOffset;      // into the PFS image
        const u64 first_sector = start / XtsSector;         // XTS sectors are 0x1000 bytes
        const u64 previousData = start - first_sector * XtsSector;
        const u64 span = (previousData + sectorSize + XtsSector - 1) / XtsSector * XtsSector;

        pkgFile.Seek(pkgheader.pfs_image_offset + first_sector * XtsSector);
        if (pkgFile.ReadRaw<u8>(encrypted.data(), span) != span ||
            !DecryptPFS(dataKey, tweakKey, std::span<const u8>(encrypted.data(), span), decrypted,
                        first_sector)) {
            failreason = fmt::format("{}: block {} cannot be read or decrypted", entry.name, j);
            return false;
        }
        std::fill(block.begin(), block.end(), u8{0});
        if (!ReadBlock(std::span<const u8>(decrypted.data() + previousData, sectorSize), block)) {
            failreason = fmt::format("{}: block {} does not decompress", entry.name, j);
            return false;
        }
        const s64 left = bsize - written;
        const s64 n = std::min<s64>(left, static_cast<s64>(PfscBlock));
        if (n > 0) {
            inflated.WriteRaw<u8>(block.data(), static_cast<size_t>(n));
            written += n;
        }
    }
    if (written != bsize) {
        failreason =
            fmt::format("{}: wrote {} bytes, the inode says {}", entry.name, written, bsize);
        return false;
    }
    return true;
}
