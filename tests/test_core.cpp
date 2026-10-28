// Synthetic libretro core for lifecycle/video/input tests. Not an arcade emulator.
#include "libretro.h"
#include <cstdint>
#include <cstring>
static retro_environment_t env;
static retro_video_refresh_t video;
static retro_audio_sample_batch_t audio;
static retro_input_state_t input;
static bool waiting_for_video=false,unsupported_state=false,reject_state=false;
static uint32_t ticks=0,boot_updates=0;
static bool orientation_frame=false,dynamic_orientation=false;static retro_pixel_format orientation_format=RETRO_PIXEL_FORMAT_XRGB8888;
static bool neogeo_startup=false,stuck_startup=false;
extern "C" {
unsigned retro_api_version(){return RETRO_API_VERSION;}
void retro_set_environment(retro_environment_t f){env=f;}
void retro_set_video_refresh(retro_video_refresh_t f){video=f;}
void retro_set_audio_sample(retro_audio_sample_t){}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t f){audio=f;}
void retro_set_input_poll(retro_input_poll_t){}
void retro_set_input_state(retro_input_state_t f){input=f;}
void retro_init(){retro_pixel_format fmt=RETRO_PIXEL_FORMAT_RGB565;env(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,&fmt);}
void retro_deinit(){}
void retro_get_system_info(retro_system_info* info){*info={"Test engine","1.0","zip",true,false};}
void retro_get_system_av_info(retro_system_av_info* av){*av=orientation_frame?retro_system_av_info{{3,2,3,2,1.5f},{60,48000}}:retro_system_av_info{{2,1,2,1,2.f},{60,48000}};}
bool retro_load_game(const retro_game_info* game){ticks=0;boot_updates=0;
 orientation_frame=game&&game->path&&std::strstr(game->path,"orientation-");dynamic_orientation=game&&game->path&&std::strstr(game->path,"orientation-dynamic");
 if(orientation_frame){
  const char* name=std::strstr(game->path,"orientation-")+12;unsigned rotation=(*name>='0'&&*name<='3')?unsigned(*name-'0'):0;
  if(!env(RETRO_ENVIRONMENT_SET_ROTATION,&rotation))return false;
  unsigned invalid=4;if(env(RETRO_ENVIRONMENT_SET_ROTATION,&invalid)||env(RETRO_ENVIRONMENT_SET_ROTATION,nullptr))return false;
  orientation_format=std::strstr(game->path,"565")?RETRO_PIXEL_FORMAT_RGB565:std::strstr(game->path,"1555")?RETRO_PIXEL_FORMAT_0RGB1555:RETRO_PIXEL_FORMAT_XRGB8888;
  if(!env(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,&orientation_format))return false;
 }
neogeo_startup=game&&game->path&&std::strstr(game->path,"neogeo-state");stuck_startup=game&&game->path&&std::strstr(game->path,"stuck");unsupported_state=game&&game->path&&std::strstr(game->path,"unsupported.zip");reject_state=false;if(game&&game->path&&std::strstr(game->path,"oom.zip")){retro_log_callback logger{};if(env(RETRO_ENVIRONMENT_GET_LOG_INTERFACE,&logger))logger.log(RETRO_LOG_ERROR,"Caught unhandled std::bad_alloc exception\n");return false;}waiting_for_video=game&&game->path&&std::strstr(game->path,"delayed.zip");return true;}
void retro_unload_game(){}
void retro_set_controller_port_device(unsigned,unsigned){}
size_t retro_serialize_size(){return unsupported_state?0:sizeof(ticks);}
bool retro_serialize(void* data,size_t size){if(size!=sizeof(ticks)||reject_state)return false;std::memcpy(data,&ticks,size);return true;}
bool retro_unserialize(const void* data,size_t size){if(!ticks||size!=sizeof(ticks))return false;std::memcpy(&ticks,data,size);return true;}
int arcade_mame_coin_counter(unsigned){return neogeo_startup?0:-1;}
int arcade_mame_coin_lockout(unsigned){return neogeo_startup?(stuck_startup||boot_updates<4):0;}
void retro_run(){
 ++boot_updates;
 // A late firmware reset is deliberately absent from the serialized payload.
 if(neogeo_startup&&boot_updates==4)ticks=0;
 ++ticks;
 reject_state=input(0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_L)!=0;

 if(orientation_frame){
  if(dynamic_orientation&&ticks>=2){unsigned rotation=ticks==2?1:0;env(RETRO_ENVIRONMENT_SET_ROTATION,&rotation);}
  // Asymmetric image with padding: red/green/blue, yellow/magenta/cyan.
  uint32_t rgb[]={0xff0000,0x00ff00,0x0000ff,0x123456,0xffff00,0xff00ff,0x00ffff,0x123456};
  uint16_t rgb565[]={0xf800,0x07e0,0x001f,0xffff,0xffe0,0xf81f,0x07ff,0xffff};
  uint16_t rgb1555[]={0x7c00,0x03e0,0x001f,0x7fff,0x7fe0,0x7c1f,0x03ff,0x7fff};
  if(orientation_format==RETRO_PIXEL_FORMAT_XRGB8888)video(rgb,3,2,4*sizeof(uint32_t));
  else video(orientation_format==RETRO_PIXEL_FORMAT_RGB565?rgb565:rgb1555,3,2,4*sizeof(uint16_t));
  return;
 }
 if(waiting_for_video){waiting_for_video=false;video(nullptr,0,0,0);return;}
 // Include stride padding and test the frontend's second controller input.
 uint16_t row[]={static_cast<uint16_t>(0xf800|((ticks-1)&31)),static_cast<uint16_t>(input(1,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_A)?0x07e0:0x001f),0xffff};
 video(row,2,1,sizeof row);int16_t sound[]={1,-1,2,-2};audio(sound,2);
}
}
