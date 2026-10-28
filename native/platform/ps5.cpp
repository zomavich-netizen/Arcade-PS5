// Native PS5 platform: ProsperoEden's EGL display and input, plus sceAudioOut.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platform.hpp"
#include "pe/platform/display_egl.hpp"
#include "radio_input.h"
#include "libretro.h"
#include <atomic>
#include <cstring>
#include <cstdio>
extern "C" void arcade_startup_note(const char*);
#include <pthread.h>
#include <vector>
extern "C" {
 int sceSystemServiceHideSplashScreen();
 int sceAudioOutInit();
 int sceAudioOutOpen(int,int,int,unsigned,unsigned,unsigned);
 int sceAudioOutOutput(int,const void*);
 int sceAudioOutClose(int);
 int sceKernelUsleep(unsigned);
}
namespace arcade::native {
namespace {
class Ps5Platform final:public Platform {
 int output_=0,refresh_=0;
 pe::ps5::Display display_;bool input_=false,first_=true;unsigned last_touch_=0,last_extra_=0;
public:
 ~Ps5Platform()override{if(input_)radio_input_shutdown();}
 void configure(const Settings& s)override{output_=s.output;refresh_=s.refresh;}
 void suspend_display()override{display_.close();}
 bool resume_display(std::string& error)override{const int widths[]={1920,2560,3840},heights[]={1080,1440,2160};if(!display_.open(widths[output_],heights[output_],refresh_?120:60)){error="Cannot restore the OpenGL Gallery display";return false;}return true;}
 void pulse(unsigned strength,unsigned duration=95)override{radio_input_pulse_for(strength,duration);}
 bool open(std::string& error)override{const int widths[]={1920,2560,3840},heights[]={1080,1440,2160};if(!display_.open(widths[output_],heights[output_],refresh_?120:60)){error="Cannot open PS5 EGL/OpenGL display";return false;}input_=radio_input_init();if(!input_){error="Cannot open the signed-in player's controller";return false;}return true;}
 Input poll()override{Input result;radio_input_poll();radio_input_event_t event;
  while(radio_input_next(&event))if(event.pressed)result.events.push_back({static_cast<Key>(event.key)});
  for(unsigned player=0;player<4;++player){alignas(16)unsigned char raw[120]{};if(!arcade_radio_input_sample(player,raw,sizeof raw)||!raw[76])continue;unsigned buttons=0;std::memcpy(&buttons,raw,4);if(buttons&0x80000000u){result.lost_focus=true;continue;}
   auto bind=[&](unsigned mask,unsigned id){if(buttons&mask)result.pads[player]|=1u<<id;};
   bind(0x10,RETRO_DEVICE_ID_JOYPAD_UP);bind(0x40,RETRO_DEVICE_ID_JOYPAD_DOWN);bind(0x80,RETRO_DEVICE_ID_JOYPAD_LEFT);bind(0x20,RETRO_DEVICE_ID_JOYPAD_RIGHT);
   bind(0x4000,RETRO_DEVICE_ID_JOYPAD_B);bind(0x2000,RETRO_DEVICE_ID_JOYPAD_A);bind(0x8000,RETRO_DEVICE_ID_JOYPAD_Y);bind(0x1000,RETRO_DEVICE_ID_JOYPAD_X);bind(0x400,RETRO_DEVICE_ID_JOYPAD_L);bind(0x800,RETRO_DEVICE_ID_JOYPAD_R);bind(0x100,RETRO_DEVICE_ID_JOYPAD_L2);bind(0x200,RETRO_DEVICE_ID_JOYPAD_R2);bind(0x1,RETRO_DEVICE_ID_JOYPAD_SELECT);bind(0x2,RETRO_DEVICE_ID_JOYPAD_L3);bind(0x8,RETRO_DEVICE_ID_JOYPAD_START);bind(0x4,RETRO_DEVICE_ID_JOYPAD_R3);
   if(raw[4]<64)result.pads[player]|=1u<<RETRO_DEVICE_ID_JOYPAD_LEFT;else if(raw[4]>192)result.pads[player]|=1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT;
   if(raw[5]<64)result.pads[player]|=1u<<RETRO_DEVICE_ID_JOYPAD_UP;else if(raw[5]>192)result.pads[player]|=1u<<RETRO_DEVICE_ID_JOYPAD_DOWN;
   if(player==0){static unsigned previous=0,notes=0;if(buttons!=previous&&notes<32){char line[96];std::snprintf(line,sizeof line,"P1 controller raw=0x%08x retro=0x%04x",buttons,result.pads[0]);arcade_startup_note(line);previous=buttons;++notes;}const unsigned masks[]={0x2,0x4,0x100,0x200};const Key extra[]={Key::L3,Key::R3,Key::L2,Key::R2};for(int i=0;i<4;++i)if((buttons&masks[i])&&!(last_extra_&masks[i]))result.events.push_back({extra[i]});last_extra_=buttons;unsigned touch=buttons&0x100000;if(touch&&!last_touch_)result.events.push_back({Key::Touchpad});last_touch_=touch;}
  }return result;
 }
 bool present()override{bool result=display_.swap();if(result&&first_){sceSystemServiceHideSplashScreen();first_=false;}return result;}
 void size(int& w,int& h)override{w=display_.width();h=display_.height();}
};
class Ps5Audio final:public AudioOutput {
 static constexpr std::size_t capacity=384000;std::vector<int16_t> ring_;
 std::atomic<std::size_t> read_{0},write_{0};std::atomic<bool> done_{false},paused_{false};
 pthread_t thread_{};int handle_=-1;bool started_=false;unsigned rate_=48000;double phase_=0;
 static void* entry(void* p){static_cast<Ps5Audio*>(p)->loop();return nullptr;}
 void loop(){alignas(64)int16_t grain[512];while(!done_.load(std::memory_order_relaxed)){
   auto read=read_.load(std::memory_order_relaxed);auto write=write_.load(std::memory_order_acquire);
   for(unsigned i=0;i<256;++i){if(paused_.load(std::memory_order_relaxed)||write-read<4){grain[i*2]=grain[i*2+1]=0;continue;}
    for(unsigned c=0;c<2;++c){double a=ring_[(read+c)%capacity],b=ring_[(read+2+c)%capacity];grain[i*2+c]=static_cast<int16_t>(a+(b-a)*phase_);}
    phase_+=rate_/48000.;auto step=static_cast<std::size_t>(phase_);phase_-=step;read+=2*step;if(read>write)read=write;
   }read_.store(read,std::memory_order_release);if(sceAudioOutOutput(handle_,grain)<0)sceKernelUsleep(5000);
  }sceAudioOutOutput(handle_,nullptr);
 }
public:
 ~Ps5Audio()override{stop();}
 bool configure(unsigned rate)override{stop();rate_=rate;phase_=0;read_=write_=0;done_=paused_=false;ring_.assign(capacity,0);sceAudioOutInit();handle_=sceAudioOutOpen(0xff,0,0,256,48000,1);if(handle_<0)return false;if(pthread_create(&thread_,nullptr,entry,this)){sceAudioOutClose(handle_);handle_=-1;return false;}started_=true;return true;}
 void enqueue(const int16_t* samples,std::size_t frames)override{if(!started_)return;auto read=read_.load(std::memory_order_acquire),write=write_.load(std::memory_order_relaxed);for(std::size_t i=0;i<frames*2&&write-read<capacity;++i)ring_[write++%capacity]=samples[i];write_.store(write,std::memory_order_release);}
 void pause(bool p)override{paused_.store(p);}
 void stop()override{if(started_){done_.store(true);pthread_join(thread_,nullptr);started_=false;}if(handle_>=0)sceAudioOutClose(handle_);handle_=-1;ring_.clear();}
};
}
std::unique_ptr<Platform> make_platform(){return std::make_unique<Ps5Platform>();}
std::unique_ptr<AudioOutput> make_audio(){return std::make_unique<Ps5Audio>();}
}
