#include "pch.h"

#include "archive.h"
#include "archive_entry.h"
#include "hfdat.h"
#include "settings.h"

namespace dhf::hfdat {

namespace {

const constexpr auto ARCHIVE_BLOCK_SIZE = 0x4000;

std::filesystem::path hfdat_path;

std::vector<std::string> hotfix_names_internal;
std::string hfdat_name_internal;

/**
 * @brief Opens an archive at the given path.
 *
 * @param path The path to the archive.
 * @return A pointer to the archive.
 */
std::shared_ptr<archive> open_archive(const std::filesystem::path& path) {
    std::shared_ptr<archive> ptr{
        archive_read_new(), [](void* data) { archive_free(reinterpret_cast<archive*>(data)); }};

    archive_read_support_filter_all(ptr.get());
    archive_read_support_format_all(ptr.get());

    auto ret =
        archive_read_open_filename(ptr.get(), path.generic_string().c_str(), ARCHIVE_BLOCK_SIZE);
    if (ret != ARCHIVE_OK) {
        throw std::runtime_error("Failed to open archive: " + std::to_string(ret));
    }

    return ptr;
}

}  // namespace

const std::vector<std::string>& hotfix_names = hotfix_names_internal;
const std::string& hfdat_name = hfdat_name_internal;

void init(void) {
    for (const auto& dir_entry :
         std::filesystem::directory_iterator{settings::dll_path.parent_path()}) {
        auto& path = dir_entry.path();
        if (!dir_entry.is_directory() && path.extension() == ".hfdat") {
            hfdat_path = path;
            break;
        }
    }

    if (!std::filesystem::exists(hfdat_path)) {
        hfdat_name_internal = "n/a";
        return;
    }
    hfdat_name_internal = hfdat_path.filename().generic_string();

    auto archive = open_archive(hfdat_path);

    archive_entry* entry{};
    while (archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK) {
        hotfix_names_internal.emplace_back(archive_entry_pathname_utf8(entry));
    }
}

}  // namespace dhf::hfdat
