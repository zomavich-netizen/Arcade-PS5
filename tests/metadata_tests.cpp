#include "library.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace arcade;
int main(int argc,char** argv){
 assert(argc==2);auto metadata=read_game_metadata(argv[1]);assert(metadata.size()==36957);
 assert(metadata.at("kof98").maker=="SNK"&&metadata.at("kof98").year==1998);
 assert(metadata.at("kof98um").maker=="SNK Playmore"&&metadata.at("kof98um").year==2008);
 assert(metadata.at("ffight").maker=="Capcom"&&metadata.at("ffight").year==1989);
 assert(metadata.at("tekken3").maker=="Namco"&&metadata.at("tekken3").year==1997);
 assert(metadata.at("kof98k").title.find("Korean")!=std::string::npos);
 const auto& dino=metadata.at("dino");assert(dino.hardware=="Capcom CPS1"&&dino.region=="World"&&dino.players==3&&dino.compatibility=="good"&&dino.save_support=="supported");
 const auto& metal=metadata.at("mslug3");assert(metal.hardware=="Neo Geo"&&metal.region=="Multi"&&metal.players==2);
 const auto& ultimate=metadata.at("kof98um");assert(ultimate.compatibility=="preliminary"&&ultimate.save_support=="unsupported"&&ultimate.players==0);
 assert(metadata.at("tekken3").compatibility=="imperfect");
 auto root=std::filesystem::temp_directory_path()/("arcade-metadata-"+std::to_string(getpid()));
 std::filesystem::create_directories(root);
 for(auto name:{"KOF98K.ZIP","tekken3.zip","unknown-game.zip","neogeo.zip"})std::ofstream(root/name)<<"fixture";
 auto games=scan(root,metadata);assert(games.size()==3);
 for(auto& game:games){
  if(game.id=="kof98k"){assert(game.maker=="SNK"&&game.year==1998);assert(game.title==metadata.at("kof98k").title);assert(game.path.filename()=="KOF98K.ZIP");}
  else if(game.id=="tekken3"){assert(game.maker=="Namco"&&game.year==1997);assert(game.title.find("Tekken 3")==0);}
  else{assert(game.id=="unknown-game"&&game.maker=="Unknown company"&&game.year==0);}
 }
 auto file=root/"metadata.tsv";
 std::ofstream(file)<<"invalid\n../bad\tBad title\tMaker\t1990\nmissing\t\tMaker\t1990\nextra\tTitle\tMaker\t1990\tbad\nuncertain\tUncertain date\tMaker\t198?\nvalid\tFull title\t\t2001\r\n";
 auto fixture=read_game_metadata(file);assert(fixture.size()==2);
 assert(fixture.at("uncertain").year==0&&fixture.at("uncertain").players==0&&fixture.at("uncertain").save_support=="unknown");assert(fixture.at("valid").maker=="Unknown company"&&fixture.at("valid").year==2001);
 assert(read_game_metadata(root/"missing.tsv").empty());
 std::ofstream(root/"robby.zip")<<"fixture";auto fallback=scan(root);
 for(auto& game:fallback)if(game.id=="robby")assert(game.maker=="Bally / Midway"&&game.year==1981);
 std::filesystem::remove_all(root);std::cout<<"Full catalogue, clone IDs, case-insensitive ROM matching, unknowns, malformed rows and fallback passed\n";
}
