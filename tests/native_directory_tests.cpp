// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the native reader with host directory records and injected malformed reads.
#include "directory.hpp"
#include "library.hpp"
#include <cassert>
#include <fstream>
#include <cstring>
#include <unistd.h>
static bool malformed=false;
static unsigned reads=0;
extern "C" int sceKernelGetdents(int fd,char* buffer,int capacity) {
    ++reads;assert(capacity==65536);
    if(malformed)return capacity+1;
    DIR* directory=fdopendir(dup(fd));assert(directory);
    int used=0;
    while(auto* entry=readdir(directory)) {
        const auto size=offsetof(dirent,d_name)+std::strlen(entry->d_name)+1;
        assert(used+size<static_cast<unsigned>(capacity));
        std::memcpy(buffer+used,entry,size);
        std::uint16_t length=size;
        std::memcpy(buffer+used+offsetof(dirent,d_reclen),&length,sizeof length);
        used+=size;
    }
    closedir(directory);return used;
}
int main() {
    namespace fs=std::filesystem;
    auto root=fs::temp_directory_path()/("arcade-native-dir-"+std::to_string(getpid()));
    fs::create_directories(root/"subfolder");
    std::ofstream(root/"robby.zip")<<"fixture";
    std::ofstream(root/"notes.txt")<<"ignore";
    fs::create_directory_symlink(root/"subfolder",root/"linked-folder");
    assert(arcade::directory_available(root));
    assert(!arcade::directory_available(root/"missing"));
    assert(!arcade::directory_available(root/"linked-folder"));
    auto entries=arcade::directory_entries(root);
    assert(entries.size()==3); // link is excluded; files and real directory remain.
    auto games=arcade::scan(root);
    assert(games.size()==1&&games.front().id=="robby");
    assert(reads>=4); // Both folder listing and game scanning use the native branch.
    malformed=true;bool rejected=false;
    try { (void)arcade::directory_entries(root); }catch(const std::exception&) {rejected=true;}
    assert(rejected);
    fs::remove_all(root);
}
