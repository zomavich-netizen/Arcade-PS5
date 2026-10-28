#include "audio/menu_audio.hpp"
#include <cassert>
#include <iostream>
using namespace arcade;using namespace arcade::native;
static void put(std::ofstream& f,unsigned value,int n){for(int i=0;i<n;++i)f.put(char(value>>(8*i)));}
static void wav(const std::filesystem::path& file,unsigned rate,unsigned channels,const std::vector<int16_t>& pcm){std::ofstream f(file,std::ios::binary);f<<"RIFF";put(f,36+pcm.size()*2,4);f<<"WAVEfmt ";put(f,16,4);put(f,1,2);put(f,channels,2);put(f,rate,4);put(f,rate*channels*2,4);put(f,channels*2,2);put(f,16,2);f<<"data";put(f,pcm.size()*2,4);for(auto s:pcm)put(f,unsigned(uint16_t(s)),2);}
int main(int argc,char** argv){assert(argc==2);auto root=std::filesystem::temp_directory_path()/"arcade-menu-audio-test";std::filesystem::create_directories(root);auto file=root/"loop.wav";
 wav(file,48000,2,{10000,-10000,-10000,10000});MenuAudio music;assert(music.load(file)&&music.music_frames()==2);Settings s;s.menu_volume=100;
 music.mix(4800,s);auto samples=music.mix(4,s);assert((samples==std::vector<int16_t>{1200,-1200,-1200,1200,1200,-1200,-1200,1200}));
 s.menu_volume=0;auto silent=music.mix(32,s,2);assert(std::all_of(silent.begin(),silent.end(),[](auto v){return v==0;}));
 s.menu_volume=30;s.mute=true;silent=music.mix(32,s,3);assert(std::all_of(silent.begin(),silent.end(),[](auto v){return v==0;}));
 s.mute=false;s.menu_music=false;music.mix(4800,s);silent=music.mix(64,s);assert(std::all_of(silent.begin(),silent.end(),[](auto v){return v==0;}));
 // A missing or malformed soundtrack still permits independent UI sound effects.
 MenuAudio missing;assert(!missing.load(root/"missing.wav"));auto cue=missing.mix(576,s,2);assert(std::any_of(cue.begin(),cue.end(),[](auto v){return v!=0;}));
 wav(file,24000,1,{12000,-12000});assert(music.load(file)&&music.music_frames()==4);s.menu_music=true;auto mono=music.mix(20,s);for(size_t i=0;i<mono.size();i+=2)assert(mono[i]==mono[i+1]);
 {std::ofstream out(file,std::ios::binary);out<<"RIFF\xff\xff\xff\xffWAVE";}assert(!music.load(file));
 {std::ofstream out(file,std::ios::binary);out<<"not an audio file";}assert(!music.load(file));
 MenuAudio production;assert(production.load(argv[1]));assert(production.music_frames()>48000*6&&production.music_frames()<48000*7);auto actual=production.mix(4800,s);assert(actual.size()==9600&&std::any_of(actual.begin(),actual.end(),[](auto v){return v!=0;}));
 std::filesystem::remove_all(root);std::cout<<"Loop boundaries, stereo/mono conversion, resampling, mute, volume, toggle, UI cues, malformed files and production music passed\n";
}
