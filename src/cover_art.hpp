#pragma once
#include "library.hpp"
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <thread>
namespace arcade {
struct CoverEntry {std::string title,parent;};
struct CoverResponse {int status=0;std::vector<unsigned char> bytes;std::string error;};
using CoverDownload=std::function<CoverResponse(const std::string&,const std::atomic<bool>&)>;
std::string thumbnail_name(std::string);
std::string cover_url(const std::string& title,const std::string& category="Named_Boxarts");
bool valid_cover(const std::vector<unsigned char>&);
class CoverCache {
public:
 CoverCache(std::filesystem::path catalogue,CoverDownload download);
 ~CoverCache();
 void discover(const std::vector<Game>&);
 void wait_idle();
 static std::filesystem::path directory(const std::filesystem::path& roms){auto path=roms.lexically_normal();if(path.filename().empty())path=path.parent_path();return path.parent_path()/"covers";}
private:
 struct Job {std::string id;std::filesystem::path directory;};
 std::map<std::string,CoverEntry> catalogue_;
 std::map<std::string,std::string> aliases_;
 CoverDownload download_;std::atomic<bool> stop_{false};std::thread worker_;
 std::mutex mutex_;std::condition_variable changed_,idle_;std::deque<Job> jobs_;std::map<std::string,std::chrono::steady_clock::time_point> seen_;bool busy_=false;
 void run();void fetch(const Job&);
};
CoverResponse download_cover(const std::string&,const std::atomic<bool>&);
}
