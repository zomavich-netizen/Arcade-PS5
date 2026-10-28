#include "ui/gallery.hpp"
#include "libretro.h"
#include "core.hpp"
#include "ui/vault_footer.hpp"
#include "ui/message_dialog.hpp"
#include <fstream>
#include <thread>
#include <cmath>
#include <cassert>
#include <filesystem>
#include <unistd.h>
using namespace arcade;
using namespace arcade::native;
int main(int argc,char** argv){
 assert(argc==2);
 auto data=std::filesystem::temp_directory_path()/("arcade-vault-state-"+std::to_string(getpid()));
 Settings defaults;
 {
  Gallery ui(data,defaults,true);assert(ui.current()->id=="robby");
  ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT);ui.input(Key::Left);assert(ui.take_feedback()==3);
  ui.input(Key::Left);assert(ui.take_feedback()!=3);ui.controller(0);
  for(int i=0;i<5;++i)ui.input(Key::Right);assert(ui.current()->id=="alienar");assert(ui.take_feedback()==1);
  ui.input(Key::Right);assert(ui.current()->id=="alienar");assert(ui.take_feedback()==3);
  ui.input(Key::L3);assert(ui.credits()==1);ui.input(Key::L3);assert(ui.credits()==2);
  ui.click(1400,740);assert(ui.game_sheet_open());ui.input(Key::Down);ui.input(Key::Down);ui.input(Key::Down);ui.input(Key::Left);
  assert(ui.runtime_preferences().volume==90);assert(ui.preferences().volume==100);
  ui.input(Key::Circle);assert(!ui.game_sheet_open());ui.input(Key::Left);assert(ui.runtime_preferences().volume==100);ui.input(Key::Right);assert(ui.runtime_preferences().volume==90);
  ui.input(Key::Square);ui.input(Key::Right);assert(ui.runtime_preferences().renderer==1&&ui.preferences().renderer==0);ui.input(Key::Circle);
  ui.input(Key::Square);ui.input(Key::R2);ui.input(Key::Down);ui.input(Key::Right);assert(ui.runtime_preferences().start_button==1);ui.input(Key::Circle);
  ui.input(Key::Triangle);ui.input(Key::Circle);assert(ui.page()==Page::Favorites&&ui.current());ui.input(Key::Circle);assert(ui.page()==Page::Library);
  ui.click(650,890);assert(ui.page()==Page::Favorites);ui.click(650,890);assert(ui.page()==Page::Library);
  ui.click(1520,890);assert(ui.page()==Page::Settings);ui.input(Key::Circle);assert(ui.page()==Page::Library);
  assert(ui.take_credits()==2&&ui.credits()==0);
 }
 {
  Gallery restored(data,read_settings(data/"settings.cfg"),true);for(int i=0;i<5;++i)restored.input(Key::Right);
  assert(restored.runtime_preferences().volume==90&&restored.runtime_preferences().start_button==1&&restored.runtime_preferences().renderer==1);
 }
 // Error text, including long diagnostic paths, remains accessible on every
 // page at both text sizes. Buttons stay centered and clicks outside do not dismiss.
 {
  std::string message="There is not enough memory to load this game. Close Arcade completely, reopen it, and try again.\n\n";
  message+=std::string(240,'p')+"/kof98um_v1_00.chd\n";
  for(int i=0;i<45;++i)message+="Diagnostic line "+std::to_string(i)+": the complete memory error remains readable.\n";
  auto compact=[](const std::string& text){std::string out;for(char ch:text)if(ch!=' '&&ch!='\n'&&ch!='\t'&&ch!='\r')out+=ch;return out;};
  for(bool large:{false,true}){
   auto layout=message_layout(message,large);assert(layout.pages>1);
   std::string recovered;for(const auto& line:layout.lines){assert(text_width(line,24,0,large)<=layout.body.w+.01f);recovered+=line;}
   assert(compact(recovered)==compact(message));
   assert(layout.box.y>=0&&layout.box.y+layout.box.h<=941);
   assert(layout.body.y+layout.body.h<layout.close.y);
   assert(text_width("Previous",21,1,large)<=layout.previous.w-92);
   assert(text_width("Exit without saving",20,1,large)<=312-92);
   assert(std::abs((layout.previous.x+layout.next.x+layout.next.w)/2-836)<.01f);
   auto options=defaults;options.large_text=large;options.swap_menu=large;Gallery ui(data/(large?"large-error":"error"),options,true);
   ui.error(message,"ARC-1001");assert(ui.message_open()&&ui.message_page()==0);
   ui.click(1,1);assert(ui.message_open());
   for(size_t i=1;i<layout.pages;++i){ui.input(Key::R1);assert(ui.message_page()==i);}
   ui.input(Key::R1);assert(ui.message_page()==layout.pages-1);ui.input(Key::L1);assert(ui.message_page()==layout.pages-2);
   ui.click(layout.previous.x+30,layout.previous.y+20);assert(ui.message_open());
   ui.input(large?Key::Circle:Key::Cross);assert(!ui.message_open());
   ui.error("A short message","ARC-1001");assert(ui.message_page()==0);
   auto short_layout=message_layout("A short message",large);assert(short_layout.pages==1&&short_layout.close.x+short_layout.close.w/2==836);
   ui.click(short_layout.close.x+30,short_layout.close.y+20);assert(!ui.message_open());
  }
 }
 // The selected card preserves equal actions at normal/large text sizes and
 // reports real save presence, times, and unknown metadata without sample data.
 {
  assert(card::play_button.w==card::settings_button.w&&card::play_button.h==card::settings_button.h);
  assert(card::button_text_size==24);
  Renderer renderer;
  for(bool large:{false,true}){renderer.large_text(large);assert(renderer.measure("Resume",card::button_text_size,1)<=card::play_button.w-92);assert(renderer.measure("Settings",card::button_text_size,1)<=card::settings_button.w-92);}
  auto state=data/"card-save.state";assert(!card::inspect_save(state).available);
  assert(card::save_time({})=="No save yet");std::ofstream(state)<<"saved state";
  auto saved=card::inspect_save(state);assert(saved.available&&saved.modified>0);
  assert(card::save_time(saved,saved.modified).find("Today, ")==0);assert(!card::inspect_save(data).available);
  Game unknown;assert(card::players(unknown)=="Unknown"&&card::save_support(unknown)=="Unknown"&&card::compatibility(unknown)=="MAME: Not rated");
  assert(card::display_title("Cadillacs and Dinosaurs (World 930201)")=="Cadillacs and Dinosaurs");
 }
 // Equal visible gaps, one baseline and a right-anchored settings shortcut,
 // including the accessibility text size.
 for(bool large:{false,true}){auto items=footer_layout(large);float gap=items[1].bounds.x-items[0].bounds.x-items[0].bounds.w;assert(gap>0);for(size_t i=1;i<5;++i){assert(std::abs(items[i].bounds.x-items[i-1].bounds.x-items[i-1].bounds.w-gap)<.01f);assert(items[i].bounds.y==items[5].bounds.y);}assert(items[0].bounds.y==items[5].bounds.y);assert(std::abs(items[5].bounds.x+items[5].bounds.w-1624)<.01f);assert(items[4].bounds.x+items[4].bounds.w<items[5].bounds.x);}
 auto collection=data/"collection";std::filesystem::create_directories(collection/"roms");std::filesystem::create_directories(collection/"covers");
 for(const auto* id:{"dino","robby"}){std::ofstream(collection/"roms"/(std::string(id)+".zip"));std::filesystem::copy_file(argv[1],collection/"covers"/(std::string(id)+".png"));}
 Settings live;live.roms=collection/"roms";
 {
  Gallery ui(data/"live",live,false);assert(ui.current()->id=="dino");
  ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT);ui.input(Key::Right);assert(ui.current()->id=="robby"&&ui.take_feedback()==1);
  ui.input(Key::Right);assert(ui.take_feedback()==3);ui.input(Key::Right);assert(ui.take_feedback()!=3);ui.controller(0);
  ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT);ui.input(Key::Left);assert(ui.current()->id=="dino"&&ui.take_feedback()==1);
  ui.input(Key::Left);assert(ui.take_feedback()==3);ui.input(Key::Left);assert(ui.take_feedback()!=3);ui.controller(0);
  // Vertical and shoulder inputs do not browse games or pulse at an edge.
  for(auto key:{Key::Up,Key::Down,Key::L1,Key::R1}){ui.input(key);assert(ui.current()->id=="dino"&&ui.take_feedback()==0);}
  ui.input(Key::Right);assert(ui.current()->id=="robby"&&ui.take_feedback()==1);
  for(auto key:{Key::Up,Key::Down,Key::L1,Key::R1}){ui.input(key);assert(ui.current()->id=="robby"&&ui.take_feedback()==0);}
  ui.input(Key::Left);assert(ui.current()->id=="dino"&&ui.take_feedback()==1);
  for(auto key:{Key::Cross,Key::Options,Key::R3}){ui.input(key);assert(!ui.launching()&&!ui.take_launch()&&!ui.take_start());assert(ui.take_feedback()!=3);}
  ui.click(1300,650);assert(!ui.launching());auto play=footer_layout()[1].bounds;ui.click(play.x+45,890);assert(!ui.launching());ui.click(263,560);assert(!ui.launching());
  ui.input(Key::L3);assert(ui.credits()==1);ui.input(Key::Cross);assert(ui.launching());std::this_thread::sleep_for(std::chrono::milliseconds(610));auto launch=ui.take_launch();assert(launch&&launch->id=="dino");assert(ui.take_credits()==1&&!ui.take_start());ui.finish_launch();ui.input(Key::Cross);assert(!ui.launching());
  ui.input(Key::L3);ui.input(Key::Options);assert(ui.launching()&&ui.take_start());
 }
 {
  auto swapped=live;swapped.swap_menu=true;Gallery ui(data/"swapped",swapped,false);ui.input(Key::Circle);assert(!ui.launching());ui.input(Key::L3);ui.input(Key::Circle);assert(ui.launching());
 }
 // Production Gallery connects scanning to its bundled metadata catalogue.
 {
  Gallery ui(data/"metadata",live,false,std::filesystem::path(argv[1]).parent_path()/"mame-artwork.tsv");
  assert(ui.current()->id=="dino"&&ui.current()->maker=="Capcom"&&ui.current()->year==1993);
  assert(ui.current()->title=="Cadillacs and Dinosaurs (World 930201)");
 }
 // Saved games offer resume, fresh start and cancel, with profile isolation.
 {
  auto saved=data/"resume";auto state=Core::state_path(collection/"roms/dino.zip",profile_directory(saved,live.profile));
  std::filesystem::create_directories(state.parent_path());std::ofstream(state)<<"fixture";
  Gallery ui(saved,live,false);ui.input(Key::Cross);assert(ui.choosing_start()&&!ui.launching());
  ui.input(Key::Circle);assert(!ui.choosing_start()&&!ui.launching());
  ui.input(Key::Options);assert(ui.choosing_start());ui.input(Key::Cross);assert(ui.launching()&&ui.load_requested()&&!ui.take_start());
  std::this_thread::sleep_for(std::chrono::milliseconds(610));auto launch=ui.take_launch();assert(launch&&launch->id=="dino");ui.finish_launch();
  ui.input(Key::Cross);ui.input(Key::Down);ui.input(Key::Cross);assert(ui.choosing_start()&&!ui.launching());
  ui.input(Key::L3);ui.input(Key::Cross);assert(ui.launching()&&!ui.load_requested()&&ui.take_credits()==1);ui.finish_launch();
  ui.input(Key::Cross);auto start_layout=ui.start_choice_layout();ui.click(start_layout.buttons[0].x+40,start_layout.buttons[0].y+30);assert(ui.launching()&&ui.load_requested());ui.finish_launch();
  ui.input(Key::Cross);auto buttons=ui.start_choice_layout().buttons;
  for(auto button:buttons)assert(button.w==336&&button.h==64);
  ui.click(buttons[2].x+40,buttons[2].y+30);assert(!ui.choosing_start()&&!ui.launching());
  ui.input(Key::Cross);ui.input(Key::Square);assert(ui.choosing_start()&&!ui.launching());
  ui.input(Key::L3);ui.input(Key::Square);assert(ui.launching()&&!ui.load_requested()&&ui.take_credits()==1);ui.finish_launch();
  ui.input(Key::Cross);ui.input(Key::L3);ui.click(buttons[1].x+40,buttons[1].y+30);assert(ui.launching()&&!ui.load_requested()&&ui.take_credits()==1);ui.finish_launch();
  ui.input(Key::Cross);ui.input(Key::Circle);ui.input(Key::Right);ui.input(Key::Cross);assert(!ui.choosing_start()&&!ui.launching());
  auto swapped=live;swapped.swap_menu=true;Gallery other(saved,swapped,false);other.input(Key::Circle);assert(other.choosing_start());other.input(Key::Cross);assert(!other.choosing_start());
  Gallery separate(data/"no-save",live,false);separate.input(Key::Cross);assert(!separate.choosing_start());
 }
 // Both new action rectangles route mouse input to the correct action.
 {
  Gallery ui(data/"card-clicks",live,false);ui.click(1450,755);assert(ui.game_sheet_open());ui.input(Key::Circle);
  ui.input(Key::L3);ui.click(1250,755);assert(ui.launching());
 }
 // A first game's vibration override must not mute collection-edge feedback.
 {
  auto saved=data/"feedback";std::filesystem::create_directories(saved/"game-settings");
  auto first=live;first.vibration=false;first.vibration_strength=25;
  write_settings(saved/"game-settings/dino.cfg",first);
  Gallery ui(saved,live,false);assert(ui.current()->id=="dino");
  assert(!ui.runtime_preferences().vibration&&ui.feedback_preferences().vibration);
  for(auto key:{Key::Left}){
   ui.controller(0);ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT);ui.input(key);
   assert(ui.take_feedback()==3&&ui.feedback_preferences().vibration_strength==live.vibration_strength);
   ui.input(key);assert(ui.take_feedback()!=3);
  }
  ui.input(Key::Square);assert(!ui.feedback_preferences().vibration);
  ui.input(Key::Circle);assert(ui.feedback_preferences().vibration);
  ui.controller(0);ui.input(Key::Right);ui.take_feedback();ui.input(Key::Right);
  assert(ui.take_feedback()==3&&ui.feedback_preferences().vibration);
 }
 std::filesystem::remove(collection/"roms/dino.zip");
 {
  Gallery ui(data/"single",live,false);assert(ui.current()->id=="robby");ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT);ui.input(Key::Right);assert(ui.take_feedback()==3);ui.input(Key::Right);assert(ui.take_feedback()!=3);
  ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT);ui.input(Key::Left);assert(ui.take_feedback()==3);ui.input(Key::Left);assert(ui.take_feedback()!=3);
  ui.controller(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT);ui.input(Key::Right);assert(ui.take_feedback()==3);
 }
 {
  Gallery ui(data/"empty",Settings{},false);assert(!ui.current());ui.input(Key::Left);assert(ui.take_feedback()!=3);ui.input(Key::Cross);assert(!ui.launching());
 }
 std::filesystem::remove_all(data);
}
