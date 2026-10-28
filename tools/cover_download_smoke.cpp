#include "cover_art.hpp"
#include <iostream>
int main(int argc,char** argv){
 if(argc<3)return 2;
 arcade::CoverCache cache(argv[1],arcade::download_cover);std::vector<arcade::Game> games;
 for(int i=3;i<argc;++i){std::string id=argv[i];games.push_back({id,id,"",0,std::filesystem::path(argv[2])/"roms"/(id+".zip")});}
 if(games.empty())games.push_back({"dino","Cadillacs and Dinosaurs","Capcom",1993,std::filesystem::path(argv[2])/"roms/dino.zip"});
 cache.discover(games);cache.wait_idle();
 for(const auto& game:games){auto path=std::filesystem::path(argv[2])/"covers"/(game.id+".png");if(!std::filesystem::is_regular_file(path))return 1;std::cout<<path<<'\n';}
}
