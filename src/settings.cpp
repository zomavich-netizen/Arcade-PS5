#include "settings.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
namespace arcade {
std::filesystem::path ps5_rom_directory(const std::filesystem::path& previous){
 auto path=previous.lexically_normal();
 if(path.filename().empty())path=path.parent_path();
 if(path.empty()||path=="/data/arcade-roms"||path=="/app0/roms"||path=="/data/homebrew/PPSA99984/roms")return "/data/arcade/roms";
 return path;
}
namespace {
struct IntField {const char* key;int Settings::*member;int low,high;};
const IntField ints[]={
 {"renderer",&Settings::renderer,0,1},{"profile",&Settings::profile,0,3},{"output",&Settings::output,0,2},{"refresh",&Settings::refresh,0,1},
 {"scaling",&Settings::scaling,0,2},{"rotation",&Settings::rotation,0,3},{"post_processing",&Settings::post_processing,0,5},
 {"speed",&Settings::speed,0,7},{"volume",&Settings::volume,0,100},{"menu_volume",&Settings::menu_volume,0,100},
 {"vibration_strength",&Settings::vibration_strength,25,100},{"coin_button",&Settings::coin_button,0,3},
 {"start_button",&Settings::start_button,0,2},{"accent",&Settings::accent,0,7},
 {"hud_size",&Settings::hud_size,0,2},{"hud_corner",&Settings::hud_corner,0,3}};
struct BoolField {const char* key;bool Settings::*member;};
const BoolField bools[]={
 {"menu_music",&Settings::menu_music},{"smooth",&Settings::smooth},{"mute",&Settings::mute},{"overlay",&Settings::overlay},
 {"pause_on_focus",&Settings::pause_on_focus},{"vibration",&Settings::vibration},
 {"large_text",&Settings::large_text},{"high_contrast",&Settings::high_contrast},{"reduce_motion",&Settings::reduce_motion},
 {"detailed_logging",&Settings::detailed_logging},{"immediate_logs",&Settings::immediate_logs},{"swap_menu",&Settings::swap_menu},
 {"hud_colors",&Settings::hud_colors},{"hud_backend",&Settings::hud_backend},{"hud_fps",&Settings::hud_fps},
 {"hud_ms",&Settings::hud_ms},{"hud_speed",&Settings::hud_speed},{"hud_resolution",&Settings::hud_resolution}};
}
void validate_settings(Settings& s){
 for(auto f:ints)s.*(f.member)=std::clamp(s.*(f.member),f.low,f.high);
 if(s.renderer==0)s.post_processing=std::min(s.post_processing,2);
 for(auto& button:s.action_buttons)button=std::clamp(button,0,7);
 // Only a file name is accepted. Wallpapers stay inside the app's wallpapers folder.
 if(s.wallpaper=="."||s.wallpaper==".."||s.wallpaper.find('/')!=std::string::npos||s.wallpaper.find('\\')!=std::string::npos)s.wallpaper.clear();
}
Settings read_settings(const std::filesystem::path& file){
 Settings s;std::ifstream in(file);std::string key,value;
 while(in>>key>>std::quoted(value)){
  if(key=="theme")s.theme=value=="night"?Theme::Night:Theme::Day;
  else if(key=="roms")s.roms=value;else if(key=="core")s.core=value;else if(key=="wallpaper")s.wallpaper=value;
  else {
   for(auto f:bools)if(key==f.key)s.*(f.member)=value=="yes";
   for(auto f:ints)if(key==f.key){try{std::size_t used=0;int n=std::stoi(value,&used);if(used==value.size())s.*(f.member)=n;}catch(const std::exception&) {}}
   for(int i=0;i<4;++i)if(key=="action_"+std::to_string(i)){try{s.action_buttons[i]=std::stoi(value);}catch(const std::exception&) {}}
  }
 }
 validate_settings(s);return s;
}
void write_settings(const std::filesystem::path& file,const Settings& settings){
 Settings s=settings;validate_settings(s);
 if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path());
 auto temp=file;temp+=".tmp";
 {std::ofstream out(temp);out<<"theme "<<std::quoted(s.theme==Theme::Night?"night":"day")<<"\nroms "<<std::quoted(s.roms.string())<<"\ncore "<<std::quoted(s.core.string())<<"\nwallpaper "<<std::quoted(s.wallpaper)<<'\n';
 for(auto f:bools)out<<f.key<<' '<<std::quoted(s.*(f.member)?"yes":"no")<<'\n';
 for(auto f:ints)out<<f.key<<' '<<std::quoted(std::to_string(s.*(f.member)))<<'\n';
 for(int i=0;i<4;++i)out<<"action_"<<i<<' '<<std::quoted(std::to_string(s.action_buttons[i]))<<'\n';
 out.flush();if(!out)throw std::runtime_error("Could not save settings. Check the data folder.");}
 std::filesystem::rename(temp,file);
}
std::filesystem::path profile_directory(const std::filesystem::path& root,int profile){return profile<=0?root:root/"profiles"/("player-"+std::to_string(std::clamp(profile,1,3)+1));}
unsigned accent_color(int accent){constexpr unsigned colors[]={0xf35419,0xd59400,0xdf6550,0x16856b,0x087f98,0x366ae0,0x9562cb,0xc44777};return colors[std::clamp(accent,0,7)];}
double speed_multiplier(int speed){constexpr double values[]={.5,.75,1,1.25,1.5,2,3,4};return values[std::clamp(speed,0,7)];}
}
