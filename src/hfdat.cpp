#include "pch.h"

#include "archive.h"
#include "archive_entry.h"
#include "hfdat.h"
#include "settings.h"

namespace dhf::hfdat {

namespace {

const constexpr auto ARCHIVE_BLOCK_SIZE = 0x4000;

const constexpr auto NO_LOADED_FILE = "n/a";

std::filesystem::path hfdat_path;

std::vector<std::string> hotfix_names_internal;
std::string hfdat_name_internal = NO_LOADED_FILE;

bool use_current_hotfixes_internal = false;
std::vector<std::pair<std::wstring, std::wstring>> hotfixes_internal;
std::string loaded_hotfixes_name_internal = NO_LOADED_FILE;

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

/**
 * @brief Reads a value from the current position in the current archive entry.
 *
 * @tparam T The type to read.
 * @param archive The archive to read from.
 * @return The value.
 */
template <typename T>
T read_from_archive(const std::shared_ptr<archive>& archive) {
    T value;
    auto ret = archive_read_data(archive.get(), &value, sizeof(value));
    if (ret < (la_ssize_t)sizeof(value)) {
        throw std::runtime_error("Fail to read from archive: " + std::to_string(ret));
    }
    return value;
}

/**
 * @brief Reads a string from the current position in the current archive entry.
 *
 * @param archive The archive to read from.
 * @param len The length of the string to read.
 * @return The string.
 */
std::wstring read_string_from_archive(const std::shared_ptr<archive>& archive, size_t len) {
    auto byte_len = sizeof(wchar_t) * len;

    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
    auto* buf = reinterpret_cast<wchar_t*>(malloc(byte_len));

    auto ret = archive_read_data(archive.get(), buf, byte_len);
    if (ret < (la_ssize_t)byte_len) {
        throw std::runtime_error("Fail to read from archive: " + std::to_string(ret));
    }

    std::wstring str{buf, len};
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
    free(buf);

    return str;
}

}  // namespace

const std::vector<std::string>& hotfix_names = hotfix_names_internal;
const std::string& hfdat_name = hfdat_name_internal;
const bool& use_current_hotfixes = use_current_hotfixes_internal;
const std::vector<std::pair<std::wstring, std::wstring>>& hotfixes = hotfixes_internal;
const std::string& loaded_hotfixes_name = loaded_hotfixes_name_internal;

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
        return;
    }
    hfdat_name_internal = hfdat_path.filename().generic_string();

    auto archive = open_archive(hfdat_path);

    archive_entry* entry{};
    while (archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK) {
        hotfix_names_internal.emplace_back(archive_entry_pathname_utf8(entry));
    }
}

void load_new_hotfixes(const std::string& name, LoadType type) {
    loaded_hotfixes_name_internal = NO_LOADED_FILE;
    hotfixes_internal.clear();
    use_current_hotfixes_internal = false;

    if (type == LoadType::NONE) {
        loaded_hotfixes_name_internal = name;
        hotfixes_internal.clear();
        return;
    }
    if (type == LoadType::CURRENT) {
        loaded_hotfixes_name_internal = name;
        use_current_hotfixes_internal = true;
        return;
    }

    try {
        auto archive = open_archive(hfdat_path);

        bool found = false;
        archive_entry* entry{};
        while (archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK) {
            if (memcmp(name.c_str(), archive_entry_pathname_utf8(entry), name.size()) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return;
        }

        auto num_hotfixes = read_from_archive<uint32_t>(archive);
        hotfixes_internal.reserve(num_hotfixes);

        for (uint32_t i = 0; i < num_hotfixes; i++) {
            auto key_size = read_from_archive<uint32_t>(archive);
            auto key = read_string_from_archive(archive, key_size);

            auto value_size = read_from_archive<uint32_t>(archive);
            auto value = read_string_from_archive(archive, value_size);

            hotfixes_internal.emplace_back(std::move(key), std::move(value));
        }

        loaded_hotfixes_name_internal = name;
    } catch (const std::exception& ex) {
        std::cerr << "[dhf] Failed to read hotfix file '" << name
                  << "' from archive: " << ex.what();
    }
}

}  // namespace dhf::hfdat
