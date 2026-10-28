#include "cover_art.hpp"
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
namespace arcade {
namespace {
constexpr std::size_t max_bytes=8*1024*1024;
bool safe_id(const std::string& id){return !id.empty()&&id.size()<=64&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos;}
std::vector<unsigned char> read_image(const std::filesystem::path& path){std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec||size>max_bytes)return {};std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
}
std::string thumbnail_name(std::string title){for(char& c:title)if(std::string("&*/:`<>?\\|\"").find(c)!=std::string::npos)c='_';return title;}
std::string cover_url(const std::string& title,const std::string& category){
 const auto name=thumbnail_name(title)+".png";const char* hex="0123456789ABCDEF";std::string escaped;
 for(unsigned char c:name){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')escaped+=char(c);else{escaped+='%';escaped+=hex[c>>4];escaped+=hex[c&15];}}
 return "https://thumbnails.libretro.com/MAME/"+category+"/"+escaped;
}
bool valid_cover(const std::vector<unsigned char>& data){
 if(data.empty()||data.size()>max_bytes)return false;int w=0,h=0,n=0;
 if(!stbi_info_from_memory(data.data(),int(data.size()),&w,&h,&n)||w<1||h<1||w>4096||h>4096||std::uint64_t(w)*h>4194304)return false;
 auto* decoded=stbi_load_from_memory(data.data(),int(data.size()),&w,&h,&n,4);if(!decoded)return false;stbi_image_free(decoded);return true;
}
CoverCache::CoverCache(std::filesystem::path catalogue,CoverDownload download):download_(std::move(download)){
 std::ifstream in(catalogue);std::string line;while(std::getline(in,line)){auto a=line.find('\t'),b=line.find('\t',a==std::string::npos?0:a+1);if(a==std::string::npos||b==std::string::npos)continue;auto id=line.substr(0,a);if(safe_id(id))catalogue_[id]={line.substr(b+1),line.substr(a+1,b-a-1)};}
 if(!catalogue.empty()){
  std::ifstream aliases(catalogue.parent_path()/"mame-artwork-aliases.tsv");
  while(std::getline(aliases,line)){auto a=line.find('\t'),b=line.find('\t',a==std::string::npos?0:a+1);if(a==std::string::npos||b==std::string::npos)continue;
   auto id=line.substr(0,a),category=line.substr(a+1,b-a-1),title=line.substr(b+1);
   if(safe_id(id)&&(category=="Named_Boxarts"||category=="Named_Titles"||category=="Named_Snaps")&&!title.empty()&&title.size()<=1024)aliases_[id+"\t"+category]=title;
  }
 }
 worker_=std::thread([this]{run();});
}
CoverCache::~CoverCache(){{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}changed_.notify_all();if(worker_.joinable())worker_.join();}
void CoverCache::discover(const std::vector<Game>& games){
 std::lock_guard<std::mutex> lock(mutex_);
 for(const auto& game:games){if(!safe_id(game.id)||game.path.empty())continue;auto dir=directory(game.path.parent_path());auto key=(dir/game.id).string();auto now=std::chrono::steady_clock::now();auto found=seen_.find(key);if(found==seen_.end()||found->second<=now){seen_[key]=now+std::chrono::minutes(5);jobs_.push_back({game.id,dir});}}
 changed_.notify_one();
}
void CoverCache::wait_idle(){std::unique_lock<std::mutex> lock(mutex_);idle_.wait(lock,[this]{return jobs_.empty()&&!busy_;});}
void CoverCache::run(){
 for(;;){Job job;{std::unique_lock<std::mutex> lock(mutex_);changed_.wait(lock,[this]{return stop_||!jobs_.empty();});if(stop_)return;job=jobs_.front();jobs_.pop_front();busy_=true;}
  try{fetch(job);}catch(const std::exception& e){std::fprintf(stderr,"[covers] %s: %s\n",job.id.c_str(),e.what());}
  {std::lock_guard<std::mutex> lock(mutex_);busy_=false;if(jobs_.empty())idle_.notify_all();}
 }
}
void CoverCache::fetch(const Job& job){
 std::filesystem::create_directories(job.directory);auto path=job.directory/(job.id+".png");
 if(valid_cover(read_image(path)))return;
 std::vector<std::string> ids;std::set<std::string> visited;auto id=job.id;
 for(int depth=0;depth<4&&visited.insert(id).second;++depth){ids.push_back(id);auto found=catalogue_.find(id);if(found==catalogue_.end())break;id=found->second.parent;if(id=="0")break;}
 std::vector<std::string> urls;std::set<std::string> unique;
 for(const auto& category:{"Named_Boxarts","Named_Titles","Named_Snaps"})for(const auto& candidate:ids){
  auto add=[&](const std::string& title){auto url=cover_url(title,category);if(unique.insert(url).second)urls.push_back(std::move(url));};
  auto alias=aliases_.find(candidate+"\t"+category);if(alias!=aliases_.end())add(alias->second);
  auto found=catalogue_.find(candidate);add(found==catalogue_.end()?candidate:found->second.title);
 }
 // Failed lookups are valid only for the same resolver and candidate filenames.
 // Legacy markers and changed aliases retry immediately instead of waiting a day.
 std::uint64_t hash=14695981039346656037ull;for(const auto& url:urls){for(unsigned char ch:url){hash^=ch;hash*=1099511628211ull;}hash^=0xff;hash*=1099511628211ull;}
 auto fingerprint="indexed-v1-"+std::to_string(hash);
 auto marker=job.directory/(job.id+".retry");std::ifstream retry(marker);long long next=0;std::string previous;retry>>next>>previous;retry.close();
 auto now=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(next>now&&previous==fingerprint)return;
 bool missing=true;
 for(const auto& url:urls){
  if(stop_)return;auto response=download_(url,stop_);if(stop_)return;
  if(response.status==404)continue;
  if(response.status!=200||!valid_cover(response.bytes)){missing=false;if(response.status!=200){std::fprintf(stderr,"[covers] %s: download unavailable (%d) %s\n",job.id.c_str(),response.status,response.error.c_str());goto finished;}continue;}
  {auto temporary=path;temporary+=".part";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(response.bytes.data()),response.bytes.size());out.close();if(!out){std::filesystem::remove(temporary);throw std::runtime_error("Cannot save cover image");}std::filesystem::rename(temporary,path);std::filesystem::remove(marker);std::fprintf(stderr,"[covers] Saved %s\n",path.c_str());return;}
 }
finished:
 // Retry missing artwork tomorrow, temporary network failures after five minutes.
 std::ofstream(marker)<<now+(missing?86400:300)<<'\n'<<fingerprint<<'\n';
}
}
