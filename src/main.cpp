#include "app.hpp"
#include <iostream>
#include <stdexcept>
#ifndef ARCADE_ROOT
#define ARCADE_ROOT "."
#endif
int main(int argc,char** argv){
 try {
  const std::filesystem::path root=ARCADE_ROOT;
  arcade::Options options;options.assets=root/"assets";options.data=root/".local/user";
  for(int i=1;i<argc;++i){std::string arg=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+arg);return std::string(argv[++i]);};
   if(arg=="--demo")options.demo=true;
   else if(arg=="--theme"){auto theme=value();if(theme!="day"&&theme!="night")throw std::runtime_error("Theme must be day or night");options.night=theme=="night";options.day=theme=="day";}
   else if(arg=="--roms")options.roms=value();else if(arg=="--core")options.core=value();else if(arg=="--data")options.data=value();
   else if(arg=="--capture")options.capture=value();else if(arg=="--ui-smoke")options.ui_smoke=true;
   else if(arg=="--frames")options.run_frames=static_cast<unsigned>(std::stoul(value()));
   else if(arg=="--help"){std::cout<<"Arcade Gallery\n  --demo                Preview six illustrative covers (no games included)\n  --theme day|night     Start in the chosen theme\n  --roms FOLDER         Game archive folder\n  --core FILE           Compatible MAME libretro core\n  --data FOLDER         Settings and save data folder\n  --capture FILE.bmp    Capture the actual launcher screen and exit\n  --frames COUNT        Stop after a bounded number of frames\n";return 0;}
   else throw std::runtime_error("Unknown argument: "+arg);
  }
  // Developer defaults only when no installation has been configured.
  auto saved=arcade::read_settings(options.data/"settings.cfg");
  if(options.roms.empty()&&saved.roms.empty()&&std::filesystem::is_directory(root/".local/roms"))options.roms=root/".local/roms";
  if(options.core.empty()&&saved.core.empty()&&std::filesystem::exists(root/".local/cores/mame_libretro.dylib"))options.core=root/".local/cores/mame_libretro.dylib";
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)throw std::runtime_error(SDL_GetError());
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
  auto* w=SDL_CreateWindow("Arcade - Gallery",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1440,810,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
  if(!w)throw std::runtime_error(SDL_GetError());
  auto* r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);if(!r)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
  if(!r){SDL_DestroyWindow(w);throw std::runtime_error(SDL_GetError());}
  SDL_RenderSetLogicalSize(r,1600,900);SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
  int result=0;
  try{arcade::App app(w,r,options);result=app.run();}catch(...){SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();throw;}
  SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return result;
 }catch(const std::exception& e){std::cerr<<"Arcade: "<<e.what()<<'\n';return 1;}
}
