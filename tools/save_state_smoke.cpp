// Optional real-engine save/load check. Uses an isolated data directory.
#include "core.hpp"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=4){std::cerr<<"Usage: save_state_smoke CORE ROM ISOLATED_DATA_DIRECTORY\n";return 2;}
 arcade::Core core;
 auto load=[&](){if(!core.load(argv[1],argv[2],argv[3])){std::cerr<<core.error()<<'\n';return false;}return true;};
 if(!load())return 1;
 for(int i=0;i<600;++i)core.run();
 if(!core.save_state()){std::cerr<<core.error()<<'\n';return 1;}
 for(int i=0;i<30;++i)core.run();auto expected=core.pixels();
 core.stop();if(!load()||!core.load_state()){std::cerr<<core.error()<<'\n';return 1;}
 for(int i=0;i<30;++i)core.run();
 if(core.pixels()!=expected){std::cerr<<"Restored frame differs\n";return 1;}
 std::cout<<"Real MAME: saved, fully unloaded, reloaded and restored; matching frame after 30 further frames\n";
}
