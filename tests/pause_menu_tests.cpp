#include "ui/pause_menu.hpp"
#include "backend/mame_session.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace arcade;using namespace arcade::native;
struct TrackedSession {
 MameSession game;int saves=0;bool audio_paused=false;
 void pause(bool value){audio_paused=value;game.pause(value);}
 bool save_state(){++saves;return game.save_state();}
 const std::string& error()const{return game.error();}const std::string& error_code()const{return game.error_code();}
};
struct FailedSession {std::string text;void pause(bool){}bool save_state(){return false;}std::string error()const{return text;}std::string error_code()const{return "ARC-2001";}};
std::string contents(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char** argv){
 assert(argc==2);auto root=std::filesystem::temp_directory_path()/("arcade-pause-"+std::to_string(getpid()));std::filesystem::create_directories(root);
 auto rom=root/"robby.zip";std::ofstream(rom)<<"fixture";Game fixture{"robby","Robby Roto","Bally",1981,rom};
 TrackedSession session;PauseMenu menu;int stops=0;auto stop=[&](){++stops;session.game.stop();};std::array<uint16_t,4> pads{};
 auto start=[&](){assert(session.game.start(fixture,argv[1],root/"data"));for(int i=0;i<3;++i)session.game.run(pads);menu.open(session);assert(menu.paused()&&session.audio_paused);};
 // Exit without Save never creates a snapshot.
 start();assert(menu.apply(PauseAction::Exit,session,stop));assert(stops==1&&session.saves==0&&!session.game.active()&&!menu.paused());
 auto state=Core::state_path(rom,root/"data");assert(!std::filesystem::exists(state));
 // Saving leaves emulation and the menu paused, and repeated Save is idempotent.
 start();auto frames=session.game.frames();assert(!menu.apply(PauseAction::Save,session,stop));assert(menu.result()==SaveResult::Success);
 assert(menu.paused()&&session.audio_paused&&session.game.active()&&session.game.frames()==frames&&session.saves==1);
 auto saved=contents(state);assert(!saved.empty());menu.apply(PauseAction::Save,session,stop);assert(session.saves==1);
 assert(!menu.apply(PauseAction::Resume,session,stop));assert(!menu.paused()&&!session.audio_paused&&session.game.active());
 session.game.run(pads);menu.open(session);assert(menu.result()==SaveResult::None);
 assert(menu.apply(PauseAction::Exit,session,stop));assert(session.saves==1&&contents(state)==saved);
 // A real write failure keeps the prior save and supports a retry without running a frame.
 start();frames=session.game.frames();auto partial=std::filesystem::path(state.string()+".partial");std::filesystem::create_directories(partial);std::ofstream(partial/"blocker")<<"fixture";
 menu.apply(PauseAction::Save,session,stop);assert(menu.result()==SaveResult::Failure&&menu.paused()&&session.game.active());assert(session.game.frames()==frames&&contents(state)==saved);
 assert(menu.message().find("write")!=std::string::npos&&menu.title().find("ARC-2001")!=std::string::npos);
 std::filesystem::remove_all(partial);menu.apply(PauseAction::Save,session,stop);assert(menu.result()==SaveResult::Success&&session.game.frames()==frames);
 auto attempts=session.saves;menu.apply(PauseAction::Exit,session,stop);assert(session.saves==attempts&&contents(state)==saved);
 // A failed save also allows Exit directly, with no hidden retry or overwrite.
 start();std::filesystem::create_directories(partial);std::ofstream(partial/"blocker")<<"fixture";menu.apply(PauseAction::Save,session,stop);attempts=session.saves;menu.apply(PauseAction::Exit,session,stop);assert(session.saves==attempts&&contents(state)==saved);
 for(bool swap:{false,true}){assert(PauseMenu::action(Key::Square,swap)==PauseAction::Save);assert(PauseMenu::action(swap?Key::Circle:Key::Cross,swap)==PauseAction::Resume);assert(PauseMenu::action(swap?Key::Cross:Key::Circle,swap)==PauseAction::Exit);}
 // Page a long error without trimming any content or overlapping the equal buttons.
 FailedSession failed;for(int i=0;i<100;++i)failed.text+="Complete diagnostic line "+std::to_string(i)+" remains readable.\n";
 PauseMenu long_error;long_error.open(failed);long_error.apply(PauseAction::Save,failed,[](){});
 auto compact=[](const std::string& text){std::string result;for(char ch:text)if(ch!=' '&&ch!='\n'&&ch!='\t'&&ch!='\r')result+=ch;return result;};
 Renderer renderer;
 for(bool large:{false,true}){auto layout=pause_layout(long_error,"The King of Fighters '98: Ultimate Match",large);assert(layout.box.h==460&&layout.box.y>=0&&layout.box.y+layout.box.h<=941);
  auto toast=save_toast_layout(long_error,large,long_error.saved_at());assert(toast.visible&&toast.pages>1&&toast.box.x==1080&&toast.box.y==24);
  std::string recovered;for(const auto& line:toast.lines){assert(text_width(line,19,0,large)<=472+.01f);recovered+=line;}assert(compact(recovered)==compact(long_error.message()));
  assert(!save_toast_layout(long_error,large,long_error.saved_at()+std::chrono::seconds(1000)).visible);
  assert(save_toast_layout(long_error,large,long_error.saved_at()+std::chrono::seconds(6)).page==1);
  renderer.large_text(large);for(int i=0;i<3;++i){auto button=layout.buttons[i];assert(button.w==336&&button.h==64);assert(layout.hit(button.x+30,button.y+30)==static_cast<PauseAction>(i+1));}
  for(auto label:{"Resume","Save","Exit"})assert(renderer.measure(label,26,1)<=244);
 }
 std::filesystem::remove_all(root);std::cout<<"Independent save/exit, paused-frame preservation, failed-write retry, prior saves, swapped input, paging and action hit targets passed\n";
}
