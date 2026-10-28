#include "cover_art.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unistd.h>
int main(int argc,char** argv){
 assert(arcade::CoverCache::directory("/data/arcade/roms/")=="/data/arcade/covers");
 assert(argc==2);namespace fs=std::filesystem;using namespace arcade;
 auto root=fs::temp_directory_path()/("arcade-covers-"+std::to_string(getpid()));fs::create_directories(root/"roms");
 std::ifstream image(argv[1],std::ios::binary);std::vector<unsigned char> png{std::istreambuf_iterator<char>(image),{}};assert(valid_cover(png));assert(!valid_cover({'<','h','t','m','l','>'}));
 {std::ofstream cat(root/"catalogue.tsv");cat<<"dino\t0\tCadillacs and Dinosaurs (World 930201)\ndinou\tdino\tCadillacs and Dinosaurs (USA 930201)\n";}
 Game dino{"dino","Dino","",1993,root/"roms/dino.zip"};int requests=0;
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string& url,const std::atomic<bool>&){++requests;assert(url=="https://thumbnails.libretro.com/MAME/Named_Boxarts/Cadillacs%20and%20Dinosaurs%20%28World%20930201%29.png");return CoverResponse{200,png,{}};});cache.discover({dino,dino});cache.wait_idle();assert(requests==1);assert(fs::is_regular_file(root/"covers/dino.png"));assert(!fs::exists(root/"covers/dino.png.part"));}
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string&,const std::atomic<bool>&){assert(false&&"Offline cache hit must not use network");return CoverResponse{};});cache.discover({dino});cache.wait_idle();}
 Game clone{"dinou","Dino US","",1993,root/"roms/dinou.zip"};requests=0;
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string& url,const std::atomic<bool>&){++requests;return url.find("USA")!=std::string::npos?CoverResponse{404,{},{}}:CoverResponse{200,png,{}};});cache.discover({clone});cache.wait_idle();assert(requests==2);assert(fs::exists(root/"covers/dinou.png"));}
 Game bad{"bad","Bad","",0,root/"roms/bad.zip"};
 {CoverCache cache(root/"catalogue.tsv",[](const std::string&,const std::atomic<bool>&){return CoverResponse{200,{'x'}, {}};});cache.discover({bad});cache.wait_idle();assert(!fs::exists(root/"covers/bad.png"));assert(fs::exists(root/"covers/bad.retry"));}
 {CoverCache cache(root/"catalogue.tsv",[](const std::string&,const std::atomic<bool>&){assert(false&&"Retry cooldown must survive restart");return CoverResponse{};});cache.discover({bad});cache.wait_idle();}
 Game traversal{"../escape","Bad","",0,root/"roms/escape.zip"};
 {CoverCache cache(root/"catalogue.tsv",[](const std::string&,const std::atomic<bool>&){assert(false&&"Unsafe ROM ID must not download");return CoverResponse{};});cache.discover({traversal});cache.wait_idle();}
 // Exact indexed filenames bypass title capitalization/punctuation mismatches.
 {std::ofstream cat(root/"catalogue.tsv",std::ios::app);cat<<"kof98\t0\tKing of Fighters 98: Dream Match (World)\n";}
 {std::ofstream aliases(root/"mame-artwork-aliases.tsv");aliases<<"kof98\tNamed_Boxarts\tKing of Fighters 98_ dream match (World)\n";}
 Game kof{"kof98","KOF98","",1998,root/"roms/kof98.zip"};
 // A legacy 24-hour failure must not prevent trying the corrected filename.
 {std::ofstream(root/"covers/kof98.retry")<<"9999999999\n";}
 requests=0;
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string& url,const std::atomic<bool>&){++requests;assert(url==cover_url("King of Fighters 98_ dream match (World)"));return CoverResponse{200,png,{}};});cache.discover({kof});cache.wait_idle();}
 assert(requests==1&&fs::exists(root/"covers/kof98.png")&&!fs::exists(root/"covers/kof98.retry"));
 fs::remove(root/"covers/kof98.png");requests=0;
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string&,const std::atomic<bool>&){++requests;return CoverResponse{404,{},{}};});cache.discover({kof});cache.wait_idle();}assert(requests>0);
 {CoverCache cache(root/"catalogue.tsv",[](const std::string&,const std::atomic<bool>&){assert(false&&"Unchanged indexed lookup keeps cooldown");return CoverResponse{};});cache.discover({kof});cache.wait_idle();}
 // A refreshed filename index invalidates that failure immediately.
 {std::ofstream aliases(root/"mame-artwork-aliases.tsv");aliases<<"kof98\tNamed_Boxarts\tKing of Fighters 98 - Dream Match\n";}
 requests=0;
 {CoverCache cache(root/"catalogue.tsv",[&](const std::string& url,const std::atomic<bool>&){++requests;assert(url==cover_url("King of Fighters 98 - Dream Match"));return CoverResponse{200,png,{}};});cache.discover({kof});cache.wait_idle();}assert(requests==1);
 assert(thumbnail_name("A & B: Test/More?")=="A _ B_ Test_More_");
 fs::remove_all(root);std::cout<<"Cover matching, parent fallback, automatic directory, atomic cache, offline reuse, invalid response and retry cooldown passed\n";
}
