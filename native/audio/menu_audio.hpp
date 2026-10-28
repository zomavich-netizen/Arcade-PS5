#pragma once
#include "audio_output.hpp"
#include "settings.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace arcade::native {
// A bounded PCM reader and menu-only mixer. Gameplay keeps exclusive ownership
// of AudioOutput while a core is active; no music is mixed into game audio.
class MenuAudio {
 std::vector<int16_t> music_,mixed_;size_t cursor_=0,cue_frame_=576;int cue_=0;double ramp_=0,remainder_=0;
 std::chrono::steady_clock::time_point last_{};
 static unsigned u16(const unsigned char* p){return p[0]|unsigned(p[1])<<8;}
 static unsigned u32(const unsigned char* p){return u16(p)|u16(p+2)<<16;}
public:
 bool load(const std::filesystem::path& file){
  music_.clear();cursor_=0;std::ifstream in(file,std::ios::binary);std::vector<unsigned char> bytes;char block[8192];
  if(!in)return false;
  while(in){in.read(block,sizeof block);auto n=size_t(in.gcount());if(bytes.size()+n>8u*1024u*1024u)return false;bytes.insert(bytes.end(),block,block+n);}
  if(!in.eof()||bytes.size()<12||std::memcmp(bytes.data(),"RIFF",4)||std::memcmp(bytes.data()+8,"WAVE",4)||u32(bytes.data()+4)>bytes.size()-8)return false;
  unsigned channels=0,rate=0,align=0;size_t offset=0,length=0;bool pcm=false;
  for(size_t pos=12;pos+8<=bytes.size();){auto* header=bytes.data()+pos;size_t n=u32(header+4);pos+=8;if(n>bytes.size()-pos)return false;
   if(!std::memcmp(header,"fmt ",4)){if(n<16)return false;channels=u16(bytes.data()+pos+2);rate=u32(bytes.data()+pos+4);align=u16(bytes.data()+pos+12);pcm=u16(bytes.data()+pos)==1&&u16(bytes.data()+pos+14)==16;}
   if(!std::memcmp(header,"data",4)){offset=pos;length=n;}
   pos+=n+(n&1);if(pos>bytes.size())return false;
  }
  if(!pcm||(channels!=1&&channels!=2)||rate<8000||rate>96000||align!=channels*2||!offset||!length||length%align)return false;
  size_t frames=length/align,out=std::llround(double(frames)*48000/rate);if(!out||out>48000u*60u)return false;
  music_.resize(out*2);
  for(size_t i=0;i<out;++i){double at=double(i)*frames/out;size_t a=size_t(at),b=(a+1)%frames;double fraction=at-a;
   for(unsigned channel=0;channel<2;++channel){unsigned c=std::min(channel,channels-1);auto sample=[&](size_t frame){return static_cast<int16_t>(u16(bytes.data()+offset+(frame*channels+c)*2));};music_[i*2+channel]=int16_t(std::lround(sample(a)*(1-fraction)+sample(b)*fraction));}
  }
  return true;
 }
 size_t music_frames()const{return music_.size()/2;}
 void reset_clock(std::chrono::steady_clock::time_point now=std::chrono::steady_clock::now()){last_=now-std::chrono::milliseconds(30);remainder_=0;ramp_=0;cue_frame_=576;}
 const std::vector<int16_t>& mix(size_t frames,const Settings& settings,int cue=0){
  frames=std::min(frames,size_t(4800));mixed_.assign(frames*2,0);if(cue){cue_=cue;cue_frame_=0;}
  double volume=settings.mute?0:settings.menu_volume/100.;double target=settings.menu_music?1:0;
  for(size_t i=0;i<frames;++i){ramp_+=std::clamp(target-ramp_,-1./4800,1./4800);double click=0;
   if(cue_frame_<576){click=std::sin(6.283185307179586*(cue_==3?145:cue_==2?620:290)*cue_frame_/48000)*std::sin(3.141592653589793*cue_frame_/575)*1100;++cue_frame_;}
   for(int channel=0;channel<2;++channel){double sample=click;if(!music_.empty())sample+=music_[cursor_*2+channel]*.12*ramp_;mixed_[i*2+channel]=static_cast<int16_t>(std::clamp(std::lround(sample*volume),-32768l,32767l));}
   if(!music_.empty())cursor_=(cursor_+1)%music_frames();
  }
  return mixed_;
 }
 void pump(AudioOutput& output,const Settings& settings,int cue=0,std::chrono::steady_clock::time_point now=std::chrono::steady_clock::now()){
  if(last_==std::chrono::steady_clock::time_point{})reset_clock(now);
  double frames=std::clamp(std::chrono::duration<double>(now-last_).count(),0.,.1)*48000+remainder_;last_=now;
  auto count=size_t(frames);remainder_=frames-count;const auto& samples=mix(count,settings,cue);if(count)output.enqueue(samples.data(),count);
 }
};
}
