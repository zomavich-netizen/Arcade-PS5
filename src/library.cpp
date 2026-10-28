#include "library.hpp"
#include "directory.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
namespace arcade {
namespace {
const std::map<std::string, Game> known={
 {"robby",{"robby","Robby Roto","Bally / Midway",1981,{}}},
 {"gridlee",{"gridlee","Gridlee","Videa",1982,{}}},
 {"circus",{"circus","Circus","Exidy",1977,{}}},
 {"alienar",{"alienar","Alien Arena","Duncan Brown",1985,{}}},
 {"looping",{"looping","Looping","Video Games GmbH",1982,{}}},
 {"targ",{"targ","Targ","Exidy",1980,{}}},
 {"pacman",{"pacman","Pac-Man","Namco",1980,{}}},
 {"mslug3",{"mslug3","Metal Slug 3","SNK",2000,{}}},
 {"dino",{"dino","Cadillacs and Dinosaurs","Capcom",1993,{}}},
 {"sf2",{"sf2","Street Fighter II","Capcom",1991,{}}},
 {"galaga",{"galaga","Galaga","Namco",1981,{}}}};
std::string lower(std::string s){for(char& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
}
GameMetadata read_game_metadata(const std::filesystem::path& file){
 GameMetadata result;std::ifstream input(file);std::string line;
 while(std::getline(input,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  std::vector<std::string> fields;size_t begin=0;
  for(;;){auto tab=line.find('\t',begin);fields.push_back(line.substr(begin,tab==std::string::npos?tab:tab-begin));if(tab==std::string::npos)break;begin=tab+1;}
  if(fields.size()!=4&&fields.size()!=9)continue;
  Game game;game.id=fields[0];game.title=fields[1];game.maker=fields[2];
  if(game.id.empty()||game.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos||game.title.empty())continue;
  if(game.maker.empty())game.maker="Unknown company";
  const auto& year=fields[3];
  if(year.size()==4&&year.find_first_not_of("0123456789")==std::string::npos)game.year=std::stoi(year);
  if(fields.size()==9){
   if(!fields[4].empty())game.hardware=fields[4];if(!fields[5].empty())game.region=fields[5];
   if(!fields[6].empty()&&fields[6].size()<3&&fields[6].find_first_not_of("0123456789")==std::string::npos)game.players=std::min(64,std::stoi(fields[6]));
   if(fields[7]=="good"||fields[7]=="imperfect"||fields[7]=="preliminary")game.compatibility=fields[7];
   if(fields[8]=="supported"||fields[8]=="unsupported")game.save_support=fields[8];
  }
  result.emplace(game.id,std::move(game));
 }
 return result;
}
std::vector<Game> scan(const std::filesystem::path& dir,const GameMetadata& metadata){
 if(!directory_available(dir))throw std::runtime_error("Game folder is unavailable: "+dir.string());
 std::vector<Game> games;
 // Top-level sets only: CHDs and BIOS files inside subdirectories are not games.
 for(const auto& entry:directory_entries(dir)){
  if(!entry.regular)continue;
  auto ext=lower(entry.path.extension().string());if(ext!=".zip"&&ext!=".7z")continue;
  auto id=lower(entry.path.stem().string());
  if(id=="neogeo"||id=="pgm"||id=="cps3"||id=="naomi"||id=="qsound")continue;
  Game g;auto complete=metadata.find(id),found=known.find(id);
  if(complete!=metadata.end())g=complete->second;
  else if(found!=known.end())g=found->second;
  else {g.id=id;g.title=entry.path.stem().string();g.maker="Unknown company";}
  g.path=std::filesystem::absolute(entry.path);games.push_back(g);
 }
 std::sort(games.begin(),games.end(),[](const Game&a,const Game&b){return a.title<b.title;});return games;
}
std::set<std::string> read_favorites(const std::filesystem::path& file){
 std::set<std::string> values;std::ifstream input(file);std::string line;while(std::getline(input,line))if(!line.empty())values.insert(line);return values;
}
void write_favorites(const std::filesystem::path& file,const std::set<std::string>& values){
 std::filesystem::create_directories(file.parent_path());auto temp=file;temp+=".tmp";
 {std::ofstream out(temp);if(!out)throw std::runtime_error("Cannot save favorites");for(const auto& v:values)out<<v<<'\n';out.flush();if(!out)throw std::runtime_error("Cannot save favorites");}
 std::filesystem::rename(temp,file);
}
std::vector<Game> sample_library(){std::vector<Game> g;for(auto id:{"robby","gridlee","circus","looping","targ","alienar"})g.push_back(known.at(id));return g;}
}
