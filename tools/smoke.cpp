// Headless real-core check; no UI design is committed here.
#include "core.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=5){std::cerr<<"Usage: arcade_smoke CORE ROM DATA OUTPUT.ppm\n";return 2;}
 
 int result=0;
 {arcade::Core core;
  if(!core.load(argv[1],argv[2],argv[3])){std::cerr<<core.error()<<'\n';return 1;}
  for(int frame=0;frame<600&&!core.shutdown();++frame){
   core.buttons[0]=frame>=300&&frame<310?1<<RETRO_DEVICE_ID_JOYPAD_SELECT:frame>=360&&frame<370?1<<RETRO_DEVICE_ID_JOYPAD_START:0;
   core.run();
  }
  std::cout<<core.name()<<" frames="<<core.video_frames<<" audio="<<core.audio_frames<<" size="<<core.width()<<'x'<<core.height()<<'\n';
  if(core.pixels().empty()||!core.audio_frames)result=1;
  else {std::ofstream out(argv[4],std::ios::binary);out<<"P6\n"<<core.width()<<' '<<core.height()<<"\n255\n";for(auto px:core.pixels()){char rgb[]={char(px>>16),char(px>>8),char(px)};out.write(rgb,3);}}
 }
 return result;
}
