#pragma once
#include <array>
#include <filesystem>
#include <string>
namespace arcade {
enum class Theme { Day, Night };
enum class SettingsCategory { Profiles,Video,Performance,Audio,Controls,Accessibility,Diagnostics,Files,Appearance,Colour,Count };
constexpr int settings_category_count=static_cast<int>(SettingsCategory::Count);
struct Settings {
 Theme theme=Theme::Day;
 bool smooth=false,mute=false,overlay=false,pause_on_focus=true;
 bool vibration=true,large_text=false,high_contrast=false,reduce_motion=false;
 bool detailed_logging=false,immediate_logs=false,swap_menu=false;
 bool hud_colors=true,hud_backend=true,hud_fps=true,hud_ms=true,hud_speed=true,hud_resolution=true;
 bool menu_music=true;
 int renderer=0;
 int profile=0,output=0,refresh=0,scaling=0,rotation=0,post_processing=0,speed=2;
 int volume=100,menu_volume=30,vibration_strength=50,coin_button=0,start_button=0;
 int accent=0,hud_size=0,hud_corner=0;
 std::array<int,4> action_buttons{{0,1,2,3}};
 std::string wallpaper;
 std::filesystem::path roms, core;
};
void validate_settings(Settings&);
std::filesystem::path ps5_rom_directory(const std::filesystem::path& previous);
Settings read_settings(const std::filesystem::path& file);
void write_settings(const std::filesystem::path& file,const Settings& settings);
std::filesystem::path profile_directory(const std::filesystem::path&,int profile);
unsigned accent_color(int accent);
double speed_multiplier(int speed);
}
