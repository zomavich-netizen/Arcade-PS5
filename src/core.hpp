#pragma once
#include "libretro.h"
#include "audio_output.hpp"
#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace arcade {
// One core instance at a time: libretro callback functions have no user context.
class Core {
public:
 explicit Core(AudioOutput* output=nullptr):output_(output){} ~Core(); Core(const Core&)=delete; Core& operator=(const Core&)=delete;
 bool load(const std::filesystem::path& module,const std::filesystem::path& content,const std::filesystem::path& data);
 void stop(); void run();
 static std::filesystem::path state_path(const std::filesystem::path& content,const std::filesystem::path& data);
 bool save_state();bool load_state();
 int coin_lockout()const;
 int coin_counter()const;
 void audio_settings(int volume,bool mute){volume_=volume;mute_=mute;}
 void audio_speed(double speed);
 void log_settings(bool detail,bool immediate){detailed_logs_=detail;immediate_logs_=immediate;}
 void pause_audio(bool paused){if(output_)output_->pause(paused);}
 const std::vector<uint32_t>& pixels()const{return pixels_;}
 unsigned width()const{return width_;} unsigned height()const{return height_;}
 double fps()const{return fps_;} float aspect()const{return rotation_%2&&aspect_>0?1/aspect_:aspect_;}
 unsigned rotation()const{return rotation_;} // Counter-clockwise quarter turns requested by the core.
 const std::string& error_code()const{return error_code_;}
 const std::string& error()const{return error_;} const std::string& name()const{return name_;}
 bool playing()const{return loaded_;} bool shutdown()const{return shutdown_;}
 std::array<uint16_t,4> buttons{};
 unsigned long video_frames=0,audio_frames=0;
private:
 static Core* active_;
 void* module_=nullptr;bool initialized_=false,loaded_=false,shutdown_=false;
 unsigned long emulation_frames_=0;
 double audio_speed_=1;unsigned sample_rate_=0;
 int volume_=100;bool mute_=false,detailed_logs_=false,immediate_logs_=false;std::vector<int16_t> mixed_audio_;
 AudioOutput* output_=nullptr;
 enum retro_pixel_format format_=RETRO_PIXEL_FORMAT_0RGB1555;
 unsigned width_=0,height_=0,rotation_=0;double fps_=60;float aspect_=0;
 std::vector<uint32_t> pixels_;std::vector<char> content_data_;
 std::string error_code_,error_,name_,content_path_,system_dir_,save_dir_,content_dir_;
 std::map<std::string,std::string> variables_;
 void (*init_)()=nullptr;void (*deinit_)()=nullptr;void (*run_)()=nullptr;
 bool (*load_game_)(const retro_game_info*)=nullptr;void (*unload_game_)()=nullptr;
 size_t (*serialize_size_)()=nullptr;
 bool (*serialize_)(void*,size_t)=nullptr;bool (*unserialize_)(const void*,size_t)=nullptr;
 void (*get_av_)(retro_system_av_info*)=nullptr;
 static bool environment(unsigned,void*);
 static void video(const void*,unsigned,unsigned,size_t);
 static size_t audio(const int16_t*,size_t);static void audio_sample(int16_t,int16_t);
 static int16_t input(unsigned,unsigned,unsigned,unsigned);static void poll(){}
 static void log(enum retro_log_level,const char*,...);
};
}
