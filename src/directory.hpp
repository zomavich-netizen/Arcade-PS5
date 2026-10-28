// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <vector>
#ifdef ARCADE_PS5
#include "prosperoeden/pe/ui/runtime_filesystem.hpp"
#endif
namespace arcade {
struct DirectoryEntry { std::filesystem::path path; bool directory,regular; };
inline bool directory_available(const std::filesystem::path& path) {
#ifdef ARCADE_PS5
    if(path.empty())return false;
    try { pe::ui::runtime_fs::check(path.string(),pe::ui::runtime_fs::Kind::directory);return true; }
    catch(const std::exception&) { return false; }
#else
    std::error_code error;return std::filesystem::is_directory(path,error);
#endif
}
inline std::vector<DirectoryEntry> directory_entries(const std::filesystem::path& path) {
    std::vector<DirectoryEntry> result;
#ifdef ARCADE_PS5
    for(const auto& name:pe::ui::runtime_fs::entries(path.string())) {
        auto child=path/name;
        try {
            pe::ui::runtime_fs::Descriptor descriptor(child.string(),pe::ui::runtime_fs::Kind::resource);
            struct stat info{};
            if(fstat(descriptor.get(),&info)==0)result.push_back({child,bool(S_ISDIR(info.st_mode)),bool(S_ISREG(info.st_mode))});
        }catch(const std::exception&) { /* Unreadable and special entries are not selectable. */ }
    }
#else
    for(const auto& entry:std::filesystem::directory_iterator(path)) {
        std::error_code error;auto status=entry.status(error);
        if(!error)result.push_back({entry.path(),std::filesystem::is_directory(status),std::filesystem::is_regular_file(status)});
    }
#endif
    return result;
}
}
