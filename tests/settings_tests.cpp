#include "settings_menu.hpp"
#include "control_mapping.hpp"
#include <cassert>
#include <fstream>
#include <unistd.h>
using namespace arcade;
int main(){
 for(const char* old:{"","/data/arcade-roms","/data/arcade-roms/","/app0/roms","/app0/roms/","/data/homebrew/PPSA99984/roms/","/data/arcade/roms","/data/arcade/roms/"})assert(ps5_rom_directory(old)=="/data/arcade/roms");
 assert(ps5_rom_directory("/data/my-games")=="/data/my-games");
 auto root=std::filesystem::temp_directory_path()/("arcade-settings-"+std::to_string(getpid()));auto file=root/"settings.cfg";
 Settings s;s.theme=Theme::Night;s.roms="/data/arcade-roms";s.profile=2;s.output=2;s.refresh=1;s.volume=60;s.mute=true;s.menu_volume=20;s.menu_music=false;s.vibration=false;s.vibration_strength=75;s.large_text=true;s.high_contrast=true;s.reduce_motion=true;s.overlay=true;s.hud_corner=3;s.hud_size=2;s.hud_fps=false;s.hud_backend=false;s.post_processing=2;s.rotation=3;s.scaling=1;s.speed=4;s.action_buttons={{4,5,6,7}};s.coin_button=3;s.start_button=2;s.swap_menu=true;s.accent=5;s.detailed_logging=true;s.immediate_logs=true;s.wallpaper="wall.png";
 write_settings(file,s);auto loaded=read_settings(file);assert(!loaded.menu_music);auto other=root/"roundtrip.cfg";write_settings(other,loaded);
 auto contents=[](auto path){std::ifstream in(path);return std::string(std::istreambuf_iterator<char>(in),{});};assert(contents(file)==contents(other));
 assert(profile_directory(root,0)==root);assert(profile_directory(root,2)==root/"profiles/player-3");
 {std::ofstream out(file);out<<"volume \"999\"\nprofile \"-9\"\noutput \"bad\"\naction_0 \"999\"\nwallpaper \"../bad.png\"\n";}loaded=read_settings(file);assert(loaded.volume==100&&loaded.profile==0&&loaded.output==0&&loaded.action_buttons[0]==7&&loaded.wallpaper.empty());
 for(int c=0;c<settings_category_count;++c){auto rows=category_rows(static_cast<SettingsCategory>(c));assert(!rows.empty());for(auto row:rows){(void)setting_value(row.id,s);Settings v=s;if(adjust_setting(row.id,v,1)){validate_settings(v);assert(!setting_value(row.id,v).empty());}}}
 Settings v;v.volume=0;adjust_setting(SettingId::Volume,v,-1);assert(v.volume==0);v.profile=0;adjust_setting(SettingId::Profile,v,-1);assert(v.profile==3);v.action_buttons[0]=7;adjust_setting(SettingId::ActionCross,v,1);assert(v.action_buttons[0]==0);
 Settings controls;unsigned raw=(1u<<RETRO_DEVICE_ID_JOYPAD_L3)|(1u<<RETRO_DEVICE_ID_JOYPAD_R3)|(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT)|(1u<<RETRO_DEVICE_ID_JOYPAD_B);auto mapped=map_controls(raw,controls);assert(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT));assert(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_START));assert(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT));assert(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_B));
 controls.action_buttons[0]=5;controls.coin_button=2;controls.start_button=1;mapped=map_controls(raw,controls);assert(!(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_B)));assert(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_R));assert(!(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)));assert(!(mapped&(1u<<RETRO_DEVICE_ID_JOYPAD_START)));assert(map_controls(0,controls)==0);
 assert(overlay_rows().size()==8&&mapping_rows().size()==5);assert(speed_multiplier(2)==1&&speed_multiplier(0)==.5);
 std::filesystem::remove_all(root);
}
