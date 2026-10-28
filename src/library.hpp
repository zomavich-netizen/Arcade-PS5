#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <map>
namespace arcade {
struct Game { std::string id, title, maker; int year=0; std::filesystem::path path;
 std::string hardware="Unknown",region="Unknown",compatibility="unknown",save_support="unknown";
 unsigned players=0;
};
using GameMetadata=std::map<std::string,Game>;
GameMetadata read_game_metadata(const std::filesystem::path& file);
std::vector<Game> scan(const std::filesystem::path& directory,const GameMetadata& metadata={});
std::set<std::string> read_favorites(const std::filesystem::path& file);
void write_favorites(const std::filesystem::path& file, const std::set<std::string>& values);
std::vector<Game> sample_library();
}
