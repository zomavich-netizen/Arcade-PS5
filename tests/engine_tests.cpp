#include "core.hpp"
#include "library.hpp"
#include "../native/platform/types.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
class CapturedAudio:public arcade::AudioOutput{public:std::vector<int16_t> samples;bool configure(unsigned)override{return true;}void enqueue(const int16_t* data,std::size_t frames)override{samples.assign(data,data+frames*2);}void pause(bool)override{}void stop()override{}};
int main(int argc,char** argv){
 assert(argc==2);
 auto root=std::filesystem::temp_directory_path()/("arcade-test-"+std::to_string(getpid()));
 std::filesystem::create_directories(root/"roms");
 std::ofstream(root/"roms/robby.zip")<<"fixture";std::ofstream(root/"roms/neogeo.zip")<<"bios";std::ofstream(root/"roms/qsound.zip")<<"sound support";std::ofstream(root/"roms/readme.txt")<<"ignore";
 auto games=arcade::scan(root/"roms");assert(games.size()==1&&games[0].title=="Robby Roto");
 arcade::write_favorites(root/"data/favorites.txt",{"robby"});assert(arcade::read_favorites(root/"data/favorites.txt").count("robby"));
 CapturedAudio output;arcade::Core core(&output);assert(core.load(argv[1],games[0].path,root/"data"));core.buttons[1]=1<<RETRO_DEVICE_ID_JOYPAD_A;core.run();
 assert(core.width()==2&&core.height()==1);assert(core.pixels()[0]==0xffff0000);assert(core.pixels()[1]==0xff00ff00);assert(core.video_frames==1&&core.audio_frames==2);
 core.audio_settings(50,false);core.run();assert((output.samples==std::vector<int16_t>{0,0,1,-1}));core.audio_settings(100,true);core.run();assert((output.samples==std::vector<int16_t>{0,0,0,0}));core.audio_settings(100,false);
 // Persist exact emulation progress across a full unload/reload.
 assert(core.save_state());auto save=arcade::Core::state_path(games[0].path,root/"data");assert(std::filesystem::is_regular_file(save));
 core.run();auto next_pixel=core.pixels()[0];
 auto read_save=[&](){std::ifstream in(save,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
 auto original=read_save();core.buttons[0]=1<<RETRO_DEVICE_ID_JOYPAD_L;core.run();assert(!core.save_state());assert(read_save()==original);
 core.buttons.fill(0);core.run();std::filesystem::create_directory(save.string()+".partial");
 assert(!core.save_state());assert(read_save()==original);std::filesystem::remove(save.string()+".partial");
 core.stop();assert(core.load(argv[1],games[0].path,root/"data"));assert(core.load_state());core.run();assert(core.pixels()[0]==next_pixel);assert(core.save_state());assert(read_save()!=original);
 {std::ofstream out(save,std::ios::binary);auto damaged=original;damaged.back()^=1;out.write(damaged.data(),damaged.size());}
 assert(!core.load_state()&&core.playing());assert(core.error().find("damaged")!=std::string::npos);
 {std::ofstream out(save,std::ios::binary);out.write(original.data(),original.size()-1);}
 assert(!core.load_state()&&core.playing());
 {std::ofstream out(save,std::ios::binary);auto incompatible=original;incompatible[0]='X';out.write(incompatible.data(),incompatible.size());}
 assert(!core.load_state()&&core.playing());
 {std::ofstream out(save,std::ios::binary);out.write(original.data(),original.size());}
 core.stop();assert(core.load(argv[1],games[0].path,root/"other-profile"));assert(!core.load_state());core.run();assert(core.pixels()[0]==0xffff0000);
 core.stop();std::ofstream(root/"roms/unsupported.zip")<<"fixture";
 assert(core.load(argv[1],root/"roms/unsupported.zip",root/"data"));assert(!core.save_state()&&core.playing());
 assert(!std::filesystem::exists(arcade::Core::state_path(root/"roms/unsupported.zip",root/"data")));
 core.stop();assert(!core.playing());assert(core.load(argv[1],games[0].path,root/"data"));core.run();assert(core.pixels()[1]==0xff0000ff);
 core.stop();assert(!core.load(root/"missing.dylib",games[0].path,root/"data"));assert(!core.error().empty());
 // MAME's first update may have no image. Vulkan must wait for actual video.
 std::ofstream(root/"roms/delayed.zip")<<"fixture";
 assert(core.load(argv[1],root/"roms/delayed.zip",root/"data"));
 core.run();arcade::native::Frame frame{core.pixels().data(),core.width(),core.height(),core.aspect()};assert(!frame.ready());
 core.run();frame={core.pixels().data(),core.width(),core.height(),core.aspect()};assert(frame.ready());core.stop();
 std::ofstream(root/"roms/oom.zip")<<"fixture";
 assert(!core.load(argv[1],root/"roms/oom.zip",root/"data"));
 assert(core.error().find("not enough memory")!=std::string::npos);assert(core.error_code()=="ARC-1001");
 assert(core.error().find("BIOS")==std::string::npos);
 assert(core.load(argv[1],games[0].path,root/"data"));core.stop();
 // Neo Geo must finish firmware startup before restoring gameplay. A single
 // warmup call leaves a later reset able to overwrite a valid snapshot.
 std::ofstream(root/"roms/neogeo-state.zip")<<"fixture";
 assert(core.load(argv[1],root/"roms/neogeo-state.zip",root/"data"));
 for(int i=0;i<10;++i)core.run();assert(core.save_state());
 core.run();auto restored_pixel=core.pixels()[0];core.stop();
 assert(core.load(argv[1],root/"roms/neogeo-state.zip",root/"data"));
 core.buttons.fill(0xffff);assert(core.load_state());assert(core.video_frames>=360);
 for(auto pad:core.buttons)assert(pad==0);
 core.run();assert(core.pixels()[0]==restored_pixel);
 for(int i=0;i<8;++i)core.run();auto later_pixel=core.pixels()[0];core.stop();
 assert(core.load(argv[1],root/"roms/neogeo-state.zip",root/"data"));assert(core.load_state());
 for(int i=0;i<9;++i)core.run();assert(core.pixels()[0]==later_pixel);core.stop();
 // Never silently fall back to a fresh game when firmware cannot get ready.
 std::ofstream(root/"roms/neogeo-state-stuck.zip")<<"fixture";
 assert(core.load(argv[1],root/"roms/neogeo-state-stuck.zip",root/"data"));
 for(int i=0;i<10;++i)core.run();assert(core.save_state());core.stop();
 assert(core.load(argv[1],root/"roms/neogeo-state-stuck.zip",root/"data"));assert(!core.load_state());
 assert(core.error().find("startup did not finish")!=std::string::npos);assert(core.video_frames==900);core.stop();
 // Respect libretro orientation for every pixel format, with padded rows.
 const uint32_t red=0xffff0000,green=0xff00ff00,blue=0xff0000ff,yellow=0xffffff00,magenta=0xffff00ff,cyan=0xff00ffff;
 const std::vector<std::vector<uint32_t>> expected={{red,green,blue,yellow,magenta,cyan},{blue,cyan,green,magenta,red,yellow},{cyan,magenta,yellow,blue,green,red},{yellow,red,magenta,green,cyan,blue}};
 for(auto format:{"8888","565","1555"})for(unsigned rotation=0;rotation<4;++rotation){
  auto rom=root/"roms"/("orientation-"+std::to_string(rotation)+"-"+format+".zip");std::ofstream(rom)<<"fixture";
  assert(core.load(argv[1],rom,root/"data"));core.run();assert(core.rotation()==rotation);
  assert(core.width()==(rotation%2?2u:3u)&&core.height()==(rotation%2?3u:2u));assert(core.pixels()==expected[rotation]);
  assert(std::abs(core.aspect()-(rotation%2?2.f/3:1.5f))<.0001f);
  assert(core.save_state());core.stop();assert(core.rotation()==0);
  assert(core.load(argv[1],rom,root/"data")&&core.load_state());assert(core.pixels()==expected[rotation]);core.stop();
 }
 auto dynamic=root/"roms/orientation-dynamic.zip";std::ofstream(dynamic)<<"fixture";
 assert(core.load(argv[1],dynamic,root/"data"));core.run();assert(core.pixels()==expected[0]);core.run();assert(core.pixels()==expected[1]);core.run();assert(core.pixels()==expected[0]);core.stop();
 assert(core.load(argv[1],games[0].path,root/"data"));core.run();assert(core.rotation()==0&&core.width()==2&&core.height()==1&&core.aspect()==2.f);core.stop();
 std::filesystem::remove_all(root);std::cout<<"Library, favorites, RGB565 stride conversion, P2 input, audio, reload and missing engine checks passed\n";
}
