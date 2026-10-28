#include "core.hpp"
#ifndef ARCADE_STATIC_MAME
#include <dlfcn.h>
#endif
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <new>
#include <cmath>
#ifdef ARCADE_PS5
extern "C" void arcade_startup_note(const char*);
extern "C" void ps5_opengl_heap_stats_print(unsigned);
extern "C" void ps5_opengl_heap_snapshot(const char*,unsigned);
#endif
#ifdef ARCADE_STATIC_MAME
extern "C" int arcade_mame_coin_lockout(unsigned);
extern "C" int arcade_mame_coin_counter(unsigned);
#endif
namespace arcade {
Core* Core::active_=nullptr;
namespace {
#ifdef ARCADE_STATIC_MAME
void* static_symbol(const char* name){
 #define CORE_SYMBOL(function) if(std::strcmp(name,#function)==0)return reinterpret_cast<void*>(&function)
 CORE_SYMBOL(retro_serialize_size);CORE_SYMBOL(retro_serialize);CORE_SYMBOL(retro_unserialize);
 CORE_SYMBOL(retro_api_version);CORE_SYMBOL(retro_init);CORE_SYMBOL(retro_deinit);CORE_SYMBOL(retro_run);CORE_SYMBOL(retro_load_game);CORE_SYMBOL(retro_unload_game);CORE_SYMBOL(retro_get_system_av_info);CORE_SYMBOL(retro_set_environment);CORE_SYMBOL(retro_set_video_refresh);CORE_SYMBOL(retro_set_audio_sample);CORE_SYMBOL(retro_set_audio_sample_batch);CORE_SYMBOL(retro_set_input_poll);CORE_SYMBOL(retro_set_input_state);CORE_SYMBOL(retro_get_system_info);CORE_SYMBOL(retro_set_controller_port_device);
 #undef CORE_SYMBOL
 return nullptr;
}
#endif
template<class T>T symbol(void* module,const char* name){
#ifdef ARCADE_STATIC_MAME
 (void)module;auto p=static_symbol(name);
#else
 auto p=dlsym(module,name);
#endif
 if(!p)throw std::runtime_error(std::string("Core is missing ")+name);return reinterpret_cast<T>(p);
}
}
Core::~Core(){stop();}
bool Core::load(const std::filesystem::path& module,const std::filesystem::path& content,const std::filesystem::path& data){
 stop();error_.clear();error_code_="ARC-1004";variables_.clear();format_=RETRO_PIXEL_FORMAT_0RGB1555;shutdown_=false;fps_=60;aspect_=0;video_frames=audio_frames=emulation_frames_=0;
 if(active_&&active_!=this){error_="Another game is already running";return false;}
 try {
  if(!std::filesystem::is_regular_file(content)){error_code_="ARC-1002";throw std::runtime_error("Game archive is missing");}
  system_dir_=std::filesystem::absolute(content.parent_path()).string();content_dir_=system_dir_;
  save_dir_=std::filesystem::absolute(data/"saves").string();std::filesystem::create_directories(save_dir_);
#ifdef ARCADE_STATIC_MAME
  (void)module;module_=this;
#else
  module_=dlopen(std::filesystem::absolute(module).c_str(),RTLD_NOW|RTLD_LOCAL);
  if(!module_){error_code_="ARC-1003";throw std::runtime_error(std::string("Cannot open arcade engine: ")+dlerror());}
#endif
  active_=this;error_code_="ARC-1003";
  auto version=symbol<unsigned(*)()>(module_,"retro_api_version");if(version()!=RETRO_API_VERSION)throw std::runtime_error("Unsupported core API version");
  init_=symbol<void(*)()>(module_,"retro_init");deinit_=symbol<void(*)()>(module_,"retro_deinit");run_=symbol<void(*)()>(module_,"retro_run");
  load_game_=symbol<bool(*)(const retro_game_info*)>(module_,"retro_load_game");unload_game_=symbol<void(*)()>(module_,"retro_unload_game");get_av_=symbol<void(*)(retro_system_av_info*)>(module_,"retro_get_system_av_info");
  serialize_size_=symbol<size_t(*)()>(module_,"retro_serialize_size");
  serialize_=symbol<bool(*)(void*,size_t)>(module_,"retro_serialize");unserialize_=symbol<bool(*)(const void*,size_t)>(module_,"retro_unserialize");
  symbol<void(*)(retro_environment_t)>(module_,"retro_set_environment")(environment);
  symbol<void(*)(retro_video_refresh_t)>(module_,"retro_set_video_refresh")(video);
  symbol<void(*)(retro_audio_sample_t)>(module_,"retro_set_audio_sample")(audio_sample);
  symbol<void(*)(retro_audio_sample_batch_t)>(module_,"retro_set_audio_sample_batch")(audio);
  symbol<void(*)(retro_input_poll_t)>(module_,"retro_set_input_poll")(poll);
  symbol<void(*)(retro_input_state_t)>(module_,"retro_set_input_state")(input);
  retro_system_info info{};symbol<void(*)(retro_system_info*)>(module_,"retro_get_system_info")(&info);
  name_=std::string(info.library_name?info.library_name:"Arcade engine")+" "+(info.library_version?info.library_version:"");
  initialized_=true;init_();
  content_path_=std::filesystem::absolute(content).string();
  if(!info.need_fullpath){std::ifstream f(content,std::ios::binary);content_data_=std::vector<char>(std::istreambuf_iterator<char>(f),{});if(!f.eof()&&f.fail())throw std::runtime_error("Cannot read game archive");}
  retro_game_info game{content_path_.c_str(),content_data_.empty()?nullptr:content_data_.data(),content_data_.size(),nullptr};
#ifdef ARCADE_PS5
  ps5_opengl_heap_stats_print(0);ps5_opengl_heap_snapshot("before-game-load",0);
#endif
  error_code_="ARC-1004";
  if(!load_game_(&game))throw std::runtime_error(error_.empty()?"Game could not start. Check its ROM set and required BIOS files; see the diagnostic log.":error_);
#ifdef ARCADE_PS5
  ps5_opengl_heap_snapshot("after-game-load",0);
#endif
  loaded_=true;
  auto set_port=symbol<void(*)(unsigned,unsigned)>(module_,"retro_set_controller_port_device");for(unsigned p=0;p<4;++p)set_port(p,RETRO_DEVICE_JOYPAD);
  retro_system_av_info av{};get_av_(&av);fps_=av.timing.fps>1?av.timing.fps:60;aspect_=av.geometry.aspect_ratio;
  if(output_&&av.timing.sample_rate>=8000&&av.timing.sample_rate<=192000){sample_rate_=static_cast<unsigned>(av.timing.sample_rate);output_->configure(static_cast<unsigned>(sample_rate_*audio_speed_));}
  return true;
 }catch(const std::bad_alloc&){error_code_="ARC-1001";error_="There is not enough memory to load this game. Close Arcade completely, reopen it, and try again. If the error continues, this game may require more memory than this build can provide.";
#ifdef ARCADE_PS5
  ps5_opengl_heap_snapshot("game-load-out-of-memory",0);
#endif
  stop();return false;
 }catch(const std::exception& e){error_=e.what();
#ifdef ARCADE_PS5
  ps5_opengl_heap_snapshot("game-load-failed",0);
#endif
  stop();return false;}
}
void Core::stop(){
 if(loaded_&&unload_game_)unload_game_();loaded_=false;
 if(initialized_&&deinit_)deinit_();initialized_=false;
 if(output_)output_->stop();
 if(module_){
#ifndef ARCADE_STATIC_MAME
  dlclose(module_);
#endif
  module_=nullptr;
 }
 if(active_==this)active_=nullptr;
 buttons.fill(0);pixels_.clear();content_data_.clear();width_=height_=rotation_=0;aspect_=0;
}
namespace {
constexpr size_t max_state_size=256u*1024u*1024u;
uint64_t state_checksum(const std::vector<char>& bytes){uint64_t hash=14695981039346656037ull;for(unsigned char byte:bytes){hash^=byte;hash*=1099511628211ull;}return hash;}
}
std::filesystem::path Core::state_path(const std::filesystem::path& content,const std::filesystem::path& data){return data/"saves"/"states"/(content.filename().string()+".state");}
bool Core::save_state(){
 error_code_="ARC-2001";error_.clear();std::filesystem::path temporary;
 try{
  if(!loaded_)throw std::runtime_error("No game is running.");
  auto size=serialize_size_();if(!size)throw std::runtime_error("This game does not support saving its state.");
  if(size>max_state_size)throw std::runtime_error("This game's save state exceeds the 256 MiB limit.");
  std::vector<char> bytes(size);if(!serialize_(bytes.data(),size))throw std::runtime_error("The game could not save its state. Resume and try again.");
  auto destination=state_path(content_path_,std::filesystem::path(save_dir_).parent_path());
  std::filesystem::create_directories(destination.parent_path());temporary=destination;temporary+=".partial";
  std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
  auto checksum=state_checksum(bytes);
  out<<"ARCADE-STATE-1\n"<<name_<<'\n'<<std::filesystem::path(content_path_).filename().string()<<'\n';
  out.write(reinterpret_cast<const char*>(&checksum),sizeof checksum);out.write(bytes.data(),bytes.size());out.flush();
  if(!out)throw std::runtime_error("Cannot write the save state. Check the save location and available space.");
  out.close();if(!out)throw std::runtime_error("Cannot finish writing the save state.");
  std::filesystem::rename(temporary,destination);return true;
 }catch(const std::exception& e){error_=e.what();if(!temporary.empty()){std::error_code ec;std::filesystem::remove(temporary,ec);}return false;}
}
bool Core::load_state(){
 error_code_="ARC-2002";error_.clear();
 try{
  if(!loaded_)throw std::runtime_error("No game is running.");
  std::ifstream in(state_path(content_path_,std::filesystem::path(save_dir_).parent_path()),std::ios::binary);
  if(!in)throw std::runtime_error("The last save is unavailable.");
  // Compare a bounded header before allocating or passing anything to the engine.
  auto header=std::string("ARCADE-STATE-1\n")+name_+"\n"+std::filesystem::path(content_path_).filename().string()+"\n";
  std::string actual(header.size(),'\0');in.read(actual.data(),actual.size());
  if(!in||actual!=header)throw std::runtime_error("The last save belongs to a different game or engine version.");
  // MAME performs its initial reset on the first run. Restoring before that
  // succeeds but the reset then overwrites the restored machine state.
  if(!emulation_frames_){buttons.fill(0);run();}
  auto size=serialize_size_();if(!size||size>max_state_size)throw std::runtime_error("This game cannot load a save state.");
  uint64_t checksum=0;in.read(reinterpret_cast<char*>(&checksum),sizeof checksum);
  auto start=in.tellg();in.seekg(0,std::ios::end);auto end=in.tellg();
  if(!in||start<0||end-start!=static_cast<std::streamoff>(size))throw std::runtime_error("The last save is incomplete or incompatible.");
  in.seekg(start);std::vector<char> bytes(size);in.read(bytes.data(),bytes.size());
  if(!in||state_checksum(bytes)!=checksum)throw std::runtime_error("The last save is damaged. You can start from the beginning.");
  // Neo Geo firmware can still be resetting and setting up its cartridge after
  // the first update. Use the same boot grace and coin-lockout readiness as
  // queued credits, without sending a coin/start or exposing the boot frames.
  const auto minimum=static_cast<unsigned long>(std::ceil(fps_*6));
  if(coin_counter()>=0&&emulation_frames_<minimum){
   const auto limit=static_cast<unsigned long>(std::ceil(fps_*15));
   const auto settle=static_cast<unsigned long>(std::ceil(fps_*.25));
   unsigned long unlocked=0;buttons.fill(0);
   while(!shutdown_&&emulation_frames_<limit&&(emulation_frames_<minimum||unlocked<settle)){
    run();if(coin_lockout()==0&&width_&&height_)++unlocked;else unlocked=0;
   }
   if(shutdown_||unlocked<settle)throw std::runtime_error("Neo Geo startup did not finish. Your save is unchanged; try loading again.");
   log(RETRO_LOG_INFO,"[state] Neo Geo startup ready after %lu updates; restoring %s\n",emulation_frames_,std::filesystem::path(content_path_).filename().c_str());
  }
  if(!unserialize_(bytes.data(),bytes.size()))throw std::runtime_error("The engine could not load the last save. You can start from the beginning.");
  buttons.fill(0);log(RETRO_LOG_INFO,"[state] Restored %s (%zu bytes)\n",std::filesystem::path(content_path_).filename().c_str(),bytes.size());return true;
 }catch(const std::exception& e){error_=e.what();return false;}
}
void Core::audio_speed(double speed){if(audio_speed_==speed)return;audio_speed_=speed;if(loaded_&&output_&&sample_rate_)output_->configure(static_cast<unsigned>(sample_rate_*speed));}
int Core::coin_lockout()const{
 if(!loaded_)return -1;
#ifdef ARCADE_STATIC_MAME
 return arcade_mame_coin_lockout(0);
#else
 auto query=reinterpret_cast<int(*)(unsigned)>(dlsym(module_,"arcade_mame_coin_lockout"));return query?query(0):-1;
#endif
}
int Core::coin_counter()const{
 if(!loaded_)return -1;
#ifdef ARCADE_STATIC_MAME
 return arcade_mame_coin_counter(0);
#else
 auto query=reinterpret_cast<int(*)(unsigned)>(dlsym(module_,"arcade_mame_coin_counter"));return query?query(0):-1;
#endif
}
void Core::run(){if(loaded_){run_();++emulation_frames_;}}
void Core::log(enum retro_log_level level,const char* fmt,...){auto* c=active_;if(level==RETRO_LOG_DEBUG&&(!c||!c->detailed_logs_))return;va_list args;va_start(args,fmt);
 char line[2048];vsnprintf(line,sizeof line,fmt,args);
 if(c&&(std::strstr(line,"bad_alloc")||std::strstr(line,"Out of memory")||std::strstr(line,"out of memory"))){c->error_code_="ARC-1001";c->error_="There is not enough memory to load this game. Close Arcade completely, reopen it, and try again. If the error continues, this game may require more memory than this build can provide.";}
#ifdef ARCADE_PS5
 arcade_startup_note(line);
#else
 fputs(line,stderr);if(c&&c->immediate_logs_)fflush(stderr);
#endif
 va_end(args);
}
bool Core::environment(unsigned cmd,void* data){
 auto* c=active_;if(!c)return false;
 switch(cmd){
 case RETRO_ENVIRONMENT_SET_ROTATION:{
  if(!data||*static_cast<const unsigned*>(data)>3)return false;
  c->rotation_=*static_cast<const unsigned*>(data);
  std::fprintf(stderr,"[video] Automatic rotation: %u degrees counter-clockwise\n",c->rotation_*90);
#ifdef ARCADE_PS5
  char note[96];std::snprintf(note,sizeof note,"[video] Automatic rotation: %u degrees counter-clockwise",c->rotation_*90);arcade_startup_note(note);
#endif
  return true;
 }
 case RETRO_ENVIRONMENT_GET_CAN_DUPE:*static_cast<bool*>(data)=true;return true;
 case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:*static_cast<const char**>(data)=c->system_dir_.c_str();return true;
 case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:*static_cast<const char**>(data)=c->save_dir_.c_str();return true;
 case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:*static_cast<const char**>(data)=c->content_dir_.c_str();return true;
 case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:{auto f=*static_cast<retro_pixel_format*>(data);if(f>RETRO_PIXEL_FORMAT_RGB565)return false;c->format_=f;return true;}
 case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:static_cast<retro_log_callback*>(data)->log=log;return true;
 case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:*static_cast<unsigned*>(data)=0;return true;
 case RETRO_ENVIRONMENT_SET_VARIABLES:{auto* v=static_cast<retro_variable*>(data);for(;v&&v->key;++v){std::string values=v->value?v->value:"";auto pos=values.find(';');if(pos==std::string::npos)continue;values=values.substr(pos+1);while(!values.empty()&&values[0]==' ')values.erase(0,1);c->variables_[v->key]=values.substr(0,values.find('|'));}return true;}
 case RETRO_ENVIRONMENT_GET_VARIABLE:{auto* v=static_cast<retro_variable*>(data);auto it=c->variables_.find(v->key?v->key:"");v->value=it==c->variables_.end()?nullptr:it->second.c_str();return v->value!=nullptr;}
 case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:*static_cast<bool*>(data)=false;return true;
 case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:return true;
 case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
 case RETRO_ENVIRONMENT_SET_MESSAGE:{auto* m=static_cast<retro_message*>(data);if(m&&m->msg)fprintf(stderr,"[core] %s\n",m->msg);return true;}
 case RETRO_ENVIRONMENT_SHUTDOWN:c->shutdown_=true;return true;
 case RETRO_ENVIRONMENT_SET_GEOMETRY:{auto* g=static_cast<retro_game_geometry*>(data);c->aspect_=g->aspect_ratio;return true;}
 default:return false;
 }
}
void Core::video(const void* data,unsigned w,unsigned h,size_t pitch){
 auto* c=active_;if(!c||!data||data==RETRO_HW_FRAME_BUFFER_VALID||!w||!h||w>8192||h>8192)return;
 const size_t bpp=c->format_==RETRO_PIXEL_FORMAT_XRGB8888?4:2;if(pitch<w*bpp)return;
 c->width_=c->rotation_%2?h:w;c->height_=c->rotation_%2?w:h;c->pixels_.resize(size_t(w)*h);++c->video_frames;
 for(unsigned y=0;y<h;++y){auto row=static_cast<const unsigned char*>(data)+y*pitch;
  // Fold orientation into the existing pixel conversion pass. Every consumer
  // receives the same upright frame, without an extra image or rotation pass.
  std::ptrdiff_t destination=size_t(y)*w,step=1;
  switch(c->rotation_){
   case 1:destination=size_t(w-1)*h+y;step=-std::ptrdiff_t(h);break;
   case 2:destination=size_t(h-1-y)*w+w-1;step=-1;break;
   case 3:destination=h-1-y;step=h;break;
  }
  for(unsigned x=0;x<w;++x,destination+=step){uint32_t out=0;
  if(bpp==4){std::memcpy(&out,row+x*4,4);out|=0xff000000;}
  else {uint16_t v;std::memcpy(&v,row+x*2,2);unsigned r,g,b;if(c->format_==RETRO_PIXEL_FORMAT_RGB565){r=(v>>11)&31;g=(v>>5)&63;b=v&31;g=(g<<2)|(g>>4);}else {r=(v>>10)&31;g=(v>>5)&31;b=v&31;g=(g<<3)|(g>>2);}r=(r<<3)|(r>>2);b=(b<<3)|(b>>2);out=0xff000000|(r<<16)|(g<<8)|b;}
  c->pixels_[size_t(destination)]=out;
 }}
}
size_t Core::audio(const int16_t* samples,size_t frames){auto* c=active_;if(c){c->audio_frames+=frames;if(c->output_){if(c->mute_||c->volume_!=100){c->mixed_audio_.resize(frames*2);for(size_t i=0;i<frames*2;++i)c->mixed_audio_[i]=c->mute_?0:static_cast<int16_t>(int(samples[i])*c->volume_/100);c->output_->enqueue(c->mixed_audio_.data(),frames);}else c->output_->enqueue(samples,frames);}}return frames;}
void Core::audio_sample(int16_t l,int16_t r){int16_t samples[]={l,r};audio(samples,1);}
int16_t Core::input(unsigned port,unsigned device,unsigned,unsigned id){auto* c=active_;if(!c||port>=4||(device&RETRO_DEVICE_MASK)!=RETRO_DEVICE_JOYPAD)return 0;if(id==RETRO_DEVICE_ID_JOYPAD_MASK)return static_cast<int16_t>(c->buttons[port]);return id<16?((c->buttons[port]>>id)&1):0;}
}
